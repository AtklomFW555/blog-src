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

上面用到了两个尚未定义的标准库函数 `strcat` 和 `strncpy`，内容如下：

**代码 27-2 `strcat`、`strncpy`（lib/string.c）**
```c
char *strcat(char *dst_, const char *src_)
{
    char *str = dst_;
    while (*str++);
    --str;
    while((*str++ = *src_++));
    return dst_;
}

char *strncpy(char *dst_, const char *src_, int n)
{
    char *str = dst_;
    while (n && (*dst_++ = *src_++)) n--;
    if (n) while (n--) *dst_++ = '\0';
    return str;
}
```

接下来就是让现有的操作适配目录，用到文件名的一共有 `create`、`open` 和 `delete` 三种操作，但 `write` 要更新文件属性，而这一个过程需要知道父目录，所以实际上有四个操作都需要修改。 ??~~那么是谁不需要修改呢？~~??

这几个过程其实大同小异，但都需要调用 `read_dir_entries`，而它只能读取根目录，这显然是不可接受的，必须立刻进行修改：

**代码 27-3 读取其他目录（fs/fat16.c）**
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

**代码 27-4 更新文件属性（fs/fat16.c）**
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

**代码 27-5 在任意位置更新文件属性（fs/fat16.c）**
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

**代码 27-6 全新的文件操作统一模板（fs/fat16.c）**
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

**代码 27-7 打开文件（逻辑部分）（fs/fat16.c）**
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

**代码 27-8 删除文件（逻辑部分）（fs/fat16.c）**
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

**代码 27-9 创建文件（逻辑部分）（fs/fat16.c）**
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

**代码 27-10 把父目录存一下（fs/file.c）**
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

**代码 27-11 把父目录存一下（2）（fs/file.c）**
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

**代码 27-12 把父目录取出来（fs/file.c）**
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

这几步都在我们的能力范围之内，开写。

**代码 27-13 创建空目录（fs/file.c）**
```c
int sys_mkdir(const char *path)
{
    fileinfo_t finfo; // 待创建目录对应的finfo
    int pdir_clustno = fat16_create_file(&finfo, (char *) path); // 创建对应文件
    if (pdir_clustno == -1) return -1; // 已有或有其他妙妙小问题，异常退出
    if (pdir_clustno == ROOT_DIR_START_LBA - SECTOR_CLUSTER_BALANCE) pdir_clustno = 0; // 不知道在转换什么，反正最终得转换回去
    char *clust = (char *) kmalloc(512); 
    int status = fat16_write_file(&finfo, pdir_clustno, clust, 512);
    kfree(clust);
    if (status == -1) return -1; // 上面四行分配了一个空簇，finfo.clustno就是这个空簇的簇号
    fileinfo_t *content = (fileinfo_t *) kmalloc(512); // 这是真正的内容
    strcpy(content[0].name, ".          "); // 第一个目录项是.，代表当前目录
    strcpy(content[1].name, "..         "); // 第二个目录项是..，代表父目录
    content[0].type = content[1].type = 0x10; // 两个东西都是目录
    content[0].size = content[1].size = 0; // 目录大小都是0
    content[0].date = content[1].date = finfo.date; // 和目录本身不求同年同月同日生
    content[0].time = content[1].time = finfo.time; // 但求同年同月同日死
    content[0].clustno = finfo.clustno; // 当前目录的簇号就是刚才拿到空簇的簇号，反正都是一个扇区，覆盖不会导致新簇加入
    content[1].clustno = pdir_clustno; // 父目录的簇号上面返回的时候已经拿到了
    memset(content[0].reserved, 0, 10);
    memset(content[1].reserved, 0, 10);
    status = fat16_write_file(&finfo, pdir_clustno, content, 512); // 把这两个目录项写入进去
    if (status == -1) return -1; // 写入失败则创建失败
    kfree(content); // 现在不再需要内容
    finfo.type = 0x10; // 哈哈 其实我是目录
    finfo.size = 0; // 我根本没有大小
    update_file_attr(&finfo, pdir_clustno); // 更新文件属性
    return 0;
}
```

感觉把它放到 `fat16.c` 里更合适呢？算了不管了。总之这个流程就是上面的流程，注释也就写得放飞自我一点，各位想必都看得懂代码我也就不管啦。

按照顺序，下一个是删，也就是 `rmdir`。和删除无牵无挂的普通文件相比，目录是拖家带口的，上有老不一定，下有小（目录项）倒是大有可能，只有这个目录也无牵无挂了，我们才能删掉它——也就是说，只有空目录才能 `rmdir`。

空目录里并不是没有目录项，而是还有两个无法删除的目录项（`.` 和 `..`），判空时要尤其注意。而要知道一个目录里到底有几个目录项，最好的办法是直接使用 `read_dir_entries`。

