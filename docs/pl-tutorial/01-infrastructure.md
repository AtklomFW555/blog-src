俗话说，工欲善其事，必先利其器。本节的主要内容，是为尚未诞生的编程语言提前写一些数据结构服务的。

也就是说，这里基本上就是一节数据结构复习课了（bushi）

或许有人会问了：“既然有 STL 这么‘好用’的东西，为什么不直接用，反而要再造轮子呢？”究其原因，还是因为不是所有的操作系统都有 STL。“可是主流操作系统都有 STL 啊？”看看隔壁，敬爱的教程里的那个自制操作系统，可就是没有 STL 的典型样例。

不过，虽然那个操作系统没有 STL，一些基本的 C 标准库函数还是有的。??至于好不好用，可以保持怀疑态度。??为了在那种最垃圾（？）的操作系统上也能够使用这个编程语言，一些必要的牺牲（指花费远超过做编程语言的时间学习数据结构）还是非常有必要的（确信）。

先简单思考一下，在写的过程中可能会用到什么数据结构呢？显然，首先动态数组 `vector` 就是非常必要的，似乎不管是在什么地方，都会有它的身影；其次，显然变量和它的值之间形成的关系是典型的 key 不重复的 key-value pair，因此我们还应该引入 `map` 后面存变量用（至于是红黑树还是 HashMap，我们暂时先不管它）。

再然后是读写文件，`fopen` 什么的直接暴露有点丑陋，最好要写一些小小的封装。内存泄漏的话，也会比较麻烦，因此分配器也要重新实现一下。

综上所述，目前需要实现的基础设施，也就只有一个动态数组、一个字符串、一个 `map` 和一个文件读写。后续如果有了更多需要添加的东西，那后续再说。

从易到难，先从动态数组开始。

首先，来定义一套基本的头文件体系。我们的编程语言显然不可能只写在一个文件里，所有的文件都要用到一些共有的函数（比如 libc），这时候就需要使用共同的这个头文件。

**代码 1-1 共有头文件（common.h）**
```cpp
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#endif
```

目前只导入了经典的这几家，然后写了两个肯定有用的最大最小值。后面也许会扩充，也许不会。

动态数组与数组相比，唯一的区别在于它的数组容量会动态伸缩。那么在实现动态伸缩之前，我们先来实现一个不会动态伸缩的数组：

**代码 1-2 数组（C++版）（dyn_array.h）**
```cpp
#ifndef _DYNARRAY_H_
#define _DYNARRAY_H_

namespace siberia::base {
    template <typename T>
    class DynamicArray {
    private:
        T *arr; // 数组缓冲区
        int len, capacity_; // len：数组长度，capacity_：实际容量

        void init_arr(int new_len) { // 以new_len为长度和容量，初始化数组
            if (new_len == 0) capacity_ = 1; // 如果不提供长度，默认容量是1
            else capacity_ = new_len; // 否则给多少就是多少
            arr = (T *) malloc(capacity_ * sizeof(T)); // malloc单位是字节，因此要搭配 sizeof(T) 分配 capacity_ 个 T 的空间
            memset(arr, 0, capacity_ * sizeof(T)); // 置零
            len = new_len; // 设置长度为给定长度
        }
    public:
        DynamicArray(int len = 0) { // 这样兼具默认构造的功能
            init_arr(len); // 以给定长度构造动态数组
        }
        DynamicArray(T *buf, int len) : len(len), capacity_(len) { // 以给定数组构造动态数组
            init_arr(len); // 先按长度给
            for (int i = 0; i < len; i++) arr[i] = buf[i]; // 复制过去
        }

        ~DynamicArray() { free(arr); } // 销毁数组，这个好说，直接释放就行

        bool empty() const { return !len; } // 动态数组长度为0就是空的

        void clear() { // 清空数组
            free(arr); // 由于不知道T类型怎样才算空，所以先释放
            init_arr(0); // 然后重新创建一个就好了
        }

        void reverse() { // 反转数组
            for (int i = 0; i < len / 2; i++) { // 只需要判断前一半就好了
                // 以下使用三变量交换把前一半与后一半对应元素交换
                T temp = arr[i];
                arr[i] = arr[len - 1 - i];
                arr[len - 1 - i] = temp;
            }
        }

        // 重载[]运算符
        T &operator[](int index) {
            return arr[index]; // 返回arr[index]的引用，这样arr[i] = T();也可以实现了
        }

        const T &operator[](int index) const {
            return arr[index]; // 返回 const T & 只是出于性能上的考虑，这样不支持 arr[i] = T()
        }

        int size() const { return len; } // size() 对应实际大小，应该返回的是 len
        int capacity() const { return capacity_; } // capacity() 对应容量，应该返回的是 capacity_
        T *buffer() { return arr; } // buffer() 返回缓冲区，一般用不到这玩意，由于可能对arr修改，不对*this声明const 
    };
}

#endif
```

这时代码已经很复杂了，但这样就足够了吗？并不。这其实是 C++ 教材上介绍重载复制构造函数时老生常谈的问题。

想象一下把这个数组传递给一个函数时会发生什么：默认的复制构造函数并不会重新复制一块缓冲区，当函数返回时，将对传进来的参数调用析构，于是缓冲区就消失了。

