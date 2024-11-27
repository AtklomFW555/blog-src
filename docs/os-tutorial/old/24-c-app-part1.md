欢迎回来。我们本节的任务十分明确：支持 `malloc`，同时为应用程序传参。

不过，在一开始我们先来点“轻松”的。或许在第 16 节和第 22 节的时候，有的读者会有这样的疑问：

> 你 shell 明明集成在内核当中，为什么还要费事去系统调用呢？

所以我们今天的第一个 surprise，就是把 shell 从内核当中给剥离出去，做成一个单独的 app。没有什么原因，只是因为这样泰裤辣！（逃

上一节已经初步支持了 C 语言应用程序，而我们的 shell 一不用传参，二来在一番微操之下规避了 `malloc`，因此可以直接放在这个框架里。

首先来把现在的 `shell` 改造成一个应用程序一样的东西：

**代码 24-1 现在的 `shell`（apps/shell.c）**
```c
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_CMD_LEN 100
#define MAX_ARG_NR 30
 
static char cmd_line[MAX_CMD_LEN] = {0}; // 输入命令行的内容
static char *argv[MAX_ARG_NR] = {NULL}; // argv，字面意思
 
static void print_prompt() // 输出提示符
{
    printf("[TUTO@localhost /] $ "); // 这一部分大家随便改，你甚至可以改成>>>
}
 
static void readline(char *buf, int cnt) // 输入一行或cnt个字符
{
    char *pos = buf; // 不想变buf
    while (read(0, pos, 1) != -1 && (pos - buf) < cnt) { // 读字符成功且没到cnt个
        switch (*pos) {
            case '\n':
            case '\r': // 回车或换行，结束
                *pos = 0;
                putchar('\n'); // read不自动回显，需要手动补一个\n
                return; // 返回
            case '\b': // 退格
                if (buf[0] != '\b') { // 如果不在第一个
                    --pos; // 指向上一个位置
                    putchar('\b'); // 手动输出一个退格
                }
                break;
            default:
                putchar(*pos); // 都不是，那就直接输出刚输入进来的东西
                pos++; // 指向下一个位置
        }
    }
}

static int cmd_parse(char *cmd_str, char **argv, char token)
{
    int arg_idx = 0;
    while (arg_idx < MAX_ARG_NR) {
        argv[arg_idx] = NULL;
        arg_idx++;
    } // 开局先把上一个argv抹掉
    char *next = cmd_str; // 下一个字符
    int argc = 0; // 这就是要返回的argc了
    while (*next) { // 循环到结束为止
        if (*next != '"') {
            while (*next == token) *next++; // 多个token就只保留第一个，windows cmd就是这么处理的
            if (*next == 0) break; // 如果跳过完token之后结束了，那就直接退出
            argv[argc] = next; // 将首指针赋值过去，从这里开始就是当前参数
            while (*next && *next != token) next++; // 跳到下一个token
        } else {
            next++; // 跳过引号
            argv[argc] = next; // 这里开始就是当前参数
            while (*next && *next != '"') next++; // 跳到引号
        }
        if (*next) { // 如果这里有token字符
            *next++ = 0; // 将当前token字符设为0（结束符），next后移一个
        }
        if (argc > MAX_ARG_NR) return -1; // 参数太多，超过上限了
        argc++; // argc增一，如果最后一个字符是空格时不提前退出，argc会错误地被多加1
    }
    return argc;
}

void cmd_ver(int argc, char **argv)
{
    puts("TutorialOS Indev");
}

int try_to_run_external(char *name, int *exist)
{
    int ret = create_process(name, cmd_line, "/");
    *exist = false;
    if (ret == -1) {
        char new_name[MAX_CMD_LEN] = {0};
        strcpy(new_name, name);
        int len = strlen(name);
        new_name[len] = '.';
        new_name[len + 1] = 'b';
        new_name[len + 2] = 'i';
        new_name[len + 3] = 'n';
        new_name[len + 4] = '\0';
        ret = create_process(new_name, cmd_line, "/");
        if (ret == -1) return -1;
    }
    *exist = true;
    ret = waitpid(ret);
    return ret;
}

void cmd_execute(int argc, char **argv)
{
    if (!strcmp("ver", argv[0])) {
        cmd_ver(argc, argv);
    } else {
        int exist;
        int ret = try_to_run_external(argv[0], &exist);
        if (!exist) {
            printf("shell: `%s` is not recognized as an internal or external command or executable file.\n", argv[0]);
        } else if (ret) {
            printf("shell: app `%s` exited abnormally, retval: %d (0x%x).\n", argv[0], ret, ret);
        }
    }
}
 
void shell()
{
    puts("TutorialOS Indev (tags/Indev:WIP, Jun 26 2024, 21:09) [GCC 32bit] on baremetal"); // 看着眼熟？这一部分是从 Python 3 里模仿的
    puts("Type \"ver\" for more information.\n"); // 示例，只打算支持这一个
    while (1) { // 无限循环
        print_prompt(); // 输出提示符
        memset(cmd_line, 0, MAX_CMD_LEN);
        readline(cmd_line, MAX_CMD_LEN); // 输入一行命令
        if (cmd_line[0] == 0) continue; // 啥也没有，是换行，直接跳过
        int argc = cmd_parse(cmd_line, argv, ' '); // 解析命令，按照cmd_parse的要求传入，默认分隔符为空格
        cmd_execute(argc, argv); // 执行
    }
    puts("shell: PANIC: WHILE (TRUE) LOOP ENDS! RUNNNNNNN!!!"); // 到不了，不解释
}

int main()
{
    shell();
    return 0;
}
```