注意到，删除文件用的 `fat16_delete_file` 并不在乎文件属性，所以判断目录为空以后可以直接用它来删除。

**代码 27-14 删除空目录（fs/file.c）**
```c
int sys_rmdir(const char *path)
{
    if (strcmp(path, "/") == 0) return -1; // 不允许删除根目录
    fileinfo_t finfo; // 待删除目录对应的finfo
    memset(&finfo, 0, sizeof(finfo));
    int pdir_clustno = fat16_open_file(&finfo, (char *) path); // 打开对应文件
    if (pdir_clustno == -1 || !(finfo.type & 0x10)) {
        if (is_relative) kfree((char *) path);
        return -1; // 有妙妙小问题，异常退出
    }
    int entries;
    fileinfo_t *dir_ents = read_dir_entries(finfo.clustno, &entries); // 读取目录项
    if (entries != 2) {
        // 只有2个目录项（.和..）才可删除
        // 被删除的目录项也会被计入，所以要判断一下其他目录项是还在还是被删了
        int delete_cnt = 0;
        for (int i = 2; i < entries; i++) {
            if (dir_ents[i].name[0] == 0xe5) delete_cnt++;
        }
        if (entries - delete_cnt != 2) {
            // 去掉被删的还是多，这是真非空
            return -1;
        }
    }
    kfree(dir_ents); // 其实只是为了拿到entries
    // 否则即可删，删除之
    int ret = fat16_delete_file((char *) path);
    return ret;
}
```

第一行首先判断是不是根目录，显然根目录我们碰都不应该碰。然后抓一个 finfo 过来，并用它装好打开之后的文件标识，如果打开文件返回 -1 或者使用 `rmdir` 删除的文件不是目录，都立即报错退出。再往下确认是目录以后，读取目录项并判断是否为空，如果为空，再去试图删除这个目录。

再往下，是改的操作，所有对普通文件进行操作的东西，都不能且不应该操作目录。具体而言，`unlink` 这个直接操作路径的函数，不应该接受除了普通文件以外的任何文件，在此对它进行修改：

**代码 27-15 文件与目录之辨（fs/file.c）**
```c
int sys_unlink(const char *filename)
{
    fileinfo_t finfo;
    int status = fat16_open_file(&finfo, (char *) filename);
    if (status == -1 || (finfo.type & 0x10)) {
        if (is_relative) kfree((char *) filename);
        return -1;
    }
    status = fat16_delete_file((char *) filename); // 直接套皮，不多说    
    return status;
}
```

一旦发现对应的文件其实是目录，函数便立刻中止，不再进行后续操作。至于 `open`，Linux 里倒是能用它打开目录，这里也支持一下算了。

注意，根目录没有对应的 `fileinfo_t` 结构，需要在 `open` 中特殊处理：

**代码 27-16 特判根目录（fs/file.c）**
```c
    if (!strcmp(filename, "/")) {
        // 根目录没有对应的fileinfo结构，自然打开什么的都不用管
        // 直接就地构造一个handle即可
        strcpy(finfo.name, "root       ");
        finfo.type = 0x10;
        finfo.size = 0;
        finfo.clustno = 0;
        current_time_t ctime;
        get_current_time(&ctime); // 获取当前时间
        // 按照前文所说依次填入date和time
        finfo.date = ((ctime.year - 1980) << 9) | (ctime.month << 5) | ctime.day;
        finfo.time = (ctime.hour << 11) | (ctime.min << 5) | ctime.sec;
        status = 0;
    } else { // 新增部分到此结束
        if (flags & O_CREAT) { // flags中含有O_CREAT，则需要创建文件
            status = fat16_create_file(&finfo, filename); // 调用创建文件的函数
            if (status == -1) {
                return status; // 创建失败则直接不管
            }
        } else {
            status = fat16_open_file(&finfo, filename); // 调用打开文件的函数
            if (status == -1) {
                return status; // 打开失败则直接不管
            }
        }
    }
```

由于使用了 `current_time_t` 及附属结构，需要在 `file.c` 开头加上 `#include "cmos.h"`。

最后一步，是查。具体地，是 `opendir`、`readdir`、`rewinddir`、`closedir` 这四个函数。

这几个函数的处理相对而言较为复杂，因为涉及到在操作系统内部操作应用程序的内存。不过只要时刻留意，哪个是给操作系统用的，哪个是给应用程序用的，哪个经过转换，哪个没经过转换，基本上问题也不大。

`opendir` 返回的是一个 `DIR` 结构的指针，`readdir` 则接收一个 `DIR` 结构的指针返回一个 `struct dirent *`，而 `struct dirent *` 总共也没多少东西，显然光靠这四个函数是不够查询目录项的。事实上，真正用来查询一个文件的信息的函数叫做 `stat`，还有变体 `fstat` 等，较为复杂，但获取信息比自己绞尽脑汁去偷（比如经典的偷文件大小）要多。