因此，必须重载复制构造函数与 `operator=`，在复制时建立一块新的缓冲区。

**代码 1-3 数组（C++版）（复制时不会出事版）（dyn_array.h）**
```cpp
        // DynamicArray(T *buf, int len)
        DynamicArray(const DynamicArray<T> &other) {
            init_arr(other.len); // 用对面的长度初始化自己的长度
            for (int i = 0; i < len; i++) arr[i] = other.arr[i]; // 然后复制；由于不知道是不是 POD，不能使用 memcpy
        }

        DynamicArray &operator=(const DynamicArray<T> &other) {
            free(arr); // 先把自己放掉，free(NULL) 无行为所以没问题
            init_arr(other.len); // 用对面的长度初始化自己的长度
            for (int i = 0; i < len; i++) arr[i] = other.arr[i]; // 然后复制；由于不知道是不是 POD，不能使用 memcpy
            return *this; // 返回自己用于连续赋值
        }
        // ~DynamicArray()
```

这样就行了吗？也不够。C++11 新添加的狗市右值引用，让写一个带指针的类时的复杂度更上层楼。

右值引用对应所谓的移动语义，或者说是一种所有权的变化，当使用移动构造函数和移动赋值的时候，相当于一种默认，传进来那个东西这辈子大概算是完了，因此也就不用新建一个缓冲区了，直接用对面的就行。

**代码 1-4 数组（C++版）（复制时不会出事版）（适配 C++11 及以上）（dyn_array.h）**
```cpp
        // DynamicArray(const DynamicArray<T> &other)
        DynamicArray(DynamicArray<T> &&other) {
            // 移动构造？对面要废了，直接全抢过来
            arr = other.arr;
            len = other.len;
            capacity_ = other.capacity_;
            other.arr = NULL; // 这一步是必要的，否则待会那个快废的东西析构的时候，缓冲区还是又被释放了
        }
        // DynamicArray &operator=(const DynamicArray<T> &other)
        DynamicArray &operator=(DynamicArray<T> &&other) {
            free(arr); // 先放掉自己的
            arr = other.arr; // 对面要废了，把属性全抢过来
            len = other.len;
            capacity_ = other.capacity_;
            other.arr = NULL; // 这一步是必要的，否则待会那个快废的东西析构的时候，缓冲区还是又被释放了
            return *this; // 返回自己用于连续赋值
        }
        // ~DynamicArray()
```

到此为止，终于是够了。一个确实能用但是用起来很不舒服的包装后的数组，就这样包装完成了。接下来，让它的容量动态起来。

调整容量一共分两种情况，一种是快要撑撑爆了需要往上调，一种是快要删没了需要往下调。撑撑爆了往上走原因是很显然的，那么为什么快要删没的时候还要往下调呢？这个目的是省空间，不然占着那么多内存不是白占了吗。

最终，调整容量的函数 `adjust_capacity` 确定如下。

**代码 1-5 调整容量（dyn_array.h）**
```cpp
        // int len, capacity_;
        void adjust_capacity(int new_len) { // 根据新长度 new_len 调整容量
            if (new_len < capacity_ / 2) { // 如果小于实际容量一半
                T *new_arr = (T *) malloc(capacity_ / 2 * sizeof(T)); // 那就缩到一半
                memset(new_arr, 0, capacity_ / 2 * sizeof(T));
                for (int i = 0; i < new_len; i++) new_arr[i] = arr[i]; // 多的部分反正估计是没用的，直接不用要了
                free(arr); // 将原本的缓冲区释放
                arr = new_arr; // 换成新的
                capacity_ /= 2; // 实际容量减半
            } else if (new_len >= capacity_) { // 或者大于等于实际容量，要撑撑爆了
                T *new_arr = (T *) malloc(capacity_ * 2 * sizeof(T)); // 直接增大一倍
                memset(new_arr, 0, capacity_ * 2 * sizeof(T));
                for (int i = 0; i < len; i++) new_arr[i] = arr[i]; // 将原数组内容复制到新缓冲区
                free(arr); // 释放原缓冲区
                arr = new_arr; // 换成新的
                capacity_ *= 2; // 实际容量加倍
            } // 否则不用管
        }
        // void init_arr(int len)
```

这样把调整容量的部分就完全写完了。这一部分代码并不复杂，具体的细节放在了注释里。

仿照 Python 的 `list`，接下来来添加 `append`、`extend`、`insert`、`remove`、`operator+` 和 `operator+=`。

首先是 `append`，代码相当直接。

**代码 1-6 在末尾添加（dyn_array.h）**
```cpp
        // ~DynamicArray()
        void append(const T &value) {
            adjust_capacity(len + 1);
            arr[len++] = value;
        }
        // bool empty() const
```

首先把 `len` 加一调整容量，然后直接在 `len` 处写入数据，最后给 `len` 加一，表示多了一个元素。

接下来 `insert` 和 `remove` 是成对的，代码细节也比较类似。

