

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

出于实现简单考虑，我们限制一个目录文件最多占 512 字节，也就是刚好一个簇（实际上 `ftcopy` 就是这么实现的）；这样，一个目录下最多只能有 14 个文件（16 个减去两个特殊的），虽说限制好像确实是有点大，但是实现起来细节就比较少了，大家理解理解吧。

总之，一口吃不成个胖子，我们把目录实现一共分为三个阶段，让我们从第一阶段开始。

### 第一阶段：跳出根目录的限制，访问任意路径下的文件

这一个阶段总体上的流程，无非就是把“找文件”这个过程对于路径的每一层逐层进行，最后找到实际上文件的过程而已。那么最为紧要的步骤，应该是首先把一个路径当中的每一层分离出来。为了方便讲述，接下来我们把这个分离的过程称为**路径解析**。

不过，在分离的同时，我们希望能够直接在这一个层面上去掉 `.`、`..` 和多余的路径分隔符 `/`，这样或许能加快一点点的速度。这样一来，问题好像就变复杂了。

没关系，有开源抄就是好，既然 myfattools 支持目录，直接拿来就是，稍作修改即可。

**代码 26-1 实现路径解析（fs/fat16.c、include/file.h）**
```c
// 路径解析
void path_parse(char *path, path_stack_t *path_stack)
{
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

接下来就是让现有的操作适配目录，用到文件名的一共有 `create`、`open` 和 `delete` 三种操作，但 `write` 要更新文件属性，而这一个过程需要知道父目录，所以实际上有四个操作都需要修改。

### 第二阶段：实现目录操作

### 第三阶段：让任务知道目录，实现相对路径