想要查询一个目录项我得先知道有哪些目录项，所以最终又回到了上面那四个函数。先从一切的起源——`opendir` 开始。

为了方便实现，我把 `DIR` 结构定义成一个 `struct dirent` 的数组，还有一些附加成员比如 `pos` 之类记录已经读到了第几个。

**代码 27-17 目录的抽象表示（include/dirent.h）**
```c
#ifndef _DIRENT_H_
#define _DIRENT_H_

#define MAX_FILE_NUM 512

struct dirent {
    char name[20]; // 给多了
    int size;
};

typedef struct {
    struct dirent dir_entries[MAX_FILE_NUM]; // 一个目录下最多这么多文件
    int entry_count; // 总共多少个目录项
    int pos;
} DIR;

#endif
```

新建了一个文件 `dirent.h`，首先是因为标准是这么写的，其次则是很多地方都要用到这个文件，不单分出来也没有办法。

在 `file.h` 的函数声明前面加上 `#include "dirent.h"`，然后就可以开始写 `opendir` 了。

**代码 27-18 打开目录（fs/file.c）**
```c
DIR *sys_opendir(const char *name)
{
    fileinfo_t *dir_ents;
    int entries;
    if (strcmp(name, "/") == 0) {
        dir_ents = read_dir_entries(ROOT_DIR_START_LBA - SECTOR_CLUSTER_BALANCE, &entries);
    } else {
        fileinfo_t finfo; // 待删除目录对应的finfo
        memset(&finfo, 0, sizeof(finfo));
        int pdir_clustno = fat16_open_file(&finfo, (char *) name); // 打开对应文件
        if (pdir_clustno == -1 || !(finfo.type & 0x10)) return NULL; // 有妙妙小问题，异常退出
        dir_ents = read_dir_entries(finfo.clustno, &entries);
    }
    // 总之折腾完后应该是拿到entries了
    // 为DIR *分配内存需要使用malloc
    // 笑点解析：sys_opendir(r0) -> malloc(r3) -> sbrk(r3) -> sys_sbrk(r0)
    DIR *ret = (DIR *) malloc(sizeof(DIR));
    // 现在是在r0的段 所以ret要加上ds_base才能正确更新到r3的ret里
    ret = (DIR *) ((char *) ret + (task_now()->ds_base));
    memset(ret, 0, sizeof(DIR));
    ret->pos = 0;
    int ret_entry_index = 0;
    for (int i = 0; i < entries; i++) {
        if (dir_ents[i].name[0] == 0xe5) continue;
        ret->dir_entries[ret_entry_index].size = dir_ents[i].size;
        int ret_name_index = 0;
        // 处理文件名
        for (int j = 0; j < 8; j++) {
            char alpha = dir_ents[i].name[j];
            if (alpha == ' ') break;
            if (alpha >= 'A' && alpha <= 'Z') alpha += 0x20;
            ret->dir_entries[ret_entry_index].name[ret_name_index++] = alpha;
        }
        for (int j = 0; j < 3; j++) {
            char alpha = dir_ents[i].ext[j];
            if (alpha == ' ') break;
            if (j == 0) ret->dir_entries[ret_entry_index].name[ret_name_index++] = '.';
            if (alpha >= 'A' && alpha <= 'Z') alpha += 0x20;
            ret->dir_entries[ret_entry_index].name[ret_name_index++] = alpha;
        }
        ret_entry_index++;
    }
    ret->entry_count = ret_entry_index;
    // 把ret减回去
    ret = (DIR *) ((char *) ret - (task_now()->ds_base));
    // 至此dir初始化完成
    return ret;
}
```

这一段代码总体上可分为两大块：第一块，是获取所有目录项；第二块，是把获取到的目录项转化一下，这里特指转化文件名。

开头 11 行是第一部分的处理，首先判断是不是根目录（因为根目录没有对应的 fileinfo 结构），如果是就直接读根目录，否则按照 finfo 结构里的 clustno 读出目录项。下面调用 `malloc` **在用户空间**分配了一块内存，反正 DIR 指针最后是给用户了，只能调用 malloc。如此一来，明明本来是在内核 r0 级别的 `sys_opendir`，要去调用用户 r3 级别的 `malloc`，之后 `malloc` 再调用 r0 级别的 `sys_sbrk`，真是弯弯绕啊。

接下来为了操控 `ret`，我们还需要让内核能够访问到 `ret`，但 `malloc` 返回的地址是用户空间内的地址，还要加上 `ds_base` 才能为内核所控。`char *` 指针以一字节为基本单位，因此把 `ret` 转换成 `char *` 再加 `ds_base` 就是加 `ds_base` 这么多字节。

