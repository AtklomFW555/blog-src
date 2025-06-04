本节的任务比较简单，只需要显示曲绘并开始播放歌曲即可。

为了简化后续的任务，我们安装一个第三方库 `pygame`，直接 `pip install pygame==2.6.1` 即可。`2.6.1` 是我写作本文（2025.2.21）时的版本，由于不知道后续是否存在兼容性问题，所以下载同样的版本最为保险。

`pygame` 顾名思义，在 Python 中可以用它来写游戏，封装了一些诸如 sprite 之类的东西，让我们的生活变得更加便利。目前我们暂时还用不到 `pygame` 里的好东西，我们只用到它的几个简单要素，我们待会再提。

好了，想要显示曲绘，先得有个窗口对吧，创建窗口是简单的：

**代码 2-1 创建窗口**

```python
# 上略...

import pygame
import pygame.mixer

def parse_chart_zip(filename): ...

def mainloop():
    pygame.init()
    pygame.mixer.init()

    pygame.display.set_caption("Pygros")

    screen = pygame.display.set_mode((800, 450))

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                exit()
        pygame.display.flip()

if __name__ == '__main__':
    mainloop()
```

这里不仅导入了 `pygame` 还导入了 `pygame.mixer`，后者是用来播放音乐的。同时在启动时，只调用 `mainloop`。这样，一个大小为 800 * 450、标题为“Pygros”的黑窗口将被创建出来。

接下来来显示曲绘。一个初步的想法是曲绘多大窗口就多大，这样才能容纳得下曲绘。`pygame` 提供 `pygame.image.load()` 用于加载图片，先读取曲绘并加载之：

**代码 2-2 读取曲绘**
```python
def mainloop():
    # 上略...
    chart_info = parse_chart_zip(sys.argv[1])
    f = zipfile.ZipFile(sys.argv[1])
    with f.open(chart_info['illustration'], "r") as ill:
        byt = ill.read()
    illustration = pygame.image.load(io.BytesIO(byt))
    ill_rect = illustration.get_rect()

    while True: ...
```

然而，经过调查研究，一个曲绘可能很大，如波塞冬 AT 的曲绘就有 2048 * 1080 的超级高清。接下来我们尝试把它缩成 800 * 450 的大小，如果无法做到，那么尽可能保证竖直方向为 450 像素，这样后期显示谱面时好看一些。

**代码 2-3 缩小曲绘**
```python
def mainloop():
    # 上略...

    while ill_rect.width >= 1920 or ill_rect.height >= 1080:
        ratio = 450 / ill_rect.height
        ill_rect = pygame.Rect(0, 0, int(ill_rect.width * ratio), int(ill_rect.height * ratio))
    
    while True: ...
```

现在就可以用修改后的 `ill_rect` 来创建窗口了：

**代码 2-4 创建窗口（新）**
```python
def mainloop():
    pygame.init()
    pygame.mixer.init()

    pygame.display.set_caption("Pygros")

    chart_info = parse_chart_zip(sys.argv[1])
    f = zipfile.ZipFile(sys.argv[1])
    with f.open(chart_info['illustration'], "r") as ill:
        byt = ill.read()
    illustration = pygame.image.load(io.BytesIO(byt))
    ill_rect = illustration.get_rect()
    while ill_rect.width >= 1920 or ill_rect.height >= 1080:
        ratio = 450 / ill_rect.height
        ill_rect = pygame.Rect(0, 0, int(ill_rect.width * ratio), int(ill_rect.height * ratio))
    screen = pygame.display.set_mode((ill_rect.width, ill_rect.height))

    while True: ...
```

在创建了 `screen` 以后，就可以调用图片的 `convert` 方法，把它转化成更易于操作的格式。接下来，把它的透明度调到 0.6，然后缩放到修改后的大小，然后显示它：

**代码 2-5 显示曲绘**
```python
def mainloop():
    # 上略...
    illustration = illustration.convert()
    illustration.set_alpha(0.6 * 255)
    illustration = pygame.transform.scale(illustration, (ill_rect.width, ill_rect.height))
    screen.blit(illustration, (0, 0))

    while True: ...
```

接下来来播放音乐。通过 `pygame.mixer.music` 系列的方法，可以播放音乐。具体步骤如下：

**代码 2-6 播放音乐**
```python
def mainloop():
    # 上略...

    with f.open(chart_info['song'], "r") as song:
        byt = song.read()
    pygame.mixer.music.load(io.BytesIO(byt))
    pygame.mixer.music.set_volume(0.5)
    pygame.mixer.music.play()
```

先加载，设置音量为 0.5，然后播放它。

最后，Phigros 的谱面以音乐结束（而不以谱面结束）为结束标志。当音乐结束时，顺带把窗口关闭。判断音乐是否仍在播放的方法是 `pygame.mixer.music.get_busy()`，将下面的循环修改如下：

**代码 2-7 音乐结束时结束程序**
```python
def mainloop():
    # 上略...

    while pygame.mixer.music.get_busy():
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                exit()
        pygame.display.flip()
    
    pygame.quit()
```

于是，一个带曲绘的音乐播放器就完成了。下面来显示 UI，它包括分数、曲名、难度以及暂停键。这些材质在：

[font_ui.ttf](font_ui.ttf)

[暂停键（右键另存为保存）](Pause.png)

这四个东西的位置是较好解决的，位于左上、左下、右上、右下。但这个左上、左下之类的可不是窗口的，而是**谱面区域的**，它是一个手机 4:3、平板 16:9 的区域。由于大部分谱面是按 4:3 写的（e.g. DESTRUCTION 3,2,1 IN/AT 里的“大金蛋”其实是 4:3 的圆形在 16:9 的显示效果），这里也按 4:3 来弄。

（咕了。后面补）

`mainloop` 完整代码：

**代码 2-8 `mainloop` 完整代码**
```python
def mainloop():
    pygame.init()
    pygame.mixer.init()

    pygame.display.set_caption("Pygros")

    chart_info = parse_chart_zip(sys.argv[1])
    f = zipfile.ZipFile(sys.argv[1])
    with f.open(chart_info['illustration'], "r") as ill:
        byt = ill.read()
    illustration = pygame.image.load(io.BytesIO(byt))
    ill_rect = illustration.get_rect()
    while ill_rect.width >= 1920 or ill_rect.height >= 1080:
        ratio = 450 / ill_rect.height
        ill_rect = pygame.Rect(0, 0, int(ill_rect.width * ratio), int(ill_rect.height * ratio))
    screen = pygame.display.set_mode((ill_rect.width, ill_rect.height))
    illustration = illustration.convert()
    illustration.set_alpha(0.6 * 255)
    illustration = pygame.transform.scale(illustration, (ill_rect.width, ill_rect.height))
    screen.blit(illustration, (0, 0))

    with f.open(chart_info['song'], "r") as song:
        byt = song.read()
    pygame.mixer.music.load(io.BytesIO(byt))
    pygame.mixer.music.set_volume(0.5)
    pygame.mixer.music.play()

    while pygame.mixer.music.get_busy():
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                exit()
        pygame.display.flip()
    
    pygame.quit()
```