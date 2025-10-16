PL 系列登神长阶（2/3）

前置芝士：[题解 P3695 - CYaRon! 语](2024-08-08-cyaron-solution.md)

本文最终代码：注册 UOJ 账号后，进入 [#783817](https://uoj.ac/submission/783817)，下方附有完整代码及在 UOJ 上的评测成绩。最终代码 43KB，无需压行。

---

书接上回。上回书说到，欲实现一个极其简单以至于能被出成 OI 题的自制编程语言，一共可以分为词法分析、语法分析、执行 AST 这三大板块。

词法分析各大题目，甚至各大语言套路都差不多，而语法分析只要给了文法就几乎也是公式，即使没给文法，难点也只是在于表达式里各种优先级、结合性那一块。虽然说执行也偏向于公式化（bushi），但执行部分细节最多，可能出的毛病也最多，因此本文将会侧重于执行部分（？）

### Quick Review

快速过一遍自制编程语言的基础知识：词法分析的过程是把字符串的字符重新组合成一个个名为 Token 的单位的过程，类似于英文中把一个句子先拆解成单词；语法分析则是根据文法把 Token 流拆解成更好理解的形式的过程，这种形式在上题和本题的语境下都是抽象语法树 AST。

一个 Token 描述的是一个小整体，比如一个变量、一个关键字、一个数字、一个运算符之类的，抽象语法树 AST 的一个节点描述的是一个程序中比较大的整体，比如 `1 + 2 + 3 * 4` 这么一个表达式，首先各个数字单独成为一个整体，然后 `1 + 2` 是一个整体，`3 * 4` 是一个整体，最后再组成 `1 + 2 + 3 * 4` 一个大的整体；`for (i = 1; i <= 10; i = i + 1) {}` 中，`i = 1`、`i <= 10`、`i = i + 1`、`{}` 是四个整体，这四个整体共同组成了上面的 `for` 循环。像上面这样把整个程序细分成各种整体，就形成了各种节点。在实践中如果发现某 AST 节点不能满足需要，则需要建立新的节点，或是修改这个节点。

文法一般使用 BNF 或是 EBNF 来描述，本题中给出的文法就是 BNF，这大大减轻了我们的工作量，不用再像 P3695 一样揣摩文法长啥样了。

最后是执行部分，执行部分说是只需要遍历 AST 即可，但是各种细节都要自己维护，稍有不慎就容易把程序整个卡死，必须理清执行一个语句时到底需要干什么，比如 `for` 和 `while` 循环要在执行完一次循环后重新计算循环条件是否还成立等等。

以上大致就是写一个极简编程语言的逻辑。下面在穿插代码的同时，也介绍一些能让编码更加轻松愉快（大嘘）的小技巧。

### X-Macros

在调试过程中，往往需要输出一个 Token 啦、输出一个 AST 啦，这就绕不开要输出 TokenType。但未来程序改的 TokenType 非常多：

```cpp
enum TokenType {
    // 单字符
    D(TT_LPAREN),
    D(TT_RPAREN),
    D(TT_LBRACE),
    D(TT_RBRACE),
    D(TT_SEMICOLON),
    D(TT_COMMA),
    D(TT_LBRACKET),
    D(TT_RBRACKET),
    D(TT_PLUS),
    D(TT_MINUS),
    D(TT_MUL),
    D(TT_DIV),
    D(TT_MOD),
    D(TT_XOR),
    D(TT_LT),
    D(TT_GT),
    D(TT_EXCLAMATION),
    D(TT_EQ),
    // 双字符
    D(TT_LE),
    D(TT_GE),
    D(TT_EE),
    D(TT_NE),
    D(TT_LAND),
    D(TT_LOR),
    D(TT_LSHIFT),
    D(TT_RSHIFT),
    // 多字符
    D(TT_IDENTIFIER),
    D(TT_KEYWORD),
    D(TT_INTEGER),
    // 特殊Token
    D(TT_EOF),
    D(TT_ERROR)
};
```

这该怎么办？

通过利用宏可以解决这一问题。注意到包裹的 `D` 了么？在前面插一行 `#define D(s) s`，这样 TokenType 里，`D(x)` 就是 `x` 的意思；在下面插入这样的代码：

```cpp

#undef D
#define D(s) #s

string type2str[] = {
    // 单字符
    D(TT_LPAREN),
    D(TT_RPAREN),
    D(TT_LBRACE),
    D(TT_RBRACE),
    D(TT_SEMICOLON),
    D(TT_COMMA),
    D(TT_LBRACKET),
    D(TT_RBRACKET),
    D(TT_PLUS),
    D(TT_MINUS),
    D(TT_MUL),
    D(TT_DIV),
    D(TT_MOD),
    D(TT_XOR),
    D(TT_LT),
    D(TT_GT),
    D(TT_EXCLAMATION),
    D(TT_EQ),
    // 双字符
    D(TT_LE),
    D(TT_GE),
    D(TT_EE),
    D(TT_NE),
    D(TT_LAND),
    D(TT_LOR),
    D(TT_LSHIFT),
    D(TT_RSHIFT),
    // 多字符
    D(TT_IDENTIFIER),
    D(TT_KEYWORD),
    D(TT_INTEGER),
    // 特殊Token
    D(TT_EOF),
    D(TT_ERROR)
};

#undef D
```

先取消前面定义的 `D`，然后定义 `D(s)` 为 `#s`，这样 `D` 内包裹的东西就会自动变成字符串。接下来在 `Token` 的 `tostring()` 方法中，直接使用 `type2str[type]` 就可以获取对应的字符串了。

### Metaprogramming the AST

在实际写程序过程中，可能会发现 AST 节点种类极其繁多，最终版代码中，共有 17 种 AST 节点，总行数达到 258 行，这时如果再用手写就要累死了。实际观察一下就会发现，不同的节点操作实际上千篇一律，那么为什么不用程序自动生成 AST 呢？

这样，用下面十几行就可以描述想要生成的 AST 了：

```plain
node Stmts
subs body

node VarDecl
subs vars

node Func
tok name
subs args
sub body

node If
sub cond
sub body
sub else_branch

node For
sub init
sub cond
sub update
sub body

node While
sub cond
sub body

node Return
sub retval

node Call
tok name
subs params

node Cin
subs exprs

node Cout
subs exprs

node Endl

node Number
tok num

node ArrayElement
tok name
subs dims

node VarAccess
tok name

node UnaryOp
tok op
sub node

node BinOp
sub left
tok op
sub right

node Program
subs stmts
```

其中，`node xxx` 是 `xxx` 节点描述的开始，下面一直到另一个 `node` 都是当前节点的内容；`tok` 表示当前节点有一个成员是一个 Token，如只表示一个数字的 `NumberNode`；`sub` 表示当前节点有一个成员是一个 AST 节点，如表示 `if` 语句的 `IfNode`，它的条件就是一个节点；`subs` 表示当前节点有一个成员是一些 AST 节点，比如表示一个语句块的 `StmtsNode`、表示整个程序的 `ProgramNode` 等。

假设所有 AST 节点都有一个统一的基类 `BaseNode`，那么，`sub` 可以表示为 `BaseNode *`，`subs` 就可以表示为 `vector<BaseNode *>`。

生成 AST 的程序如下：

```python
# ast_gen.py

def parse(texts, allow_tostring):
    # step 1: group each "node" to its subs "tok xxx, sub xx, subs xxx"
    grouped_texts = []
    group = []
    ast_list = []
    for text in texts:
        text = text.strip()
        if text == '': continue
        if text.startswith("node "):
            if group != []: grouped_texts.append(group)
            group = []
            text = text[5:]
            ast_list.append(text.upper())
        group.append(text.strip())
    grouped_texts.append(group) # add the last remaining group
    # step 2: generate enum NodeType & baseNode
    source = ["enum NodeType {"]
    for name in ast_list:
        source.append(f"    NODE_{name},")
    source.append("};")
    source.append('')
    source.append("struct BaseNode {")
    source.append("public:")
    source.append("    virtual ~BaseNode() {}")
    if allow_tostring: source.append("    virtual string tostring(int depth = 0, bool with_prefix = true) = 0;")
    source.append("    virtual NodeType getType() = 0;")
    source.append("};")
    source.append('')
    # step 3: generate string * int for pretty print
    source.append("string operator*(const string &str, int count) {")
    source.append("    string res;")
    source.append("    for (int i = 0; i < count; i++) res += str;")
    source.append("    return res;")
    source.append("}")
    source.append("")
    # step 4: for each group, generate its own AST structure
    for group in grouped_texts:
        source.append(f"struct {group[0]}Node : public BaseNode " + "{")
        # get subs & toks
        if len(group) == 1:
            # blank node
            source.append("public:")
            if allow_tostring:
                source.append("    virtual string tostring(int depth = 0, bool with_prefix = true) {")
                source.append(f"        return string(\"    \") * depth + \"{group[0]}Node[]\";")
                source.append("    }")
            source.append("")
        else:
            subs = []
            toks = []
            subss = []
            requirements_without_prefix = []
            for requirements in group[1:]:
                if requirements.startswith("sub "):
                    subs.append(requirements[4:])
                    requirements_without_prefix.append(requirements[4:])
                elif requirements.startswith("tok "):
                    toks.append(requirements[4:])
                    requirements_without_prefix.append(requirements[4:])
                elif requirements.startswith("subs "):
                    subss.append(requirements[5:])
                    requirements_without_prefix.append(requirements[5:])
                else:
                    print(f"Error: unrecognizable prefix \"{requirements.split(' ')[0]}\" in AST definition of {group[0]}Node (expected 'tok', 'sub' or 'subs')")
                    exit()
            if toks: source.append("    Token " + ", ".join(toks) + ";")
            if subs: source.append("    BaseNode " + ", ".join(map(lambda name: "*" + name, subs)) + ";")
            if subss: source.append("    vector<BaseNode *> " + ", ".join(subss) + ";")
            source.append("public:")
            # generate constructor
            constructor = f"    {group[0]}Node("
            for requirement in requirements_without_prefix:
                if requirement in subs:
                    constructor += f"BaseNode *{requirement}, "
                elif requirement in toks:
                    constructor += f"Token {requirement}, "
                else:
                    constructor += f"vector<BaseNode *> {requirement}, "
            constructor = constructor[:-2] + ")"
            source.append(constructor)
            # generate idk what is that
            assigner = "      : "
            assigner += ", ".join(map(lambda x: f"{x}({x})", requirements_without_prefix))
            assigner += " {}"
            source.append(assigner)
            source.append("")
            # generate deconstructor
            source.append(f"    virtual ~{group[0]}Node() " + "{")
            # delete sub & subs
            if subs: source.append("        " + "; ".join(map(lambda x: f"delete {x}", subs)) + ";")
            if subss: source.append("\n".join(map(lambda x: f"        for (int i = 0; i < {x}.size(); i++) delete {x}[i];", subss)))
            source.append("    }")
            source.append("")
            # generate tostring
            if allow_tostring:
                # basic idea: tok -> .value, sub -> .tostring(), subs -> a concat for loop
                source.append("    virtual string tostring(int depth = 0, bool with_prefix = true) {")
                source.append("        string res;")
                source.append("        if (with_prefix) res = string(\"    \") * depth;")
                source.append(f"        res += \"{group[0]}Node[\\n\";")
                for requirement in requirements_without_prefix:
                    line = ""
                    if requirement in toks:
                        line += f"        res += string(\"    \") * (depth + 1) + \"{requirement} = \" + {requirement}.tostring()"
                    elif requirement in subs:
                        line += f"        res += string(\"    \") * (depth + 1) + \"{requirement} = \" + {requirement}->tostring(depth + 1, false)"
                    else:
                        line += f"        res += string(\"    \") * (depth + 1) + \"{requirement} = " + "{\\n\";\n" 
                        line += f"        for (int i = 0; i < {requirement}.size(); i++) " + "{\n"
                        line += f"            res += {requirement}[i]->tostring(depth + 2);\n"
                        line += f"            if (i != {requirement}.size() - 1) res += \", \\n\";\n"
                        line += "        }\n"
                        line += "        res += \"\\n\";\n"
                        line += "        res += string(\"    \") * depth;\n"
                        line += "        res += \"    }\";"
                    if requirement not in subss:
                        if requirement != requirements_without_prefix[-1]:
                            line += " + \", \\n\""
                        else:
                            line += " + \"\\n\""
                        line += ";"
                    else:
                        if requirement != requirements_without_prefix[-1]:
                            line += "\n        res += \", \\n\";"
                        else:
                            line += "\n        res += \"\\n\";"
                    source.append(line)
                source.append("        res += string(\"    \") * depth;")
                source.append("        res += \"]\";")
                source.append("        return res;")
                source.append("    }")
                source.append("")
        
        source.append("    virtual NodeType getType() {")
        source.append(f"        return NODE_{group[0].upper()};")
        source.append("    }")
        source.append("};")
        source.append("")
    
    source = source[:-1]
    print("\n".join(source))

def main(argv):
    with open(argv[1], "r") as f:
        texts = f.readlines()
    
    allow_tostring = argv[2] != "no" if len(argv) > 2 else True
    parse(texts, allow_tostring)

if __name__ == '__main__':
    import sys
    main(sys.argv)
```

使用方法（将上面 AST 的描述存为 ast_nodes.txt）：

```plain
python ast_gen.py ast_nodes.txt
```

这会把生成的代码输出到标准输出。

以上生成的代码中还包含了一个 `tostring()`，它可以把一个 AST 转化成字符串，并使用缩进描述层级结构，调试时非常好用，但很占地方，因此在不必要时，可以通过在上述命令的结尾添加一个 `no` 来禁止它生成。经过测试这样可以使源码削减 9KB。

这里还需要简单提一下 C++ 的 OOP，一个基类中只有虚方法（带 `virtual` 的方法）可以用于多态，不带的只能局限于此；子类与父类的指针可以相互转化，如果是父类的指针类型但实际存的是子类的指针，在该指针上调用方法会进入子类的方法；如果一个带 `virtual` 的方法后接 `= 0`，则为纯虚方法，子类必须实现，否则无法实例化。因此，即使语法分析返回的全是 `BaseNode` 的指针，也能够通过 `getType()` 分辨出到底是哪一个节点。这样应该就能够理解生成出来的代码为什么能够工作了。

### Erasing Left Recursion

词法分析器已经非常公式化，UVA12421 是专门练习词法分析器的，大多数语言都可以直接照搬能 AC UVA12421 的词法分析器，一般只需要在关键字方面做一些小修改即可；或者参看 P3695，以及本文开头所附代码都可以，因此不多赘述。

至于语法分析器，则值得一说。如果仍然依照 P3695 题解中提到的方法进行语法分析，看到非终结符就调用对应处理函数的话，面对以下文法时：

```plain
UNIT6 ::=
| UNIT5
| UNIT6 ^ UNIT5
```

将写出如下伪代码：

```cpp
BaseNode *parse_UNIT6() {
    BaseNode *node = parse_UNIT5();
    if (node 匹配失败) {
        BaseNode *left = parse_UNIT6();
        Token op = current_tok;
        advance(); // 跳过异或
        BaseNode *right = parse_UNIT5();
        node = new BinOpNode(left, op, right);
    }
    return node;
}
```

这样的写法看似没啥问题，实则问题很大。如果 node 匹配失败，那么 `parse_UNIT6()` 将调用 `parse_UNIT6()`。但此时，由于 `UNIT5` 没有匹配上任何东西，语法分析器中的当前 Token 并没有发生改变，新一轮 `parse_UNIT6()` 仍将继续调用 `parse_UNIT6()`，从而陷入无限递归中出不来。这是因为，`UNIT6` 实际上是一个**左递归文法**。面对有左递归的文法时，切不可直接照搬，而是要消除左递归。

对于本例而言，上述左递归的描述实际上等同于：

```plain
UNIT6 ::= UNIT5 ^ UNIT5 ^ UNIT5 ^ ...
```

也即 `UNIT5` 后面可以跟上许多 `^ UNIT5`，从而可使用 EBNF 描述为：

```plain
UNIT6 ::= UNIT5 (^ UNIT5)*
```

接下来使用 while 循环匹配异或符号即可，这样就消除了左递归。

其余没什么好注意的，仿照 P3695 中提到的逻辑进行分析即可。

### Executing the AST

终于到了执行的部分。相较于 P3695 的执行，增加了函数以及作用域的有关内容，增加了多维数组，还添加了一大堆运算符。

先来看看最基本的值类型的定义：

```cpp
enum ValueType {
    VALUE_INTEGER,
    VALUE_ARRAY,
    VALUE_FUNCTION,
    VALUE_NULL
};    

struct Value {
public:
    virtual ValueType getType() = 0;
};

struct Null : public Value {
public:
    virtual ValueType getType() {
        return VALUE_NULL;
    }
};

struct Integer : public Value {
private:
    int innerVal;
public:
    Integer(Token integer) : innerVal(0) {
        if (!integer.matches(TT_INTEGER)) innerVal = 0;
        else {
            string s = integer.value;
            for (int i = 0; i < s.size(); i++) {
                innerVal = innerVal * 10 + s[i] - '0';
            }
        }
    }
    Integer(int innerVal = 0) : innerVal(innerVal) {}
    explicit operator int() {
        return innerVal;
    }
    Integer operator+(const Integer &other) const {
        return Integer(innerVal + other.innerVal);
    }
    Integer operator-(const Integer &other) const {
        return Integer(innerVal - other.innerVal);
    }
    Integer operator*(const Integer &other) const {
        return Integer(innerVal * other.innerVal);
    }
    Integer operator/(const Integer &other) const {
        return Integer(innerVal / other.innerVal);
    }
    virtual ValueType getType() {
        return VALUE_INTEGER;
    }
};
```

这段代码与 P3695 中对应部分完全相同。

由于多维数组不好实现，这里选择把多维数组压缩成一维。

```cpp
struct Array : public Value {
    int *arr;
    vector<int> dims;
public:
    Array(vector<int> dims) : dims(dims) {
        int len = 1;
        for (int i = 0; i < dims.size(); i++) len *= dims[i];
        arr = new int[len];
        memset(arr, 0, len * sizeof(int));
    }
    virtual ~Array() { delete[] arr; }
    int generate_1d_index(vector<int> inds) {
        int ind = 0, pre = 1;
        for (int i = dims.size() - 1; i >= 0; i--) {
            ind += inds[i] * pre;
            pre *= dims[i];
        }
        return ind;
    }
    int &operator[](int ind) {
        return arr[ind];
    }
    virtual ValueType getType() {
        return VALUE_ARRAY;
    }
};
```

核心部分是 `Array` 的构造函数以及 `generate_1d_index`。构造函数容易理解，多维数组的大小就是把每一维乘起来。而 `generate_1d_index` 则是把一个多维的索引压缩成一维。

例如，已知 `int a[n1][n2][n3][...][nx]`，现在我要访问 `a[a1][a2][a3][...][ax]`。按照 C/C++ 语言的数组在内存中的表示，`a` 实际上是一个长度为 `n1` 的数组，每一维都是一个 `int[n2][...][nx]` 类型的值。因此，想要访问 `a[a1]`，就要跳过 `n2 * n3 * ... * nx * a1` 个数，后文同理。于是就得到了把多维数组压缩成一维的公式：

`ind = n2 * n3 * n4 * ... * nx * a1 + n3 * n4 * ... * nx * a2 + ... + nx * a(x-1) + ax`。

这样硬算是 $\text{O}(n^2)$ 的，通过使用因式分解，可以把时间复杂度压缩到 $\text{O}(n)$，也就是上面的实现中用到的方法。

接下来进行作用域系统的实现。

```cpp
struct SymbolTable {
    SymbolTable *parent;
private:
    map<string, Value *> symbol_table;
public:
    Value *get(string key) {
        if (symbol_table.count(key)) return symbol_table[key];
        if (parent) return parent->get(key);
        return (Value *) 1;
    }
    void set(string key, Value *val) {
        if (symbol_table.count(key)) symbol_table[key] = val;
        else if (parent) parent->set(key, val);
    }
    void declare(string key) {
        symbol_table[key] = new Integer(0);
    }
};

struct Context {
    Context *parent;
    SymbolTable *symbol_table;

    Value *current_function_retval;
    bool current_function_returned;
    Context(Context *parent = NULL, SymbolTable *symbol_table = NULL)
      : symbol_table(symbol_table), parent(parent), current_function_retval(NULL), current_function_returned(false) {}
};

Context *global_context = new Context(NULL, new SymbolTable());

Value *global_get(string name)
{
    return global_context->symbol_table->get(name);
}

void global_put(string name, Value *value)
{
    global_context->symbol_table->declare(name);
    global_context->symbol_table->set(name, value);
}
```

每一个 scope（在代码中体现为 `Context`）都有它的父亲，同时有一个 symbol table 用于存储当前 scope 内所有变量的值。对于一个 symbol table，可以进行 `get`、`set` 和 `declare` 的操作，`get` 可以获取当前作用域变量的值，如果获取不着就进一步向父亲寻找，直到 NULL 为止，也就是找到了 `global_context`，如果没有，返回 `(Value *) 1`，返回这么一个值这是为了方便调试；`declare` 用于在当前作用域声明一个变量；`set` 则用于给变量赋值，如果这个作用域不存在这个变量，那就进一步往上找，以此类推。

在 `Context` 上还添加了当前函数是否已经返回的标记，这是因为如果一个函数已经返回了，那么不能执行后面的语句，而是要跳过它。这在下面的代码中将有所体现。

函数分为两种：built-in（`putchar`）以及 user-defined（被表示为 `FuncNode`）。由于语法分析阶段没有做出区分，只能在执行阶段再做打算。

这里采用的方法是，让它们都继承一个共同的基类 `BaseFunction`：

```cpp
struct BaseFunction : public Value {
    string name;
    Context *cur_context;
public:
    BaseFunction(string name, Context *cur_context) : name(name), cur_context(cur_context) {}
    virtual string getName() {
        return name;
    }
    virtual Context *generate_new_context() {
        Context *new_context = new Context(cur_context, new SymbolTable());
        new_context->symbol_table->parent = cur_context->symbol_table;
        return new_context;
    }
    virtual void check_and_populate_args(vector<string> &arg_names, vector<Value *> &args, Context *exec_ctx) {
        if (arg_names.size() < args.size()) {
            *(int *) NULL;
        } else if (arg_names.size() > args.size()) {
            *(int *) NULL;
        }

        for (int i = 0; i < args.size(); i++) {
            exec_ctx->symbol_table->declare(arg_names[i]);
            exec_ctx->symbol_table->set(arg_names[i], args[i]);
        }
    }
    virtual Value *execute(vector<Value *> &params) = 0;
};
```

它的两个主要方法是 `check_and_populate_args` 以及 `generate_new_context`，分别用于生成新的 `Context` 以及填入函数参数。在调用函数时，统一调用 `BaseFunction` 的 `execute` 方法。

这样便可以如此实现 `putchar` 了：

```cpp
struct NativePutchar : public BaseFunction {
    string name;
    Context *cur_context;
public:
    NativePutchar(string name, Context *cur_context = global_context) : BaseFunction(name, cur_context) {}
    virtual Value *execute(vector<Value *> &params) {
        int ch = int(*(Integer *) params[0]);
        return new Integer(putchar(ch));
    }
    virtual ValueType getType() {
        return VALUE_FUNCTION;
    }
};
```

普通函数的调用由于依赖后面的 `Interpreter`，所以放到后面再说，这里先简单放一个 stub：

```cpp
struct Function : public BaseFunction {
    BaseNode *body;
    vector<string> arg_names;
public:
    Function(string name, Context *cur_context, BaseNode *body, vector<string> arg_names)
      : BaseFunction(name, cur_context), body(body), arg_names(arg_names) {}
    virtual Value *execute(vector<Value *> &params);
    virtual ValueType getType() {
        return VALUE_FUNCTION;
    }
};
```

接下来是作用域系统在实际的执行流程中的实现。首先实现一个 `visit` 方法：

```cpp
struct Interpreter {
public:
    Value *visit(BaseNode *node, Context *exec_ctx = NULL) {
        if (exec_ctx == NULL) exec_ctx = global_context;
        switch (node->getType()) {
            case NODE_PROGRAM:
                return visit_ProgramNode((ProgramNode *) node, exec_ctx);
            case NODE_STMTS:
                return visit_StmtsNode((StmtsNode *) node, exec_ctx);
            case NODE_VARDECL:
                return visit_VarDeclNode((VarDeclNode *) node, exec_ctx);
            case NODE_FUNC:
                return visit_FuncNode((FuncNode *) node, exec_ctx);
            case NODE_IF:
                return visit_IfNode((IfNode *) node, exec_ctx);
            case NODE_FOR:
                return visit_ForNode((ForNode *) node, exec_ctx);
            case NODE_WHILE:
                return visit_WhileNode((WhileNode *) node, exec_ctx);
            case NODE_RETURN:
                return visit_ReturnNode((ReturnNode *) node, exec_ctx);
            case NODE_CALL:
                return visit_CallNode((CallNode *) node, exec_ctx);
            case NODE_CIN:
                return visit_CinNode((CinNode *) node, exec_ctx);
            case NODE_COUT:
                return visit_CoutNode((CoutNode *) node, exec_ctx);
            case NODE_ENDL:
                return visit_EndlNode((EndlNode *) node, exec_ctx);
            case NODE_NUMBER:
                return visit_NumberNode((NumberNode *) node, exec_ctx);
            case NODE_ARRAYELEMENT:
                return visit_ArrayElementNode((ArrayElementNode *) node, exec_ctx);
            case NODE_VARACCESS:
                return visit_VarAccessNode((VarAccessNode *) node, exec_ctx);
            case NODE_UNARYOP:
                return visit_UnaryOpNode((UnaryOpNode *) node, exec_ctx);
            case NODE_BINOP:
                return visit_BinOpNode((BinOpNode *) node, exec_ctx);
            default:
                return NULL;
        }
    }
};
```

在调用 `visit` 时，必须提供一个 `exec_ctx` 表示当前语句正在哪个 `Context` 中执行，并基于这个 `exec_ctx` 创建新的 `Context`。在语法分析阶段，已经把 `if`、`for`、`while` 后接的大括号看成一个复合语句（从文法的暗示上讲确实如此），因此要新开作用域的情况就只有两种：调用函数以及进入复合语句。

调用函数时，使用 `generate_new_context` 生成一个新作用域。注意所有函数都是在全局域声明的，因此生成的新作用域的父亲全部都是全局域。

```cpp
Value *Function::execute(vector<Value *> &params) {
    Interpreter interp;
    Context *new_ctx = generate_new_context();
    check_and_populate_args(arg_names, params, new_ctx);
    interp.visit(body, new_ctx);
    if (new_ctx->current_function_returned) {
        Value *retval = new_ctx->current_function_retval;
        return retval;
    } else return new Integer(0);
}
```

大概类似于这样。

在复合语句中创建新的作用域基本等于把 `generate_new_context` 给搬运过来：

```cpp
    Value *visit_StmtsNode(StmtsNode *node, Context *exec_ctx) {
        if (exec_ctx->current_function_returned) return new Null();
        Context *new_context = new Context(exec_ctx, new SymbolTable());
        new_context->symbol_table->parent = exec_ctx->symbol_table;
        for (int i = 0; i < node->body.size(); i++) {
            visit(node->body[i], new_context);
        }
        return new Null();
    }
```

上述截取的第 3、4 两行就是创建新作用域，第 6 行的 `visit` 全部在新作用域中执行语句。第 2 行则是 `return` 时要用的。

在 `return` 时，要给它的 `Context` 挂上一个当前函数已经返回的标志。注意有可能一个函数在 `if` 或者 `while` 很里面 `return`，所以需要一直挂到当前函数的作用域为止。

```cpp
    Value *visit_ReturnNode(ReturnNode *node, Context *exec_ctx) {
        if (exec_ctx->current_function_returned) return new Null();
        Integer *retval = (Integer *) visit(node->retval, exec_ctx);
        while (exec_ctx->parent) {
            exec_ctx->current_function_returned = true;
            exec_ctx->current_function_retval = retval;
            exec_ctx = exec_ctx->parent;
        }
        return new Null();
    }
```

在一个 `Context` 内给变量赋值码量较大，因此封装了一个 `assign` 函数专门用于赋值：

```cpp
    void assign(Context *exec_ctx, BaseNode *var, Value *value) {
        if (var->getType() == NODE_VARACCESS) {
            VarAccessNode *van = (VarAccessNode *) var;
            string name = van->name.value;
            exec_ctx->symbol_table->set(name, value);
        } else if (var->getType() == NODE_ARRAYELEMENT) {
            ArrayElementNode *aen = (ArrayElementNode *) var;
            string name = aen->name.value;
            Array *arr = (Array *) exec_ctx->symbol_table->get(name);
            vector<int> indexes;
            for (int i = 0; i < aen->dims.size(); i++) {
                Integer *dim = (Integer *) visit(aen->dims[i], exec_ctx);
                indexes.push_back(int(*dim));
            }
            int ind = arr->generate_1d_index(indexes);
            (*arr)[ind] = int(*(Integer *) value);
        }
    }
```

按普通变量和数组分类讨论，对普通变量直接 `set`，对数组则先把每一维计算出来，再调用 `generate_1d_index` 算出压缩成一维后的索引，最后赋值。以上代码可以重用在实现 `cin` 以及后面的赋值运算符中。

对于 `cout`，由于把 `endl` 单独做成了一个节点，需要特判 `endl`，不能调用它的 `visit` 方法，因为 `cout` 只能输出整数而不能输出字符，后者是 `putchar` 的工作。

赋值运算符被放在了 `BinOpNode` 里，因此还需要特判：

```cpp
    Value *visit_BinOpNode(BinOpNode *node, Context *exec_ctx) {
        if (exec_ctx->current_function_returned) return new Null();
        Value *ret;
        if (node->op.matches(TT_EQ)) {
            BaseNode *var = node->left;
            Integer *val = (Integer *) visit(node->right, exec_ctx);
            assign(exec_ctx, var, val);
            ret = val;
        } else {
            Integer *left = (Integer *) visit(node->left, exec_ctx);
            Integer *right = (Integer *) visit(node->right, exec_ctx);
            int l = int(*left);
            int r = int(*right);
            switch (node->op.type) {
                case TT_PLUS: ret = new Integer(l + r); break;
                case TT_MINUS: ret = new Integer(l - r); break;
                case TT_MUL: ret = new Integer(l * r); break;
                case TT_DIV: ret = new Integer(l / r); break;
                case TT_MOD: ret = new Integer(l % r); break;
                case TT_LT: ret = new Integer(l < r); break;
                case TT_GT: ret = new Integer(l > r); break;
                case TT_LE: ret = new Integer(l <= r); break;
                case TT_GE: ret = new Integer(l >= r); break;
                case TT_EE: ret = new Integer(l == r); break;
                case TT_NE: ret = new Integer(l != r); break;
                case TT_XOR: ret = new Integer(l ^ r); break;
                case TT_LAND: ret = new Integer(l && r); break;
                case TT_LOR: ret = new Integer(l || r); break;
                default: ret = new Null(); break;
            }
        }
        return ret;
    }
```

以上差不多就是几个比较大的细节点，除此以外执行中还有很多非常小的细节点，比如遇到 `for` 的条件是空的如何处理（给出代码中在语法分析阶段已经处理，若条件为空则默认为一个数字节点 1），定义一个函数时要先声明它，等等等等。在前面的代码中也有一些已经覆盖到的小的细节点，包括但不限于函数的参数要声明之类的，这里不再赘述。总之遇到 bug 只能自己慢慢调了（）

fun fact：我一开始只得到 20 分。给函数添加了默认返回值 0 得了 10 分，给数组设置默认值 0 得了 10 分，修了作用域问题得了 30 分，修了一元运算符解析出错得了 20 分，修了 `for` 循环解析出错又拿 10 分，这才终于拿到满分。这里把这些易错点都列出来以供后人参考，同时警示后人。

以上。