接下来的操作只是把 FAT16 认为的 8.3 文件名转换成平常常用的那种文件名，由于中间可能删除了一些项，需要一个 `ret_entry_index` 来单独记录下一个文件信息应该放在哪里。最后给出目录内一共有多少个项，然后把 `ds_base` 减回去，这就初始化完了一个目录的软件结构。

接下来的三个处理 `DIR` 结构的函数加起来还没有上面那一个长：

**代码 27-19 操作目录（fs/file.c）**
```c
struct dirent *sys_readdir(DIR *dir)
{
    // C语言笑传之查查边
    if (dir->pos >= dir->entry_count) return NULL;
    struct dirent *ret = &dir->dir_entries[dir->pos++];
    ret = (struct dirent *) ((char *) ret - (task_now()->ds_base));
    return ret;
}

void sys_rewinddir(DIR *dir)
{
    dir->pos = 0;
}

void sys_closedir(DIR *dir)
{
    free((char *) dir - (task_now()->ds_base));
}
```

三个部分都很简单。第一个部分读取目录项，只需直接从数组当中取，目录结构中有一个 `pos` 记录读到了哪里，从那读出来再加一即可；开头查了一下边界情况。需要注意的是虽然 `dir` 指针是给我们自动加了一个 `ds_base`，但是返回的 `ret` 指针没有，需要我们自己减掉，应用程序才能知道。第二个部分直接把 `pos` 设置为 0。第三个部分直接 `free` 掉，由于 `free` 本体在用户空间还需要把 `ds_base` 减掉。

最后，为了能让 `ls` 之类的应用程序获取到更多信息，还要再添加一个函数 `stat`。`stat` 是用来获取文件信息的，基本上我们能给什么就给什么。目前，我们能给出的信息也就只有：文件大小、文件最后修改时间、文件属性（也只有是不是目录这一项）这三种。`stat` 还有变体 `fstat` 和 `lstat`，`stat` 和 `lstat` 接收的是文件名，而 `fstat` 接收的是文件描述符；`stat` 与 `lstat` 的区别在我们没实现的符号链接上体现，可以认为二者没有区别。

下面先添加表示文件信息的结构体 `struct stat`：

**代码 27-20 文件信息结构体 `struct stat`（include/fcntl.h、include/time.h）**
```c
#ifndef _STAT_H_
#define _STAT_H_

#include "time.h"

typedef enum FILE_TYPE {
    FT_USABLE,
    FT_REGULAR,
    FT_DIRECTORY,
    FT_UNKNOWN
} file_type_t;

typedef enum oflags {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
} oflags_t;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct stat {
    uint32_t st_size;
    file_type_t st_type;
    struct tm st_time;
};

int stat(const char *filename, struct stat *st);

#endif
```
```c
#ifndef _TIME_H_
#define _TIME_H_

struct tm {
    int tm_year, tm_month, tm_mday, tm_hour, tm_min, tm_sec;
};

#endif
```

在 `file.h` 中再加一行 `#include "fcntl.h"`，单开一个文件的理由和 `dirent.h` 一致。这里顺便挖了俩东西过来（记得在原来的地方把它删了）又把 `stat` 声明撂在这真没什么意思。

`st_time` 文件夹本应放置时间戳，但因为??我不会??系统目前没有操作时间戳的工具，所以被迫使用了类似的 `struct tm`。

由于 `stat` 最后肯定要打开文件，所以在系统端我们实现的是 `fstat`：

**代码 27-21 获取文件信息用 `sys_fstat`（fs/file.c）**
```c
int sys_fstat(int fd, struct stat *st)
{
    // 从fd获取fileinfo_t
    int global_fd = task_now()->fd_table[fd];
    fileinfo_t *finfo = (fileinfo_t *) file_table[global_fd]->handle;
    // 向stat结构体填充信息
    st->st_size = finfo->size;
    if (finfo->type & 0x10) st->st_type = FT_DIRECTORY;
    else st->st_type = FT_REGULAR;
    st->st_time.tm_year = ((finfo->date & 0xfe00) >> 9) + 1980;
    st->st_time.tm_month = (finfo->date & 0x01e0) >> 5;
    st->st_time.tm_mday = finfo->date & 0x001f;
    st->st_time.tm_hour = (finfo->time & 0xf800) >> 11;
    st->st_time.tm_min = (finfo->time & 0x07e0) >> 5;
    st->st_time.tm_sec = finfo->time & 0x001f;
    return 0;
}
```

这两步简单直接，不用我多讲了吧。对 `date` 和 `time` 的拆分是第 18 节创建文件时合并的逆过程，请自行参阅，这里不再赘述。