**代码 1-7 在中间插入/删除（dyn_array.h）**
```cpp
        // void append(const T &value)
        void insert(int index, const T &value) {
            adjust_capacity(len + 1);
            for (int i = len - 1; i >= index; i++) arr[i + 1] = arr[i];
            arr[index] = value;
            len++;
        }

        void remove(int index) {
            for (int i = index; i < len - 1; i++) arr[i] = arr[i + 1];
            adjust_capacity(len - 1);
            len--;
        }
        // bool empty() const
```

插入的开头还是先调整容量。下面的循环是给 `index` 腾地方，从 `index` 往后，所有元素右移一格。注意，这个右移是从末尾开始，逐次向左，把当前格用左边格替换。如果循环反了，那么所有的数据都会变成 `arr[index + 1]`。不过这个也不需要特别记忆，用反了调回来就是了。空出来地方以后，就可以把这个位置写上对应的数据了。同样地，最后要给 `len` 加一。

删除的逻辑是基本相反的。从 `index` 一直到数组末尾左移一格，具体做法是，从 `index` 处开始循环，从左往右，把当前格用右边格替换。如果反了，所有的数据都会变成末尾处的数据。一定要在做完这个流程，这个数据消失了以后，再调整容量，不然就出事故了。最后给长度减一，表示这个函数消失了。

`extend`，在当前数组上拼接下一个数组。只需要把另一个数组的所有数据依次添加进来就好了：

**代码 1-8 拼接下一个数组（dyn_array.h）**
```cpp
        // void remove(int index)
        void extend(const DynamicArray<T> &other) {
            for (int i = 0; i < other.size(); i++) append(other[i]);
        }
        // bool empty() const
```

最后是 `operator+` 和 `operator+=`：

**代码 1-9 数组拼接、数组拼接后覆盖（dyn_array.h）**
```cpp
        // const T &operator[](int index) const
        DynamicArray<T> operator+(const DynamicArray<T> &other) {
            DynamicArray<T> res = *this;
            for (int i = 0; i < other.size(); i++) res.append(other[i]);
            return res;
        }

        DynamicArray<T> operator+=(const DynamicArray<T> &other) {
            for (int i = 0; i < other.size(); i++) append(other[i]);
            return *this;
        }
        // int size() const
```

重载加号，首先复制一个自己，然后把对方的所有元素添加进来。重载 `+=`，则相当于是做一个 `extend`，最后返回自己。之所以不直接用 `+` 后覆盖，是为了免去两次复制的时间。

至此，我们的动态数组就构建完成了。函数上下两行的注释里是变量或函数原型，代表这个函数应该写在这两处之间，以后不再声明。请各位读者自己拼接一下石块（雾）

接下来，在动态数组的基础上，实现一个基于 ASCII 的字符串，也不是一件困难的事情。以下直接给出完整代码：

**代码 1-10 只支持 ASCII 的字符串（str.h）**
```cpp
#ifndef _STR_H_
#define _STR_H_

#include "dyn_array.h"

namespace siberia::base {
    class String {
    private:
        DynamicArray<char> arr;
    public:
        String() {
            arr.append(0);
        }
        String(char buf) {
            arr.append(buf);
            arr.append(0);
        }
        String(int num) {
            do {
                arr.append(num % 10 + '0');
                num /= 10;
            } while (num);
            arr.reverse();
            arr.append(0);
        }
        String(char *buf) {
            for (; *buf; buf++) arr.append(*buf);
            arr.append(0);
        }

        String(const char *buf) {
            for (; *buf; buf++) arr.append(*buf);
            arr.append(0);
        }

        bool operator<(const String &other) const { return strcmp(arr.buffer(), other.arr.buffer()) < 0; }
        bool operator>(const String &other) const { return strcmp(arr.buffer(), other.arr.buffer()) > 0; }
        bool operator<=(const String &other) const { return strcmp(arr.buffer(), other.arr.buffer()) <= 0; }
        bool operator>=(const String &other) const { return strcmp(arr.buffer(), other.arr.buffer()) >= 0; }
        bool operator!=(const String &other) const { return strcmp(arr.buffer(), other.arr.buffer()) != 0; }
        bool operator==(const String &other) const { return strcmp(arr.buffer(), other.arr.buffer()) == 0; }

        String operator+(const String &other) {
            String res = *this;
            res.arr = arr;
            res.arr.remove(res.arr.size() - 1);
            res.arr += other.arr;
            return res;
        }

        String operator+=(const String &other) {
            arr.remove(arr.size() - 1);
            arr += other.arr;
            return *this;
        }

        char &operator[](int i) { return arr[i]; }
        char operator[](int i) const { return arr[i]; }

        char *c_str() {
            return arr.buffer();
        }

        void reverse() {
            arr.reverse();
        }

        int size() { return arr.size() - 1; }
    };
}

#endif
```

字符串与普通的动态数组，最大的不同之处在于几点：第一，字符串之间可以进行大小比较（字典序）；第二，字符串以一个字符 `0` 为结尾。只要掌握了这两点，就可以直接在 `DynamicArray<char>` 的基础上写出一个字符串类来。字符串的扩容一类的机制，都交给底层的 `DynamicArray<char>` 来处理。

在 `+` 和 `+=` 的重载中，由于残留着本体结尾自带的 `\0`，因此要先把结尾的那个东西删掉。最后一个 `size()` 要减一也同理。

