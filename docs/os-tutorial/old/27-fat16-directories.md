看看标题，又是 FAT16，没错，我们又要回到文件系统当中去了。不过我保证，这就是最后一次了，本节结束以后将不再涉及与 FAT16 相关的内容。

好了，我们开始吧。既然要实现目录，首先得看怎么模拟一个带目录的磁盘来，这样才能有实现的余地。万幸的是，我们前面一直在使用的 `ftcopy` 就支持目录，这一部分也就暂告解决。

那么，在 FAT16 文件系统当中，目录是如何表示的呢？在第 18 节我们曾经提到，根目录区里是一堆 32 字节的文件结构如下：

```c
typedef struct FILEINFO {
    uint8_t name[8], ext[3]; // 文件名，扩展名
    uint8_t type, reserved[10]; // 类型，预留
    uint16_t time, date, clustno; // 修改日期，修改时间，首簇号（下一节再讲）
    uint32_t size; // 文件大小
}  __attribute__((packed)) fileinfo_t;
```

首先，目录在它的父目录那里是一个特殊的文件，它的 `type` 并非普通文件对应的 0x00 或 0x20，而是 0x10，同时它的 `size` 也为 0；其次，这个目录文件的内容也很特殊，也是一个 `fileinfo_t` 的数组，但是前两项被限定为了两个特殊的目录项：`.` 和 `..`，分别代表当前目录和父目录。

出于实现简单考虑，我们限制一个目录文件最多占 512 字节，也就是刚好一个簇（实际上 `ftcopy` 就是这么实现的）；这样，一个目录下最多只能有 14 个文件（16 个减去两个特殊的），虽说限制好像确实是有点大，但是实现起来细节就比较少了。后面我会想办法绕过这个限制的，诸位大可放心。

总之，一口吃不成个胖子，我们把目录实现分成几个阶段分别来做，让我们从第一阶段开始。

### 第一阶段：跳出根目录的限制，访问任意路径下的文件

这一个阶段总体上的流程，无非就是把“找文件”这个过程对于路径的每一层逐层进行，最后找到实际上文件的过程而已。那么最为紧要的步骤，应该是首先把一个路径当中的每一层分离出来。为了方便讲述，接下来我们把这个分离的过程称为**路径解析**。

不过，在分离的同时，我们希望能够直接在这一个层面上去掉 `.`、`..` 和多余的路径分隔符 `/`，这样或许能加快一点点的速度。这样一来，问题好像就变复杂了。

没关系，有开源抄就是好，既然 myfattools 支持目录，直接拿来就是，稍作修改即可。