对目录的操作也已完成，但总感觉还差点什么？对了，现在所有的路径都是绝对路径，相对路径还没有实现，赶快进入下一个阶段。

### 第三阶段：让任务知道目录，实现相对路径

相对路径相对路径，总得“相对”点什么东西吧。相对了个啥呢？我们随便打开一个 shell，看看旁边的提示符，不管打扮得多花哨，一般都有一个路径，它有另一个名字，叫做**工作目录**。这个工作目录，说白了就是一个任务认为自己在一个什么路径里。

既然它是每一个任务独有的，自然应该被放在我们用来表示任务的 `task_t` 结构体里：

**代码 27-22 新版任务结构体（include/mtask.h）**
```c
typedef struct TASK {
    uint32_t sel;
    int32_t flags;
    exit_retval_t my_retval;
    int fd_table[MAX_FILE_OPEN_PER_TASK];
    gdt_entry_t ldt[2];
    int ds_base;
    bool is_user;
    void *brk_start, *brk_end;
    char *work_dir;
    tss32_t tss;
} task_t;
```

对于这样一个新成员，自然应该在 `task_alloc` 中初始化它：

**代码 27-23 默认工作目录（kernel/mtask.c）**
```c
task_t *task_alloc()
{
    // 上略
            task->is_user = false;
            task->work_dir = kmalloc(5);
            strcpy(task->work_dir, "/"); // 默认工作目录为根目录
            return task;
    // 下略
}
```

给所有任务的默认工作目录都是根目录，反正到时候应用程序启动的时候都会重新改。

在实现相对路径有关的所有 API 以前，需要让所有与路径有关的程序首先把相对路径转化为绝对路径。这些程序包括：`open`、`unlink`、`opendir`、`mkdir` 以及 `rmdir`。

下面正式开始转化。其实转化的过程非常简单：第一步，把任务的工作目录和相对路径拼接到一起；第二步，使用 `path_parse` 把它拆成各层，这一步是为了去除多余的 `/` 并且简化 `.` 和 `..`；第三步，把各层再拼起来，就得到了真正的绝对路径。

将以上三步转化成代码，就形成了相对路径转绝对路径的程序 `rel2abs`。

**代码 27-24 相对路径转绝对路径（fs/file.c）**
```c
static char *rel2abs(const char *path)
{
    char *abspath = (char *) kmalloc(strlen(task_now()->work_dir) + strlen(path) + 5);
    strcpy(abspath, task_now()->work_dir);
    strcat(abspath, path);
    path_stack_t path_stack; path_stack.path_stack_top = 0;
    path_parse(abspath, &path_stack);
    memset(abspath, 0, strlen(abspath));
    abspath[0] = '/';
    for (int i = 0; i < path_stack.path_stack_top; i++) {
        strcat(abspath, path_stack.path_stack[i]);
        strcat(abspath, "/");
    }
    // 至此相对路径已转换为绝对路径存储至abspath
    path_stack_deinit(&path_stack);
    return abspath;
}
```

接下来就是把 `rel2abs` 接入到各个程序中，这一步的关键是确认一个路径是不是相对路径：如果一个路径以 `/` 这个根目录开头，便认为它一定是绝对路径。由于 `rel2abs` 用到 `kmalloc`，如果经过转换则此路径应当被 `kfree`。

给上面提到的这五个程序（再列一遍：`open`、`unlink`、`opendir`、`mkdir` 以及 `rmdir`）都加上一头一尾：

**代码 27-25 处理相对路径转绝对路径（fs/file.c）**
```c
{
    int is_relative = false;
    // 先处理相对路径
    if (filename[0] != '/') {
        is_relative = true;
        filename = rel2abs(filename);
    }
    // 中略...
    if (is_relative) kfree(filename);
    // 返回略...
}
```

在所有错误退出的地方还要提前判断以便释放资源，这一部分不同函数不一样就不打了。不同的函数变量名还不大一样，有的是 `filename`，有的是 `name`，有的是 `path`，懒得管了，到时候该改的就改。

现在已经实现的函数就都已经实现相对路径了。但是，虽然有了相对路径，却没有修改相对路径的方法，而这归根到底，是对工作目录这个字段进行读写。读使用的 API 是 `getcwd`，写使用的 API 是 `chdir`，我们来逐一实现它。

`getcwd` 的函数签名长这样：

```c
char *getcwd(char *buf, int len);
```

由用户给出缓冲区和长度，工作目录的路径就放在这个缓冲区里。当缓冲区为 NULL 时，则认为用户请求操作系统进行 `malloc`；当缓冲区为 NULL 且 `len` 为 0 时，则认为用户让操作系统分配大小合适的缓冲区。`malloc` 得来的缓冲区统一由 `getcwd` 返回。