到现在，已经支持了动态数组和字符串，考虑到后续文件操作时出现的意外会比较多，可以先建立一套自己的异常体系。

**代码 1-11 自建异常体系（exception.h）**
```cpp
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "common.h"
#include "dyn_array.h"
#include "str.h"

namespace siberia::base {
    class Exception {
    private:
        DynamicArray<String> warnings;
        String error;
    public:
        bool is_error = false, is_warning = false;

        void raise(String msg) {
            error = msg;
            is_error = true;
        }

        void warn(String msg) {
            warnings.append(msg);
            is_warning = true;
        }

        String get_error() { return error; }
        DynamicArray<String> get_warning() { return warnings; }
    };
};

#endif
```

非常好理解，使用了一个 String 来存所有的错误和一个动态 String 数组来存所有的警告，这是因为警告可能有很多个，但只要有一个错误就可以停掉整套流程了。不过这样直接用不太好用，搞一堆宏包装一下：

**代码 1-12 异常处理宏定义（exception.h）**
```cpp
// #include "str.h"
// 以下宏在使用时需要声明 Exception *ex;

#define TRY
#define THROW(msg) ex->raise(msg)
#define CATCH if (ex->is_error)
#define ERR_MSG (ex->get_error())

#define WARN(msg) ex->warn(msg)
#define CATCH_WARNINGS if (ex->is_warning)
#define WARNING_MSG (ex->get_warning())

// namespace siberia::base
```

这样，以后判断是否有错误，只需要使用 `CATCH` 宏，后面接大括号处理，`ERR_MSG` 为错误消息；警告则是使用 `CATCH_WARNINGS` 和 `WARNING_MSG`。如果是要抛出错误或警告，只需要使用 `THROW` 和 `WARN` 宏。至于前面那个 `TRY`，纯粹是为了和 `CATCH` 好看才加的，使用的时候直接摆在那就行，千万不能加大括号。

有了异常处理的逻辑，就可以开始写文件读写的工具了。

**代码 1-13 文件读写工具（file.h）**
```cpp
#ifndef _FILEREADWRITER_H_
#define _FILEREADWRITER_H_

#include "common.h"
#include "exception.h"

namespace siberia::base {
class FileReadWriter {
private:
    Exception *ex;

    FILE *fp;
    int size_;
    FileReadWriter(const FileReadWriter &other) = delete;
    FileReadWriter(FileReadWriter &&other) = delete;
    FileReadWriter &operator=(const FileReadWriter &other) = delete;
    FileReadWriter &operator=(FileReadWriter &&other) = delete;

    void updateSize() {
        fseek(fp, 0, SEEK_END);
        size_ = ftell(fp);
    }
public:
    FileReadWriter(Exception *ex, const char *filename) : ex(ex) {
        fp = fopen(filename, "rb+");
        if (!fp) {
            THROW(String("file opening error: ") + filename);
        } else updateSize();
    }
    ~FileReadWriter() {
        fclose(fp);
    }
    int size() { return size_; }
};
}

#endif
```

首先声明了一个异常 `Exception *ex`，这样在类中就可以使用 `THROW` 等一系列宏了；然后是文件指针和文件大小，最后删除了所有和复制有关的东西，以后传递这个工具，只传引用。

`updateSize()` 函数用于更新文件大小，反正后面读写的时候会要设置文件读写位置，一开头这里自然就不用管了。

接下来在构造函数中根据给出的文件名打开文件。在这个语言的使用场景下，`Exception *ex` 应该始终是同一个指针，所以直接复制指针，不用新建指针复制值了。接下来打开文件，如果打开出了问题，就使用 `THROW` 宏抛出异常，否则更新文件大小。析构函数中释放所有资源，此处就是文件指针 `fp`。

为了好玩（？），接下来实现的文件读写将暴露出类似数组的 API。具体而言，读写一个文件时，可以使用类似于访问数组的方法，例如用 `f[0]` 表示一个 `FileReadWriter` 对应的文件的第一个字节。

显然读写操作是不一样的，如果直接重载 `operator[]`，那么不能区分读写，比较麻烦。因此，需要实现一个单独的临时中间类型 `FilePos` 表示一个文件对应的位置，实际操作均由 `FilePos` 的结构来完成。

**代码 1-14 文件位置 `FilePos`（file.h）**
```cpp
    // FileReadWriter &operator=(FileReadWriter &&other) = delete;
    class FilePos {
    private:
        FileReadWriter *parent;
        FILE *fp;
        Exception *ex;
        int pos;
    public:
        FilePos(FileReadWriter *parent, Exception *ex, FILE *fp, int pos) : parent(parent), ex(ex), fp(fp), pos(pos) {}
        ~FilePos() {}
        operator uint8_t() {
            if (!fp) return -1;
            if (pos == parent->size()) {
                THROW("The position at the end of the file is only writable");
                return -1;
            }
            fseek(fp, pos, SEEK_SET);
            char buf = 0;
            fread(&buf, 1, 1, fp);
            return buf;
        }
        uint8_t operator=(const uint8_t &byte) {
            if (!fp) return byte;
            fseek(fp, pos, SEEK_SET);
            char buf = byte;
            fwrite(&buf, 1, 1, fp);
            fflush(fp);
            parent->updateSize();
            return byte;
        }
    };
    // void updateSize()
    // ...
    // ~FileReadWriter()
    FilePos operator[](int pos) {
        if (pos > size_ || pos < 0) {
            String err_msg = "invalid position: ";
            err_msg += pos;
            err_msg += ", where size is ";
            err_msg += size_;
            THROW(err_msg);
            return FileReadWriter::FilePos(this, ex, NULL, -1);
        }
        return FileReadWriter::FilePos(this, ex, fp, pos);
    }
    // int size()
```