**代码 27-1 实现路径解析（fs/fat16.c、include/file.h）**
```c
// 路径解析
void path_parse(char *path, path_stack_t *path_stack)
{
    path_stack->path_stack_top = 0;
    path_stack->path_stack = (char **) kmalloc(strlen(path) * sizeof(char *)); // 初始化栈
    if (path[0] != '/') { // 第一个不是/，对后续处理会有影响
        char *new_path = (char *) kmalloc(strlen(path) + 5); // 从今天起你就是新的path了
        strcpy(new_path, "/"); // 先复制一个/
        strcat(new_path, path); // 再把后续的路径拼接上
        path = new_path; // 夺舍
    }
    char *level_start = path; // 当前路径层级的起始
    char *level_end = level_start + 1; // 当前路径层级的结尾
    while (*level_end) { // 直到还没到结尾
        while (*level_end != '/' && *level_end) {
            level_end++; // 遍历直到抵达`/`
        }
        int level_len = level_end - level_start; // 这一级路径的长度（前/计后/不计）
        if (level_len == 1) { // 如果就只有后面的一个/
            level_start = level_end; // start变为现在的end
            level_end = level_start + 1; // end变为现在的start+1
            continue; // 下一层
        }
        path_stack->path_stack[path_stack->path_stack_top] = kmalloc(level_len); // 初始化这一层路径栈
        char *p = level_start + 1; // 跳过本层路径一开始的/
        strncpy(path_stack->path_stack[path_stack->path_stack_top], p, level_len - 1); // 将本层路径拷入路径栈，只拷level_len - 1（去掉一开头的/）的长度
        if (!strcmp(path_stack->path_stack[path_stack->path_stack_top], "..")) { // 如果是..
            kfree(path_stack->path_stack[path_stack->path_stack_top]); // 首先释放新的这一层
            path_stack->path_stack_top--; // 然后弹栈
            kfree(path_stack->path_stack[path_stack->path_stack_top]); // 然后旧的那一层也就可以释放了
            if (path_stack->path_stack_top < 0) path_stack->path_stack_top = 0; // 如果都弹到结尾了，那你还真是nb，避免溢出
        } else if (!strcmp(path_stack->path_stack[path_stack->path_stack_top], ".")) {
            kfree(path_stack->path_stack[path_stack->path_stack_top]); // 如果是.，那就相当于白压了，释放即可
        } else path_stack->path_stack_top++; // 否则就正常入栈
        if (!*level_end) break; // 如果已经到达结尾，直接break，不要指望一开始的while
        level_start = level_end; // start变为现在的end
        level_end = level_start + 1; // end变为start+1
    }
}

// 回收path_stack
void path_stack_deinit(path_stack_t *path_stack)
{
    for (int i = 0; i < path_stack->path_stack_top; i++) kfree(path_stack->path_stack[i]);
    kfree(path_stack->path_stack);
}
```
```c
#ifndef _FILE_H_
#define _FILE_H_

// 略...
typedef struct {
    int path_stack_top;
    char **path_stack;
} path_stack_t;
// 略...

#endif
```

具体所做的工作无非是把 `path_stack_top` 和 `path_stack` 整理到了一个结构体中，然后把代码的主体搬过来了而已。具体的逻辑就是把路径看成一个栈，每次遇到一个 / 就标记一下开始，一直遍历到下一个 / 处结束，这样就把中间这一层给拎出来了。遇到一个 .. ，则进行一个弹栈操作；. 则什么也不做（看似做了点事，其实就是把刚分配的这一层栈又释放掉了）。主要还是得看注释啊（笑）。

接下来就是让现有的操作适配目录，用到文件名的一共有 `create`、`open` 和 `delete` 三种操作，但 `write` 要更新文件属性，而这一个过程需要知道父目录，所以实际上有四个操作都需要修改。 ??~~那么是谁不需要修改呢？~~??

这几个过程其实大同小异，但都需要调用 `read_dir_entries`，而它只能读取根目录，这显然是不可接受的，必须立刻进行修改：

**代码 27-2 读取其他目录（fs/fat16.c）**
```c
// 读取一个目录的目录项，使用clustno指定簇号
fileinfo_t *read_dir_entries(int clustno, int *dir_ents)
{
    int size = SECTOR_SIZE;
    if (clustno == ROOT_DIR_START_LBA - SECTOR_CLUSTER_BALANCE) size = ROOT_DIR_SECTORS * SECTOR_SIZE;
    fileinfo_t *root_dir = (fileinfo_t *) kmalloc(ROOT_DIR_SECTORS * SECTOR_SIZE);
    memset(root_dir, 0, ROOT_DIR_SECTORS * SECTOR_SIZE);
    hd_read(clustno + SECTOR_CLUSTER_BALANCE, ROOT_DIR_SECTORS, root_dir);
    int i;
    for (i = 0; i < size / sizeof(fileinfo_t); i++) {
        if (root_dir[i].name[0] == 0) break; 
    }
    *dir_ents = i;
    return root_dir;
}
```

函数第二行做了一个申必的操作，为了让根目录对应扇区能通过加一个 `SECTOR_CLUSTER_BALANCE` 的方式得到，因此做了这个赋值。由于目录最大也就是根目录这么大，所以统一分配这么大内存，也统一按根目录标准读取。最后计算一共有几项的方法没动。

不过实际而言，根目录的簇号无论何处都是 0 而不是上面这个数字。但是由于整个程序没有对簇号直接操作的东西出现，我们的所有操作也都是使用第二行那个常数代表根目录，所以不必管它。