实现起来细节还不少，先看代码。

**代码 27-26 `getcwd`（fs/file.c）**
```c
char *sys_getcwd(char *buf, int size)
{
    task_t *task = task_now();
    buf -= task->ds_base;
    char *res = buf;
    if (size && strlen(task->work_dir) >= size) return NULL; // 装不下
    if (!size && buf) return NULL; // 大小为0又不malloc，你要干什么！
    if (!buf) {
        if (size) res = malloc(size);
        else res = malloc(strlen(task->work_dir) + 5);
    }
    res += task->ds_base;
    strcpy(res, task->work_dir);
    res -= task->ds_base;
    return res;
}
```

首先因为要给 `buf` 判空，把 `buf` 减掉 `ds_base`。接下来分别判断各类异常情况，但主要就是装不下的情况。然后处理缓冲区为 NULL，这里搞了一个 `res` 变量，它存的是减去 `ds_base` 之后的 `buf`，这样后面 `malloc` 也要加上 `ds_base`，就顺便归到一起了。然后把 `res` 加上 `ds_base`，把工作目录路径复制过去，再减回去让用户能访问到这块内存，最后返回它。

接下来 `chdir` 需要判断一下这个工作目录到底是不是个目录，存不存在：

**代码 27-27 `chdir`（fs/file.c）**
```c
int sys_chdir(const char *path)
{
    task_t *task = task_now();
    bool is_relative = false;
    if (path[0] == '/') {
        is_relative = true;
        path = rel2abs(path);
    }
    fileinfo_t finfo;
    int status = fat16_open_file(&finfo, path);
    if (status == -1 || !(finfo.type & 0x10)) {
        if (is_relative) kfree(path);
        return -1;
    }
    kfree(task->work_dir);
    task->work_dir = kmalloc(strlen(path) + 5);
    strcpy(task->work_dir, path);
    if (is_relative) kfree(path);
    return 0;
}
```

这个简单到看代码就能理解不用我多说了吧。

差点忘了还有一件事，开始运行新应用程序的时候要设置它的工作目录，这是在 `create_process` 中通过早年预留的 `work_dir` 传递的：

**代码 27-28 设置新任务工作目录（kernel/exec.c）**
```c
// 上略...
    new_task->work_dir = kmalloc(strlen(work_dir) + 5);
    strcpy(new_task->work_dir, work_dir);
// 下略...
```

把这两行放在 `task_run(new_task)` 之前即可。严格来说应该判断一下工作目录存不存在的，但好像没有判断的办法，所以就不管它了。

现在可以说我们对目录的实现已经完整，但这些东西都还没有经过测试。实践出真知，我们来写点应用程序还有命令，用上这些操作，结束本节内容。

### 第四阶段：实现文件相关应用程序及命令：`ls`、`mkdir`、`pwd`、`rm`、`cd`

既然提到应用程序还有命令，这些都是用户层的东西，上面实现了一堆 `sys_xxx`，但到头来一个系统调用都没添加。正好借着这个机会，把系统调用一块再小小修改一下：既然 `syscall_impl.asm` 里的函数到最后都是在套公式，那还不如直接写成宏算了。

新版的 `syscall_impl.asm` 长这样。这比以往应该简单多了吧……

**代码 27-29 系统调用小改（kernel/syscall_impl.asm）**
```asm
section .text

%macro SYSCALL0 2
[global %1]
%1:
    mov eax, %2
    int 80h
    ret
%endmacro

%macro SYSCALL1 2
[global %1]
%1:
    push ebx
    mov eax, %2
    mov ebx, [esp + 8]
    int 80h
    pop ebx
    ret
%endmacro

%macro SYSCALL2 2
[global %1]
%1:
    push ebx
    mov eax, %2
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    int 80h
    pop ebx
    ret
%endmacro

%macro SYSCALL3 2
[global %1]
%1:
    push ebx
    mov eax, %2
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    mov edx, [esp + 16]
    int 80h
    pop ebx
    ret
%endmacro

SYSCALL0 getpid, 0
SYSCALL3 write, 1
SYSCALL3 read, 2
SYSCALL2 open, 3
SYSCALL1 close, 4
SYSCALL3 lseek, 5
SYSCALL1 unlink, 6
SYSCALL3 create_process, 7
SYSCALL1 waitpid, 8
SYSCALL1 exit, 9
SYSCALL1 sbrk, 10
SYSCALL1 opendir, 11
SYSCALL1 readdir, 12
SYSCALL1 rewinddir, 13
SYSCALL1 closedir, 14
SYSCALL1 mkdir, 15
SYSCALL1 rmdir, 16
SYSCALL2 fstat, 17
SYSCALL1 chdir, 18
SYSCALL2 getcwd, 19
```