重载的 `operator[]` 返回一个只读的 `FilePos` 对象，这样一来，以下代码：

```cpp
FileReadWriter rw("name.txt");
rw[0] = '1';
putchar(rw[1]);
```

第二行的 `rw[0]` 会先返回 `FilePos`，而后面的 `= '1'` 则相当于调用 `FilePos` 的 `operator=`，从而走写入分支；

第三行的 `rw[1]` 也会先返回 `FilePos`，由于在 `putchar` 里，C++ 编译器会寻找到 `uint8_t` 的隐式转换，于是进入 `FilePos` 的 `operator uint8_t()`，从而走读取分支。

这样一来，就巧妙地在把读写分流的同时，使用基本相同的 API。忘了是在什么地方看到的这个技巧，总之不是我的原创，太天才了！

只要理解了上面的说明，立刻就能明白 `FilePos` 这个类是在干什么。虽然这里似乎使用友元会更好一些，但由于 ??作者不会?? 这样写更清晰，所以最终还是把用到的东西都传进来了。实际的读写操作，都是分别在上面说明中提到的转换函数中进行的。

这里还有一个比较有意思的点，那就是 `FilePos` 实际上允许使用“文件末尾”这个不存在的位置，但是只能写不能读。为什么呢？其实这是用来在文件后追加东西的。例如，想要追加一个字符串 `s`，只需要这么写就行了：

```cpp
FileReadWriter rw("name.txt");
for (; *s; s++) rw[rw.size()] = *s;
```

因此，不管是什么场合，只要走了写入分支，都会调用一遍 `updateSize()`。其实可以加个判断的，只是后面应该也用不到写入，所以作者就咕掉了）

前面提到的基础设施只剩下一个 `map` 还没有实现了。`map` 和 `set` 其实非常相像，本质上都是一堆元素不重不漏地出现。因此接下来的讨论主要集中在实现 `set` 上。

显然，实现 `set` 的第一个方法就是直接使用动态数组，但是这样增删改查都是 O(n)，速度极慢。链表也有一样的问题，增删改查都是 O(n) 的，我们需要更高级的数据结构。

按照目前市场行情（？），这方面最专业的是红黑树。不过，首先还是应该要介绍一下红黑树到底是什么东西，然后再看看红黑树到底有什么优势。

红黑树是一种特殊的二叉搜索树，以下先介绍二叉搜索树的实现。

二叉搜索树是一种特殊的二叉树，对于其上每一个节点，它都带有一个关键字（key），或者说一个值。对于任意一个节点，它的左子树所带的所有值（如果有）都应该小于它本身所带的值，它的右子树所带的所有值（如果有）都应该大于它本身所带的值。

以上这一条就是二叉搜索树的根本性质，下面开始实现。

首先，定义二叉搜索树的节点如下：

**代码 1-15 二叉搜索树节点（tree_map.h）**
```cpp
#ifndef _TREEMAP_H_
#define _TREEMAP_H_

template <typename T>
struct TreeNode {
    T key;
    TreeNode *fa;
    TreeNode *left;
    TreeNode *right;

    TreeNode(T key) : key(key), fa(NULL), left(NULL), right(NULL) {}
};

template <typename T>
struct Tree {
    TreeNode<T> *root = NULL;
};

#endif
```

`struct Tree` 是后续操作的接口，以后的操作一般不传 `TreeNode`。

接下来理论上要实现增删改查，但接下来实现的是 `set`，查和改是基于 `set` 实现 `map` 时做的单独封装，这里只实现增删即可。

新增一个节点非常简单：从根节点开始，逐个比较要新增的值与当前节点的值，依二叉搜索树的性质，要新增的值如果小于当前节点值，当前节点就转到左子节点；如果大于，就转到右子节点；否则，说明已经存在这个节点，什么也不做。

**代码 1-16 向二叉搜索树中插入节点（tree_map.h）**
```cpp
    // TreeNode<T> *root = NULL;

    void insert(T key)
    {
        TreeNode<T> *node = root, *pos = NULL; // pos 为待插入节点的父亲节点
        while (node) { // 只要 node 节点还存在，说明还没有到达待插入节点本该在的位置
            pos = node;
            if (key < node->key) node = node->left;
            else if (key > node->key) node = node->right;
            else {
                return;
            }
        }
        // 走到这里，node 为该节点本该存在的位置，pos 为该节点要插入的位置
        TreeNode<T> *new_node = new TreeNode<T>(key); // 这时再新建节点
        new_node->fa = pos; // 设置父节点
        if (!pos) root = new_node; // 如果没有父亲节点，说明连根都没有，设置成根节点
        else if (key < pos->key) pos->left = new_node; // 否则按照位置，把新节点放进去，并设置好父子关系
        else pos->right = new_node;
    }

// };
```