接下来改哪个呢？先改比较好改的 `write`。来到 `fat16_write_file` 的最后几行，替换如下：

**代码 27-3 更新文件属性（fs/fat16.c）**
```c
int fat16_write_file(fileinfo_t *finfo, int pdir_clustno, const void *buf, uint32_t size)
{
    // 上略
    finfo->time = (ctime.hour << 11) | (ctime.min << 5) | ctime.sec;
    update_file_attr(finfo, pdir_clustno); // 用此行替换中间所有行
    hd_write(ROOT_DIR_START_LBA, ROOT_DIR_SECTORS, root_dir); // 同步到硬盘
    kfree(root_dir);
    return 0;
}
```

这里不仅仅把更新文件属性的操作单独拿了个文件出来，同时给 `fat16_write_file` 还加了个参数——这个参数是为了让人知道这个文件应该被放在哪个目录里。由于声明改动，请将它同步更新到 `file.h` 中。

把更新文件属性提取出来自然是因为下一个阶段有用，它基本上就是把之前根目录有关的东西改成了任意目录：

**代码 27-4 在任意位置更新文件属性（fs/fat16.c）**
```c
void update_file_attr(fileinfo_t *finfo, int pdir_clustno)
{
    int entries;
    if (pdir_clustno == 0) pdir_clustno = ROOT_DIR_START_LBA - SECTOR_CLUSTER_BALANCE;
    fileinfo_t *root_dir = read_dir_entries(pdir_clustno, &entries);
    for (int i = 0; i < entries; i++) {
        if (!memcmp(root_dir[i].name, finfo->name, 8) && !memcmp(root_dir[i].ext, finfo->ext, 3)) {
            root_dir[i] = *finfo; // 找到对应的文件，写进根目录
            break;
        }
    }
    hd_write(pdir_clustno + SECTOR_CLUSTER_BALANCE, ROOT_DIR_SECTORS, root_dir); // 同步到硬盘
    kfree(root_dir);
}
```

流程比较简单，开头先做转换，然后读取要写入进的目录的信息，找到对应文件，直接覆盖属性，然后把目录写回。这样，对 `write` 的修改就完成了。

然后呢？然后来处理 `open`。`open` 要处理的边界情况相对较少，代码较短，也有利于我们观察新的模板。直接将原来的 `open` 推倒重来，迎面向我们走来的是适用于三个函数的修改模板：

**代码 27-5 全新的文件操作统一模板（fs/fat16.c）**
```c
// 打开文件
int fat16_open_file(fileinfo_t *finfo, char *filename)
{
    path_stack_t path_stack;
    path_parse(filename, &path_stack);
    int current_layer = 0;
    int current_layer_clustno = ROOT_DIR_START_LBA - SECTOR_CLUSTER_BALANCE;
    char sfn[20] = {0};
    int entries;
    fileinfo_t *root_dir = read_dir_entries(current_layer_clustno, &entries);
    for (current_layer = 0; current_layer < path_stack.path_stack_top; current_layer++) {
        memset(sfn, 0, sizeof(sfn));
        int ret = lfn2sfn(path_stack.path_stack[current_layer], sfn);
        if (ret) {
            path_stack_deinit(&path_stack);
            return -1;
        }
        // 在此处塞入逻辑……
    }
    return current_layer_clustno;
}
```

把原来的逻辑改吧改吧塞到注释要我们填入逻辑的地方，基本上就可以改出一个支持目录的框架来。这个框架大致是这样的：首先进行路径解析，把原来的路径拆成一层层的目录；然后，对于每一层目录，都把它拆成 sfn 的形式，如果拆不成则直接退出。再往后的逻辑中会详细处理更多事务（如进入下一层目录等），最终返回到最后一层时的簇号，这是留给 `fat16_write_file` 用的。

框架大概就是这样，下面来看看具体逻辑如何。

打开是最简单的，毕竟只需要找到文件就行。

