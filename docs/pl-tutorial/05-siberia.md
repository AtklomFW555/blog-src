以下是我们的目标语言 Siberia 的一个示例文件：

**代码 5-1 test.siberia**
```sib
a = 0
b = 0

if (1 + 1 == 2) {
    a = 1
} elif (1 + 1 == 3) {
    a = 2
} else {
    a = 3
}

for (i = 1; i <= 10; i = i + 1) {
    a = b = null
}


a = 1
while (a < 10) {
    a = a + 1
}

a = 1
b = 0.1
c = "abc"
d = [1, 0.1, ["a", 'bc']]

func foo(a, b) {
    c = a + b; return a + b
}

c = foo(1, 2)
print(c) # this is a comment

#=
this
is
also
a
comment
=#
```