`SYSCALL` 系列宏的第一个参数是系统调用名，第二个参数是系统调用号。而 `SYSCALL` 后面的那个数字，代表这个系统调用接收的参数数量。以后添加系统调用，不用再费事打代码，一行 `SYSCALLx name, id` 完事。

接下来在 `syscall.c` 中添加调用它们的代码。

**代码 27-30 添加新系统调用（kernel/syscall.c）**
```c
// 上略...
        case 11:
            ret = (int) sys_opendir((char *) ((const char *) ebx + ds_base));
            break;
        case 12:
            ret = (int) sys_readdir((DIR *) ((const char *) ebx + ds_base));
            break;
        case 13:
            sys_rewinddir((DIR *) ((const char *) ebx + ds_base));
            break;
        case 14:
            sys_closedir((DIR *) ebx);
            break;
        case 15:
            ret = sys_mkdir((const char *) ebx + ds_base);
            break;
        case 16:
            ret = sys_rmdir((const char *) ebx + ds_base);
            break;
        case 17:
            ret = sys_fstat(ebx, (struct stat *) ((char *) ecx + ds_base));
            break;
        case 18:
            ret = sys_chdir((const char *) ebx + ds_base);
            break;
        case 19:
            ret = (int) sys_getcwd((char *) ebx + ds_base, ecx);
            break;
// 下略...
```

想必我不说大家也应该知道这些东西应该放在哪（

对了，记得在 `unistd.h` 里加上函数声明，还要 include 两个头文件，懒得改的直接抄下面的这个：

**代码 27-31 系统调用列表（include/unistd.h）**
```c
#ifndef _UNISTD_H_
#define _UNISTD_H_

#include "stdint.h"
#include "dirent.h"
#include "fcntl.h"

int open(char *filename, uint32_t flags);
int write(int fd, const void *msg, int len);
int read(int fd, void *buf, int count);
int close(int fd);
int lseek(int fd, int offset, uint8_t whence);
int unlink(const char *filename);
int waitpid(int pid);
int exit(int ret);
void *sbrk(int incr);
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
void closedir(DIR *dir);
int mkdir(const char *path);
int rmdir(const char *path);
int fstat(int fd, struct stat *st);
int chdir(const char *path);
char *getcwd(char *buf, int size);

int create_process(const char *app_name, const char *cmdline, const char *work_dir);

#endif
```

只有系统调用方可在列，通过标准库实现的都不算。说到标准库，是时候实现 `stat` 了：

**代码 27-32 实现 `stat`（lib/stat.c）**
```c
#include "unistd.h"

int stat(const char *filename, struct stat *st)
{
    int fd = open((char *) filename, O_RDWR);
    if (fd == -1) return -1;
    int ret = fstat(fd, st);
    close(fd);
    return ret;
}
```

有了 `fstat` 打底，剩下的都不用管，打开完了把 fd 给过去就行了。这就把上面写的所有程序都暴露给用户了。

至此，一锤定音。

尘埃，已然落定。

给 Makefile 的 `LIBC_OBJECTS` 那行加上 `out/stat.o`，把 `stat` 链接进标准库。终于，可以开始写用户程序了。

`cd` 是集成在 shell 里的内部命令，正好 shell 还有别的要改，先把 shell 改了。还记得前面提到的提示符里显示工作目录路径吗？其实在第 16 节写 shell 的时候就已经把它留出来了，那时候只是有个 `/`，现在可真要拿它显示工作目录了。

修改输出提示符部分如下：

**代码 27-33 提示符输出工作目录路径（apps/shell.c）**
```c
static char *cwd_cache = NULL;
 
static void print_prompt() // 输出提示符
{
    printf("[TUTO@localhost %s] $ ", cwd_cache); // 这一部分大家随便改，你甚至可以改成>>>
}
```

新加了一个 `cwd_cache` 变量，它就是用来存当前工作目录的。一直 `getcwd` 也不太好，还是缓存着，到 `cd` 的时候再改。

然后来添加一个内部命令 `cd`，改变 shell 的当前工作目录：

**代码 27-34 更改 shell 的当前工作目录（apps/shell.c）**
```c
void cmd_cd(int argc, char **argv)
{
    if (chdir(argv[1]) == -1) printf("cd: invalid path\n");
    else {
        free(cwd_cache);
        cwd_cache = getcwd(NULL, 0);
    }
}
```

第一行在 `chdir` 的同时判断返回值，算是一个常见的技巧。接下来如果 `chdir` 成功，那么就更新 `cwd_cache`。由于 `getcwd` 用 `malloc` 分配内存，这里可以安全地用 `free` 释放。算上函数的包边，也就八行就把 cd 写完了。添加一个内部命令的流程希望大家都还会，不会的话赶紧回第 16 节复习去。