删除则要麻烦许多，删除一个节点，首先要找到这个节点，然后要依照它有没有孩子来讨论。

**代码 1-17 从二叉搜索树中删除一个节点（1）找到节点（tree_map.h）**
```cpp
    // void insert(T key)
    bool remove(T key)
    {
        TreeNode<T> *node = root, *p = NULL;
        while (node) {
            if (key != node->key) p = node; // 在没找到之前更新为上一次的node，p就是node的父亲
            if (key < node->key) node = node->left;
            else if (key > node->key) node = node->right;
            else break;
        }
        if (!node) return false;
        // node就是要找的节点，以下分情况讨论
        return true;
    }
// };
```

第一种情况，待删除节点没有孩子，那非常简单，直接删除，并且把它在父亲中对应的位置设置成 NULL。

**代码 1-18 从二叉搜索树中删除一个节点（2）无孩子（tree_map.h）**
```cpp
    // node就是要找的节点，以下分情况讨论
    // p1. 无孩子
    if (!node->left && !node->right) {
        // 直接删除。
        if (p) { // 如果删的是根节点自然不用考虑这个
            if (p->left == node) p->left = NULL;
            else if (p->right == node) p->right = NULL;
        } else { // 否则删除的是根节点
            root = NULL; // 为避免遍历时出现错误，设置root为NULL
        }
        delete node;
    }
    // return true;
```

第二种情况，只有一个孩子。这时把它孩子的值、左子树和右子树抢夺过来，然后把它的孩子删除即可。为什么可以这样做呢？

以下不妨设这要删除的节点是 N，它是它的父亲 P 的左孩子，而 N 的唯一一个孩子是 C。那么，由于要删除的是 N，由二叉搜索树的性质，无论 C 是 N 的左孩子还是右孩子，C 所带的值都应该比 P 要小。因此，用 C 代替 N，并不会破坏二叉搜索树的性质；反之同理。

**代码 1-19 从二叉搜索树中删除一个节点（3）只有一个孩子（tree_map.h）**
```cpp
    // p1. 无孩子
    // p2. 只有一个孩子，用孩子替代它
    else if (!node->left) {
        TreeNode<T> *right = node->right;
        node->key = right->key;
        node->left = right->left;
        node->right = right->right;
        if (node->left) node->left->fa = node;
        if (node->right) node->right->fa = node;
        delete right;
    } else if (!node->right) {
        TreeNode<T> *left = node->left;
        node->key = left->key;
        node->left = left->left;
        node->right = left->right;
        if (node->left) node->left->fa = node;
        if (node->right) node->right->fa = node;
        delete left;
    }
    // return true;
```

在抢夺孩子的子树的过程中，一并重新设置了父亲，这样无论从什么地方，都再也找不到这个孩子的踪迹了。

第三种情况，有两个孩子。先给结论：这种情况下，用它的右子树的最小值（左子树的最大值也行）替换掉它本来的值，然后删除右子树的最小值原本所在的节点。

为什么要这样做呢？首先，右子树的最小值所在节点，依照二叉搜索树的性质，它是没有左孩子的，不然它的左孩子的值比它的值更小，它的值就不是最小值了；从而删除这个节点，属于上面的第二种情况，是已经能够解决的问题。

第二，自然是因为这样做不会破坏任何二叉搜索树的性质。设这要删除的结点为 N，它右子树的最小值所在节点为 S。由于它是右子树的最小值，右子树所带的所有值都大于 S 的值；又由于它位于右子树，它的值比 N 的值要大，从而比 N 的左子树的任何一个值都要大。从而，用 S 的值替换 N 的值，不破坏任何二叉搜索树的性质。

右子树最小值虽然没有左孩子，但是可能有右孩子，在删除时，应该把它的右孩子过继给它的父亲。

那么为什么前面说左子树的最大值也行呢？留给感兴趣的读者作为习题。仿照上面逻辑论证一遍即可。

这段虽然原理比较难，但代码并没有增加。

**代码 1-20 从二叉搜索树中删除一个节点（4）有两个孩子（tree_map.h）**
```cpp
    // p2. 只有一个孩子，用孩子替代它
    // p3. 有两个孩子，用右子树的最小值替代它
    else {
        TreeNode<T> *succ = node->right, *succ_p = node;
        while (succ->left) succ_p = succ, succ = succ->left;
        node->key = succ->key;
        succ_p->left = succ->right;
        if (succ->right) succ->right->fa = succ_p;
        delete succ;
    }
    // return true;
```

至此，就已经实现了二叉搜索树，并没有什么难度。但是极端情况下，二叉搜索树会退化成一条链变成链表，于是平衡树应运而生。

平衡树所谓的“平衡”，自然是与不平衡相对的。不平衡的极端，就是上面说过的变成链表。那么什么树是比较平衡的呢？满二叉树、完全二叉树，应该是最为平衡的树了。除此以外，什么是平衡的，什么是不平衡的，没有明确的界定。一般是看左子树与右子树的高度差，以及节点个数等。