**代码 27-6 打开文件（逻辑部分）（fs/fat16.c）**
```c
int fat16_open_file(fileinfo_t *finfo, char *filename)
{
    // 模板前部略...
        int file_index = entries; // 默认找不到
        for (int i = 0; i < entries; i++) {
            if (!memcmp(root_dir[i].name, sfn, 8) && !memcmp(root_dir[i].ext, sfn + 8, 3)) {
                file_index = i; // 找到了
                break; // 跳出
            }
        }
        if (file_index < entries) { // 在到达entries之前退出，确实是找到了
            if (current_layer == path_stack.path_stack_top - 1) { // 最后一层，也就是刚好找到要找的文件那一层
                *finfo = root_dir[file_index]; // 返回finfo
                kfree(root_dir); // 以下释放资源
                path_stack_deinit(&path_stack);
                return current_layer_clustno; // 返回当前层（也就是文件父目录）的簇号，给write用
            } else { // 否则是目录
                if (!(root_dir[file_index].type & 0x10)) { // 但是这一层根本就不是目录啊
                    kfree(root_dir); // 同样释放资源
                    path_stack_deinit(&path_stack);
                    return -1; // 不管它
                }
                current_layer_clustno = root_dir[file_index].clustno; // 当前层簇号为这个目录的簇号
                kfree(root_dir); // 释放本层目录
                root_dir = read_dir_entries(current_layer_clustno, &entries); // 读取下一层目录项，相当于进入下一层
            }
        }
        else { // 没找到，直接释放资源并退出
            kfree(root_dir);
            path_stack_deinit(&path_stack);
            return -1;
        }
    // 模板后部略...
    return current_layer_clustno;
}
```

这一部分的整体逻辑参见注释，不再赘述。

接下来删除比创建要简单些，先做删除：

**代码 27-7 删除文件（逻辑部分）（fs/fat16.c）**
```c
// 删除文件
int fat16_delete_file(char *filename)
{
    // 模板前部略...
        int file_ind = -1;
        for (int i = 0; i < entries; i++) {
            if (!memcmp(root_dir[i].name, sfn, 8) && !memcmp(root_dir[i].ext, sfn + 8, 3)) {
                file_ind = i; // 找到对应文件了
                break;
            }
        }
        if (file_ind == -1) { // 没有找到
            kfree(root_dir); // 不用删了
            path_stack_deinit(&path_stack);
            return -1;
        } else {
            if (current_layer != path_stack.path_stack_top - 1) {
                // 找到文件但不是最后一层，则做一个类似cd的操作
                if (!(root_dir[file_ind].type & 0x10)) {
                    // 不是目录你cd牛魔呢
                    kfree(root_dir); // 不用了
                    path_stack_deinit(&path_stack);
                    return -1;
                }
                current_layer_clustno = root_dir[file_ind].clustno;
                kfree(root_dir);
                root_dir = read_dir_entries(current_layer_clustno, &entries);
                continue; // 跳过后续处理
            }
        }
        path_stack_deinit(&path_stack); // 最后一层，path_stack已无用
        root_dir[file_ind].name[0] = 0xe5; // 标记为已删除
        int size = ROOT_DIR_SECTORS;
        if (current_layer > 0) /* 不在根目录 */ size = 1;
        hd_write(current_layer_clustno + SECTOR_CLUSTER_BALANCE, size, root_dir); // 更新根目录区数据
        if (root_dir[file_ind].clustno == 0) {
            kfree(root_dir); // 释放临时缓冲区
            return 0; // 内容空空，那就到这里就可以了
        }
        kfree(root_dir); // 释放临时缓冲区
        unsigned short clustno = root_dir[file_ind].clustno, next_clustno; // 开始清理文件所占有的簇
        while (1) {
            next_clustno = get_nth_fat(clustno); // 找到这个文件下一个簇的簇号
            set_nth_fat(clustno, 0); // 把下一个簇的簇号设为0，这样就找不到下一个簇了
            if (next_clustno >= 0xfff8) break; // 已经删完了，直接返回
            clustno = next_clustno; // 下一个簇设为当前簇
        }
    // 模板后部略...
    return 0; // 注意此处与模板不同！
}
```