最后，`cwd_cache` 需要一个初始值，在进入无限循环前更新一下：`cwd_cache = getcwd(NULL, 0);` 即可。

工作目录的变化目前没有反映到新的任务上来，因为 `create_process` 的参数还是默认的 `/`，把它们都改成 `cwd_cache`，对 shell 的修改就此结束。

剩下的几个程序，从易到难排序的话，应该是 `pwd`、`rm`、`mkdir`、`ls`。先从简单的开始。

`pwd`，是 **p**rint **w**orking **d**irectory 的缩写，可不是什么 **p**ass**w**or**d**。它是用来输出工作目录的。

那这程序可太简单了，请看 VCR：

**代码 27-35 输出工作目录的程序 `pwd`（apps/pwd.c）**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

int main()
{
    char *cwd = getcwd(NULL, 0);
    puts(cwd);
    free(cwd);
    return 0;
}
```

开头叠了一堆甲没绷住。主体部分就是下面那几行，我都懒得说了。

接下来的三个程序，难度断崖式增长，一个比一个难，一个比一个细节多，一个比一个更不像是一个操作系统教程里应该出现的东西。

先来看 `rm`。`rm` 本体并不难，难的是它的 `-r` 选项，它的功能是递归删除一个目录下的所有文件。说是难，其实还是细节比较多而已。看看代码吧：

**代码 27-36 删除文件的程序 `rm`（apps/rm.c）**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

int recursive = 0;

void rm_recursive(char *path)
{
    struct stat st;
    memset(&st, 0, sizeof(st));
    DIR *dir = opendir(path);
    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->name, ".") || !strcmp(ent->name, "..")) continue;
        char *new_path = malloc(strlen(ent->name) + strlen(path) + 5);
        strcpy(new_path, path);
        strcat(new_path, "/");
        strcat(new_path, ent->name);
        int status = stat(new_path, &st);
        if (status == -1) {
            printf("rm: error: file `%s` not exist\n", new_path);
            continue;
        }
        if (st.st_type == FT_DIRECTORY) {
            rm_recursive(new_path);
        } else {
            status = unlink(new_path);
            if (status == -1) {
                printf("rm: error: error deleting file `%s`", new_path);
                continue;
            }
        }
        free(new_path);
    }
    closedir(dir);
    int status = rmdir(path);
    if (status == -1) printf("rm: error: cannot delete directory `%s`\n", path);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("rm: error: no arguments\n");
        return 1;
    }
    recursive = !strcmp(argv[1], "-r");
    int status;
    struct stat st;
    for (int i = 1; i < argc; i++) {
        if (i == 1 && !strcmp(argv[1], "-r")) continue;
        status = stat(argv[i], &st);
        if (status == -1) {
            printf("rm: error: path `%s` does not exist\n", argv[i]);
            continue;
        }
        if (st.st_type == FT_REGULAR) {
            status = unlink(argv[i]);
            if (status == -1) {
                printf("rm: error: unknown error when deleting `%s`\n", argv[i]);
                continue;
            }
        } else if (!recursive && st.st_type == FT_DIRECTORY) {
            printf("rm: error: path `%s` is a directory; use `-r` flag to remove it\n", argv[i]);
            continue;
        } else {
            rm_recursive(argv[i]);
        }
    }
    return 0;
}
```

一共 70 多行，全是应用知识，没有底层知识，理论上有过一定 Linux 开发经验的都能读懂（确信）。

快速跳过开头咏唱，第一个函数 `rm_recursive` 顾名思义，就是用来递归删除一个路径对应的目录里的所有文件的（包括这个目录本体）。首先定义一个 `struct stat` 结构并把它清空，然后用 `opendir` 打开，再进入 `readdir` 读取循环。跳过一开头的 `.` 和 `..` 目录项，然后把现在的 `path` 和读到目录项的名字 `ent->name` 连接成新的路径 `new_path`。然后，调用 `stat` 获取它的信息，返回 -1 代表文件不存在，如果文件类型是目录那就进入这一个路径进行递归，如果文件类型是普通文件那就调用 `unlink` 删除即可。

下面主程序首先判断 `argc` 是否小于 2，小于则说明参数有问题不进行后续操作，大于等于 2 则至少有一个参数，可以用 `argv[1]` 安全读取。这里判断 `argv[1]` 是否与 `-r` 相等是为了确认是否需要递归。然后开始遍历参数，跳过第一个 `-r`，对于接下来的每一个参数都执行 `stat`，是文件则直接 `unlink`，是目录则先判断能不能递归，如果能递归就调用 `rm_recursive`，否则报错无法删除目录。

下一个是 `mkdir`，用来创建目录，同样它的难度也不在于此而是在它的 `-p` 选项，意思是如果中间缺层就逐层创建。