本系列文章是笔者试图制作一个 Phigros 模拟器的记录，不是任何形式的教程，一切内容仅供参考。

---

首先进行谱面文件的解析。在进行解析之前，应该先有这样一个谱面。以这样一个谱面为例：

[谱面演示](https://www.bilibili.com/video/BV17M411z7aA/)

[点击下载谱面](INFiNiTE ENERZY -Overdoze-(SP).zip)

一张谱子本身不应该包含音乐、曲绘、作者之类的信息，但在播放谱面时这些都是极有用的，所以它们都和谱面一起，被放在了一个 `zip` 文件中。我们本节的任务，就是分离出这个谱面中的音乐、曲绘、谱面，并显示出它的信息来。

Python 中使用 `zipfile` 模块来操作 zip 文件。想要读取一个 zip 文件中的所有文件名非常简单：

**代码 1-1 读取 zip 文件的所有文件名**
```python
import zipfile
import csv
import json

def parse_chart_zip(filename):
    f = zipfile.ZipFile(filename)
    print(f.namelist())

if __name__ == '__main__':
    import sys
    parse_chart_zip(sys.argv[1])
```

这样，使用 `python pygros.py <刚下的谱面的路径>`，就可以得到如下的输出：

```plain
['1662291.ogg', '1662291.jpg', '1662291.json', 'info.txt']
```

这其中的音频文件就是音乐，图片文件就是曲绘，`json` 文件就是谱子（这个下一节甚至更远才会开始），我们本节只对 `info.txt` 这一个特定的文件感兴趣。

不过在此之前，先要分离一下音乐、曲绘和谱面。

**代码 1-2 分离音乐、曲绘、谱面**
```python
import zipfile
import csv
import json
import io

music_format = ('.ogg', '.wav', '.mp3')
image_format = ('.jpg', '.png')

def parse_chart_zip(filename):
    f = zipfile.ZipFile(filename)
    for name in f.namelist():
        if name.endswith(music_format):
            music = name
        elif name.endswith(image_format):
            illustration = name
        elif name.endswith('.json'):
            chart = name
    print("音乐文件：", music)
    print("曲绘文件：", illustration)
    print("谱面文件：", chart)

if __name__ == '__main__':
    import sys
    parse_chart_zip(sys.argv[1])
```

此时运行应该得到如下输出：

```plain
音乐文件： 1662291.ogg
曲绘文件： 1662291.jpg
谱面文件： 1662291.json
```

由于音乐和图片格式繁多，这里只列举了我目前见到的几种。直接用 `endswith` 判断后缀有点不妥，但先这么放着。

本节先不着急谈这三个东西，我们只对 `info.txt` 感兴趣。通过下面的方法，可以在不解压的情况下读取 zip 内的文件：

**代码 1-3 读取 info.txt**
```python
def parse_chart_zip(filename):
    # 上略...
    with f.open("info.txt", "r") as info_file:
        info = info_file.read()
    
    info = info.decode('utf-8')
    print(info)
```

用这种方法读取的 `info` 变量最终是字节串而非字符串，所以需要再 `decode` 一遍。这一次输出的信息就更多了：

```plain
#
Name: INFiNiTE ENERZY -Overdoze-
Path: 1662291
Song: 1662291.ogg
Picture: 1662291.jpg
Chart: 1662291.json
Level: SP Lv.?
Composer: Reku Mochizuki
Charter: -囃×狼-
```

第一行 `#` 应该是固定的不用管。剩下的几行，通过给文件名的方式直接确定了文件的位置，相当于省略了我们自己去找的麻烦。

`info.txt` 格式比较固定，以下对它进行解析：

**代码 1-4 解析 `info.txt`**
```python
def parse_chart_zip(filename):
    # 上略...
    infos = info.splitlines()[1:]
    infos = [info.split(':', 1) for info in infos]
    infos = [[x, y.strip()] for x, y in infos]
    infos = dict(infos)
    print(infos)
```

最终的 `infos` 被解析成一个字典如下：

```plain
{'Name': 'INFiNiTE ENERZY -Overdoze-', 'Path': '1662291', 'Song': '1662291.ogg', 'Picture': '1662291.jpg', 'Chart': '1662291.json', 'Level': 'SP Lv.?', 'Composer': 'Reku Mochizuki', 'Charter': '-囃×狼-'}
```

从这个字典中，就可以读出谱师、曲师之类的东西来：

**代码 1-5 利用 `info.txt` 的内容输出信息**
```python
def parse_chart_zip(filename):
    f = zipfile.ZipFile(filename)

    if "info.txt" in f.namelist():
        with f.open("info.txt", "r") as info_file:
            info = info_file.read()
        
        info = info.decode('utf-8')
        infos = info.splitlines()[1:]
        infos = [info.split(':', 1) for info in infos]
        infos = [[x, y.strip()] for x, y in infos]
        infos = dict(infos)
        
        name = infos.get("Name", "UK")
        song = infos.get("Song")
        illustration = infos.get("Picture")
        chart = infos.get("Chart")
        level = infos.get("Level", "???")
        composer = infos.get("Composer", "UK")
        charter = infos.get("Charter", "UK")
        illustrator = infos.get("Illustrator", "UK")

        print("谱面名称：{} ({})".format(name, level))
        print("谱师：{}".format(charter))
        print("曲师：{}".format(composer))
        print("曲绘画师：{}".format(illustrator))
        print("谱面文件：{} 曲绘文件：{} 乐曲文件：{}".format(chart, illustration, song))
```

注意这里没有“上略”了，也就是说应该用这个代码替换掉整个函数。对于这个谱面，输出如下：

```plain
谱面名称：INFiNiTE ENERZY -Overdoze- (SP Lv.?)
谱师：-囃×狼-
曲师：Reku Mochizuki
曲绘画师：UK
谱面文件：1662291.json 曲绘文件：1662291.jpg 乐曲文件：1662291.ogg
```

这样就初步完成了对于谱面 zip 文件的解析。

然而，不是所有的谱面都有 `info.txt`，有的谱面是 `info.csv`，有的干脆没有这一类的东西，这就需要分情况进行讨论。

以下将会再各提供一个使用 `info.csv` 和无 `info` 文件的谱面（后者多为手工打包得来）。

[使用 info.csv 的谱面（2022 愚人节）](Spasmodic (Haocore Mix).zip)

[无 info 文件的谱面（Ποσειδών AT）](Ποσειδών.zip)

使用 `info.csv` 的情况更为简易，因为 Python 有解析 `csv` 的内置库，已在代码 1-1 中导入了。

**代码 1-6 解析 `info.csv`**
```python
def parse_chart_zip(filename):
    # 上略...
    if "info.txt" in f.namelist(): # 中略...
    elif "info.csv" in f.namelist():
        with f.open("info.csv", "r") as info_file:
            info = info_file.read()
        
        info = info.decode('utf-8')
        reader = csv.DictReader(io.StringIO(info))
        next(reader)
        entry = next(reader)
        name = entry.get("Name", "UK")
        song = entry.get("Music")
        illustration = entry.get("Image")
        chart = entry.get("Chart")
        level = entry.get("Level", "???")
        composer = entry.get("Artist", "UK")
        charter = entry.get("Designer", "UK")
        illustrator = entry.get("Illustrator", "UK")
```

最后是都没有的情况，需要自己遍历：

**代码 1-7 zip笑传之猜chart be**
```python
def parse_chart_zip(filename):
    # 上略...
    if "info.txt" in f.namelist(): # 中略...
    elif "info.csv" in f.namelist(): # 中略...
    else:
        name = "UK"
        level = "???"
        composer = "UK"
        charter = "UK"
        illustrator = "UK"
        chart = "UK"
        song = "UK"
        illustration = "UK"
        for entry in f.namelist():
            if entry.endswith(".json"):
                chart = entry
            elif entry.endswith(music_format):
                song = entry
            elif entry.endswith(image_format):
                illustration = entry
```

最后，我们返回一个格式统一的字典：

**代码 1-8 谱面解析结束**
```python
def parse_chart_zip(filename):
    # 上略...
    if "info.txt" in f.namelist(): # 中略...
    elif "info.csv" in f.namelist(): # 中略...
    else: # 中略...
    return {"name": name, "song": song, "illustration": illustration, "chart": chart, "level": level, "composer": composer, "charter": charter, "illustrator": illustrator}
```

把用来运行程序的最后几行改成这样：

**代码 1-9 运行时的脚本修改**
```python
if __name__ == '__main__':
    import sys
    print(parse_chart_zip(sys.argv[1]))
```

这样，分别用我们的程序去解析 IEO SP 自制谱、2022 愚人节和波塞冬 AT，可以得到类似的输出：

IEO SP：

```plain
{'name': 'INFiNiTE ENERZY -Overdoze-', 'song': '1662291.ogg', 'illustration': '1662291.jpg', 'chart': '1662291.json', 'level': 'SP Lv.?', 'composer': 'Reku Mochizuki', 'charter': '-囃×狼-', 'illustrator': 'UK'}
```

2022 愚人节：

```plain
{'name': 'Spasmodic(Haocore Mix)', 'song': 'SpasmodicSP.ogg', 'illustration': 'SpasmodicSP.png', 'chart': 'SpasmodicSP.json', 'level': 'SP\u2002Lv.?', 'composer': 'UK', 'charter': 'Phigros Spasming Team "無極病院"', 'illustrator': '笔记RE'}
```

波塞冬 AT：

```plain
{'name': 'UK', 'song': 'Ποσειδών.wav', 'illustration': '▓¿╚√╢¼.png', 'chart': 'chart ▓¿╚√╢¼.json', 'level': '???', 'composer': 'UK', 'charter': 'UK', 'illustrator': 'UK'}
```

因为编码问题所以波塞冬 AT 的文件名乱了，不过总体上问题不大。至此第一节就可以结束了。