删除文件不需要返回簇号，因此返回 0 即可。它的前半部分逻辑是打开的逻辑，后半部分逻辑是之前删除的逻辑，具体细节可以参见注释，这里同样不再赘述。

最后一部分内容是创建。创建文件不知是不是我想复杂了，行数写得极其多，或许还能化简，还望各位斧正。

**代码 27-8 创建文件（逻辑部分）（fs/fat16.c）**
```c
int fat16_create_file(fileinfo_t *finfo, char *filename)
{
    // 模板前部略...
        int free_slot = entries; // 默认的空闲位置是最后一个
        int exist = -1;
        for (int i = 0; i < entries; i++) {
            if (!memcmp(root_dir[i].name, sfn, 8) && !memcmp(root_dir[i].ext, sfn + 8, 3)) { // 文件名和扩展名都一样
                exist = i; // 虽然存在，但还得再看一看
            }
            if (root_dir[i].name[0] == 0xe5) { // 已经删除（文件名第一个字节是0xe5）
                free_slot = i; // 那就把这里当成空闲位置
                break;
            }
        }
        if (exist != -1 && current_layer == path_stack.path_stack_top - 1) {
            // 在最后一层存在，那自然不用创建
            kfree(root_dir);
            path_stack_deinit(&path_stack);
            return -1;
        }
        if (exist != -1 && current_layer != path_stack.path_stack_top - 1) {
            // 不在最后一层存在，则做一个类似cd的操作
            if (!(root_dir[exist].type & 0x10)) {
                // 不是目录你cd牛魔呢
                kfree(root_dir); // 不用了
                path_stack_deinit(&path_stack);
                return -1;
            }
            current_layer_clustno = root_dir[exist].clustno;
            kfree(root_dir);
            root_dir = read_dir_entries(current_layer_clustno, &entries);
            continue; // 跳过后续处理
        }
        if (exist == -1 && current_layer != path_stack.path_stack_top - 1) {
            // 不在最后一层还不存在，中间有一点不存在，那不是我的问题
            kfree(root_dir);
            path_stack_deinit(&path_stack);
            return -1;
        }
        // 到此应该只剩下文件不存在且当前是最后一层的情况
        path_stack_deinit(&path_stack); // 最后一层也不需要path_stack了
        int size = current_layer > 0 ? 1 : ROOT_DIR_SECTORS; // 本层目录一共有多少个扇区？只要不是第一层，就只占一个扇区；
        if (free_slot == size * SECTOR_SIZE / sizeof(fileinfo_t)) { // 如果空闲位置已经到达本层目录末尾
            kfree(root_dir); // 没地方创建也就不用创建了
            return -1;
        }
        // 开始填入fileinfo_t对应的项
        memcpy(root_dir[free_slot].name, sfn, 8); // sfn为name与ext的合体，前8个字节是name
        memcpy(root_dir[free_slot].ext, sfn + 8, 3); // 后3个字节是ext
        root_dir[free_slot].type = 0x20; // 类型为0x20（正常文件）
        root_dir[free_slot].clustno = 0; // 没有内容，所以没有簇号（同样放在下一节讲）
        root_dir[free_slot].size = 0; // 没有内容，所以大小为0
        memset(root_dir[free_slot].reserved, 0, 10); // 将预留部分全部设为0
        current_time_t ctime;
        get_current_time(&ctime); // 获取当前时间
        // 按照前文所说依次填入date和time
        root_dir[free_slot].date = ((ctime.year - 1980) << 9) | (ctime.month << 5) | ctime.day;
        root_dir[free_slot].time = (ctime.hour << 11) | (ctime.min << 5) | ctime.sec;
        if (finfo) *finfo = root_dir[free_slot]; // 创建完了不能不管，传给finfo留着
        hd_write(current_layer_clustno + SECTOR_CLUSTER_BALANCE, size, root_dir); // 将新的根目录区写回硬盘
        kfree(root_dir); // 成功完成
    // 模板后部略...
    return current_layer_clustno;
}
```