前面提到的红黑树，就是一种平衡树。除此以外，平衡树还有 treap、splay、AVL 等等多种。

treap 的核心思想是，给一棵树上的每一个节点两个值，第一个值是用户自己要存的值，称它为 key；第二个值是随机分配的，我们称它为 rank。treap 是 tree 和 heap 的结合，它也是一种二叉树，但是 key 满足二叉搜索树的性质，而 rank 满足堆的性质。由于不是所有 OS 都有随机，treap 不予考虑。

splay 的核心思想是，每次访问了一个节点（这里的访问指查找与插入），都要把刚刚被访问的节点变成根节点。由于 ??我不能理解为什么要这么做?? 这样做太过复杂，splay 不予考虑。

AVL 的核心思想是，既然你不平衡来源于高度差，我就限制你的高度差不能超过 2，一旦越界，就意味着树不再平衡，需要进行调整。在插入节点和删除节点时，有可能会造成这样的变化。从而，在发生变化以后，要一路从插入/删除节点的父节点一路调整到根，这就导致它要进行大量调整，在插入/删除较少而查询较多时，使用 AVL 更快。

红黑树，业界良心，零差评，从操作系统到编程语言标准库，从操作系统底层数据结构到用户每日使用的基本结构，都有红黑树的身影。问题是平衡维护情况多，讨论繁，全是体力活。

简单介绍一下红黑树，它在二叉搜索树的基础上，额外给所有节点染上红黑两色，并且把所有节点中那个不存在的子节点看做一个真实存在的节点 NIL。

在染色时，需要遵循以下规则：

1) 根节点和 NIL 节点都是黑色的；

2) 红节点的子节点都是黑色的；

3) 染色后，从根节点到 NIL 节点的所有路径中，黑色节点的数量相同。

从 2、3 两条规则稍微尝试一下就可以发现，一个退化成链表的二叉搜索树是无法在规则要求内染色的，必须经过调整。而调整的过程就是一个把树变平衡的过程，至于为什么，我不会证，不管它，知道是这样就行。