从上面的文件名就可以知道，我们已经把 `shell` 挪到了 `apps` 目录下；同时，在最后也加了一个 `int main()`，虽然说可以直接在 `shell()` 上改，但留点遗存也不是不行（？）

下面引入了一堆头文件，其中的 `unistd.h` 是上一节所造，`stdio.h` 早已有之，剩下的 `stdint.h`、 `stdbool.h` 以及 `stddef.h` 是从 `common.h` 里分离出来的产物：

**代码 24-2 三个头文件（include/stdint.h、include/stdbool.h、include/stddef.h）**
```c
#ifndef _STDINT_H_
#define _STDINT_H_

typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

#endif
```
```c
#ifndef _STDBOOL_H_
#define _STDBOOL_H_

typedef _Bool bool;
#define true 1
#define false 0

#endif
```
```c
#ifndef _STDDEF_H_
#define _STDDEF_H_

#define NULL ((void *) 0)

#endif
```

如你所见，这三个头文件基本上全都很短小，什么安全保护措施也没加，毕竟纯玩玩也用不到，到时候从什么地方copy一个就行了（bushi）。

另外，在 `stdio.h` 中把 `#include "common.h"` 替换成了 `#include "string.h"` 和 `#include "stdint.h"` 两行，在 `unistd.h` 的函数声明开始前增加了一行 `#include "stdint.h"`。这样做是为了确保这些标准库文件与操作系统文件无关 ~~其实就是闲的~~。

接下来在 `kernel_main` 启动 `shell` 的部分也要修改：

**代码 24-3 启动 shell（kernel/main.c）**
```c
void kernel_main() // kernel.asm会跳转到这里
{
    monitor_clear();
    init_gdtidt();
    init_memory();
    init_timer(100);
    init_keyboard();
    asm("sti");
    task_init();

    sys_create_process("shell.bin", "", "/");

    task_exit(0);
}
```

`task_a` 变量从头到尾没有被用到，因此就删了。下面的 `sys_create_process` 实质上开启了一个新任务执行 `shell.bin`。最后，调用 `task_exit(0)` 退出当前任务，于是操作系统就进入后台，而主要是 ring3 用户层的 `shell` 在起交互作用了。

在 `Makefile` 的 `APPS` 中加入 `out/shell.bin`，`OBJS` 中删除 `out/shell.o`，完成最后的交接。`shell` 的地位甚至因此还提升了（？）

最后当然是编译运行啦：

![](images/graph-24-1.png)
（图 24-1 效果）

执行内部命令还是应用程序都没问题，不过按理来说也算理所应当吧，到最后也没做多少修改（笑）。