这个逻辑大致是这样的：一开始同步寻找当前层的文件和当前层的空闲位置。在两个部分都完成后，分成以下四种情况讨论：

> 当前层文件存在，且当前层是最后一层。

待创建的文件已经存在，那么直接释放资源并退出即可。

> 当前层文件存在，且当前层不是最后一层。

也就是说，中间某一层目录是存在的。参考上面的打开、删除操作，先判断这一层是不是目录，如果是，则将当前层切换到这层目录里去，跳过后续所有处理。

> 当前层文件不存在，且当前层不是最后一层。

路径在中间断开，后续完全不用找了，同样释放资源并退出。

> 当前层文件不存在，且当前层不是最后一层。

这里没有显式的 `if`，因为上面三种情况对应的执行流均无法到达此处。这正是最理想的情况，我们这就要在本层创建对应的文件。

接下来创建的逻辑和第 18 节类似，故不再重复。

对 FAT16 底层操作部分的修改到此完全结束，本文后续将不再对 fat16.c 进行修改。

把这一部分的代码接入到 `file.c` 是相当顺畅的：首先，在 `file.h` 中找到 `file_t`，新增一个成员：

**代码 27-9 把父目录存一下（fs/file.c）**
```c
typedef struct FILE_STRUCT {
    void *handle;
    void *buffer;
    int pos;
    int size;
    int open_cnt;
    int pdir_clustno; // 这个成员是新添加的
    file_type_t type;
    oflags_t flags;
} file_t;
```

然后在 `sys_open` 中，把修改过的 `create` 与 `open` 的返回值存到这个成员里：

**代码 27-10 把父目录存一下（2）（fs/file.c）**
```c
int sys_open(char *filename, uint32_t flags)
{
    // 上略
    file_table[global_fd].buffer = kmalloc(finfo.size + 5); // 分配一个缓冲区
    file_table[global_fd].pdir_clustno = status; // 此行是新添加的
    // 下略
}
```

最后在 `sys_write` 中，给 `fat16_write_file` 传递父目录：

**代码 27-11 把父目录取出来（fs/file.c）**
```c
int sys_write(int fd, const void *msg, int len)
{
    // 上略
    int status = fat16_write_file(cfile->handle, cfile->pdir_clustno /* 这里是新添加的 */, cfile->buffer, cfile->size); // 写入完毕，立刻更新到硬盘
    // 下略
}
```

至此，已经可以支持对任意目录下的文件进行操作，但是对目录本身，我们依然什么都做不了。因此，是时候进入第二阶段了：

### 第二阶段：实现目录操作

文件系统也是一种数据结构，其中的目录也是，因此目录的操作，无外乎增、删、改、查。增，即创建目录；删，即删除目录；改，即修改目录项，这个工作是 `sys_open` 等一系列操作文件的函数完成的；查，即读取目录项。需要注意的是，`sys_read` 读取的是目录项的内容而非目录项本身。

因此，一共有三个功能需要实现：创建目录、删除目录、读取目录项，它们分别由函数 `mkdir`、`rmdir` 和 `readdir` 实现。依照文件系统的惯例，读取之前要先打开，有打开就有关闭，因此还有配套的 `opendir`、`closedir`；有读取就想到读取指针，因此还提供了 `rewinddir` 用来把读取指针指回开头。

综上，这一阶段一共有六个函数需要我们实现，我们一个一个来。

按照顺序，先实现 `mkdir`。`mkdir` 的总体流程如下：

> 1.先创建一个和目录同名的文件。
>
> 2.既然要求一个目录必须有.和..的目录项，就先创建它们——目录的内容是一个 `fileinfo_t` 数组，因此相当于填充一个数组的前两个成员。
>
> 3.把文件变成目录，具体而言，把它的 `type` 从 0x20 变成 0x10，把它的 `size` 变成 0，然后更新文件属性。
>
> 4.于是一个空目录就创建出来了。

这几步都在我们的能力范围之内，

### 第三阶段：让任务知道目录，实现相对路径

### 第四阶段：实现文件相关应用程序及命令：`ls`、`mkdir`、`pwd`、`touch`、`rm`、`cd`