更多细节可以参见 OI Wiki：[红黑树](https://oi-wiki.org/ds/rbtree/)

在总结了以上几种常见的平衡树后，经过权衡，最终选择 AVL 而不是红黑树来实现 map，因为红黑树情况太多 ??而我太懒??，实现起来太麻烦了。

AVL 中，维护平衡的关键在于树节点的高度。那么什么是一个节点的高度呢？定义：叶子节点的高度为 1；否则，取左右子树中较高者加一。

接下来就可以定义高度以及更新高度的代码了：

**代码 1-21 添加高度成员并维护高度（tree_map.h）**
```cpp
struct TreeNode {
    // TreeNode *right;
    int height;

    TreeNode(T key) : key(key), fa(NULL), left(NULL), right(NULL), height(1) {}
    
    void update_height() {
        if (!left && !right) height = 1;
        else if (!left) height = right->height + 1;
        else if (!right) height = left->height + 1;
        else height = max(left->height, right->height) + 1;
    }
};
```

由于涉及到 NULL，这一段代码比较恶心，或许这个函数不应该做成成员函数。

对于每一个节点，还定义它有一个平衡因子，也就是前面提到的高度差，它的值是：左子树高度减右子树高度。

由于要考虑到节点是 NULL 的可能性，这一部分的代码也相对复杂。

**代码 1-22 计算平衡因子（tree_map.h）**
```cpp
struct TreeNode {
    // void update_height()
    int balance_factor() {
        if (!left && !right) return 0;
        else if (!left) return -right->height;
        else if (!right) return left->height;
        else return left->height - right->height;
    }
};
```

接下来就可以正式进入调整平衡的部分了。

前面光说了半天调整调整，怎么做到既改变节点的高度差，又能够维护二叉搜索树的性质呢？还真有这样的操作，我们称之为**旋转**。

旋转分为左旋和右旋：

```plain
    R                      L
   / \                    / \
  L   G    <----->       S   R
 / \                        / \
S   M                      M   G
```

如图所示，对于这样一棵子树，从左到右的变化被称为右旋，从右到左的变化被称为左旋，操作的对象都是这棵子树的根节点。依二叉搜索树性质，S 为小于 L 的子树，M 为大于 L 小于 R 的子树，G 为大于 R 的子树，读者由此不难看出，这样的变化并不破坏这棵子树内部的二叉搜索树性质。

在旋转过程中，如果 S 很低而 G 很高，这样进行右旋有助于降低 G 的高度，抬升 S 的高度，使树更加平衡；反之亦然。旋转是二叉搜索树为了维护平衡进行自我调整，最重要且唯一的手段。

只需要对着上面的图示一点点来，就可以写出旋转的代码，并不难做。

**代码 1-23 左旋与右旋（tree_map.h）**
```cpp
    // TreeNode<T> *root = NULL;

    void left_rotate(TreeNode<T> *l)
    {
        TreeNode<T> *r = l->right, *p = l->fa;
        l->right = r->left;
        r->left = l;

        r->fa = p;
        if (p) {
            if (p->left == l) p->left = r;
            else p->right = r;
        } else root = r;

        l->fa = r;
        if (l->right) l->right->fa = l;

        l->updateHeight();
        r->updateHeight();
    }

    void right_rotate(TreeNode<T> *r)
    {
        TreeNode<T> *l = r->left, *p = r->fa;
        r->left = l->right;
        l->right = r;

        l->fa = p;
        r->fa = l;
        if (p) {
            if (p->left == r) p->left = l;
            else p->right = l;
        } else {
            root = l;
        }
        if (r->left) r->left->fa = r;

        l->updateHeight();
        r->updateHeight();
    }

    // void insert()
```

看似代码很多，真正重要的只有前三行，剩下几行的工作都只是重新设置父节点、有必要时重新设置根节点以及更新高度。只要脑子里有图，照着图一点一点写，一定可以把代码写出来。

那么该怎么维护平衡呢？接下来的讨论在以下的子树中进行（以下省略不发生变化的子树）：

```plain
    D
   / \
  B   E
 / \
A   C
```

由于维护路径是从底维护到根，所以此处假设 A、B、C、E 的平衡维护均已完成。显然此图中 B 应该比 E 高，若 E 比 B 高，只需要把以下结论中的左右子树对调，左右旋对调，高度大小关系也对调就可以了。

由于 B 比 E 高且需要维护平衡，说明 B 的高度比 E 的高度大 2（否则早该维护平衡了轮不到这时候）。设 E 的高度为 x，则 B 的高度为 x + 2。

若 A 的高度大于等于 C，那么 A 的高度应为 x + 1（由高度定义显然），而 C 的高度为 x 或 x + 1 其中之一。此时右旋节点 D，子树变成：

```plain
    D            B
   / \          / \
  B   E  --->  A   D
 / \              / \
A   C            C   E
```

由于没有动 A、C、E 的子树，A、C、E 的高度均不变。而 D 的高度为 C 的高度与 E 的高度中较大者加一，也就是 x + 2，与 A 的高度差缩小到 1，从而完成了这棵子树平衡的维护。

若 A 的高度小于 C，则 A 的高度应为 x，C 的高度应为 x + 1。此时先左旋节点 B：

```plain
    D               D
   / \             / \
  B   E     --->  C   E
 / \             /
A   C           B
               /
              A 
```

那么这时，由于没有动 A 和 E 的子树，两者高度不变，B 的高度变为 x + 1，C 的高度变为 x + 2，而 E 的高度仍然为 x。虽然此时树并没有平衡，但是考虑到 C 未经变化的右子树此时高度仍然为 x，这就相当于转变成了上一种情况，只需再次右旋节点 D 即可。

把上面得到的结论再镜像，就得到了在单个节点上维护平衡的函数 `balance_fixup`：

**代码 1-24 维护平衡（tree_map.h）**
```cpp
    // void right_rotate(TreeNode<T> *r)
    void balance_fixup(TreeNode<T> *node)
    {
        if (!node) return;
        node->update_height();

        int factor = node->balance_factor();

        if (factor > 1) {
            if (node->left && node->left->balance_factor() < 0) left_rotate(node->left);
            right_rotate(node);
        } else if (factor < -1) {
            if (node->right && node->right->balance_factor() > 0) right_rotate(node->right);
            left_rotate(node);
        }
    }
    // void insert(T key)
```

注意，沿着节点维护到根，是沿着节点还没有动之前的路径，而不是动之后的路径，因此在写维护到根的函数时，要提前把父节点存起来，而不是在维护平衡完后再进行访问。

**代码 1-25 维护平衡到根（tree_map.h）**
```cpp
    // void balance_fixup(TreeNode<T> *node)
    void balance_fixup_to_root(TreeNode<T> *node)
    {
        if (!node) return;
        TreeNode<T> *p = node->fa;
        while (p) {
            balance_fixup(node);
            node = p;
            p = node->fa;
        }
    }
    // void insert(T key)
```

至此，维护平衡的操作就写完了，只需要在插入和删除时进行调用即可：

**代码 1-26 执行维护操作（tree_map.h）**
```cpp
    void insert(T key)
    {
        // else pos->right = new_node;
        if (pos) balance_fixup_to_root(pos);
    }

    bool remove(T key)
    {
        // p2. 只有一个孩子，用孩子替代它
        else if (!node->left) {
            // if (node->right) node->right->fa = node;
            balance_fixup_to_root(node);
            // delete right;
        } else if (!node->right) {
            // if (node->right) node->right->fa = node;
            balance_fixup_to_root(node);
            // delete left;
        }
        // p3. 有两个孩子，用后继替代它
        else {
            //if (succ->right) succ->right->fa = succ_p;
            balance_fixup_to_root(succ_p);
            //delete succ;
        }
    }
```

删除叶子节点的情况本来也应该维护一下平衡，转念一想好像没必要，所以就不这么干了。??主要是加上了会卡死，至于为什么会卡死，留给读者思考（逃）??

至此，AVL 树也写完了，代码并没有增加几行（应该？）。使用非常 naive 的基准测试方法（rand 出 1e7 个数字然后往 AVL 和 map 里做操作比较时间）发现，手写的 AVL 与 map 性能基本相当，插入 AVL 略快，查找参差不齐，删除 AVL 略慢。

最后一步，是把 AVL 封装成一个 map 一样的东西。