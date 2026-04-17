# 项目简介

本项目为路灯节能控制研究方案。通过`Python`获取环境数据（天气、光照、AQI），结合`C++`算法与`Arduino`硬件交互，最终利用`Web`端实现成果展示及路灯控制。

## 如何使用

### Python

爬虫部分用[uv](https://github.com/astral-sh/uv)管理虚拟环境

使用的是`Python>=3.11`, 理论上支持`Python>=3.8`

1. 先确保你有python
2. `fork`一份源码到你喜欢的地方

- 如果你有`uv`
    1. `cd`到相关目录, 然后`uv sync`即可, 别忘了`./Scripts/activate`
- 如果你没有
    1. 那就去到源码目录, 在命令行输入`pip install -e .`

#### api list

函数: `grab_data(data_str:str)`

参数: `data_str` 格式为`'YYYY-MM-DD'`即`2026-04-15`

返回值 `(sunrise_time:str, sunset_time:str, AQI:int)`：

- 若其中一个解析为`None`/`Null`时 则返回为`(None,None,None)`

 > *Written by Senvlin*

### Web

在仓库根目录安装依赖并构建（Bun 工作区，子包在 `packages/` 下）：

```bash
bun install
bun run build:web
bun run build:server
```

也可进入子目录单独执行，例如：

```bash
cd packages/web
bun run build
```

```bash
cd packages/web-server
bun run build
```

1. 复制vue编译文件至执行文件目录下web文件夹中
2. 运行服务器

```bash
./server
```

### Arduino

1. vscode 安装`PlatformIO IDE`插件
2. 导入项目

 > *Written by 你觉得我是谁*

### C++

#### 简介

这部分是项目的主程序，串联起 Web, Python , Arduino 程序，核心功能：

- 自动获取当前日期和时间
- 调用 Python API获取日出、日落时间及 AQI
- 备用算法：若 Python 调用失败，切换至正弦函数模型估算日出日落时间
- 根据当前数据计算 PWM，将亮度指令发送给 Arduino 控制 LED 灯带
- 支持每 5 分钟自动运行
- 人工输入（待实现）

#### 环境说明

- 编写是在 Windows 10 专业工作站版 22H2 19045.6466 下进行的，包括 time 和串口发送都没有支持 Linux
- 编译器：MinGW-w64 (g++13.0.2，C++11 及以上)
- 其他：本地使用的环境为Python 3.13，具体参考上面

#### 编译与运行

1. 默认你已经 clone 了仓库并获取了源码
2. 将 weather_api.py 和 main.cpp 放在同一目录下，cmd 编译，命令参考（目录请自行替换成对应的 Python 目录）：

```bash
g++ main.cpp -O2 -o main.exe -I"C:\Users\CUI\AppData\Local\Programs\Python\Python313\include" -L"C:\Users\CUI\AppData\Local\Programs\Python\Python313\libs" -lpython313
```

 > *Written by cui_xiaoyu*
