# 如何让能源利用效率最大化？——基于数学模型和信息技术的智能路灯控制系统
## 项目背景
珲春市第二高级中学校于 2026.3 至 2026.5 开展首届 “智汇珲春·创享未来” 项目式学习活动，本项目组选择此课题进行学习。

## 项目介绍
本项目针对珲春市区部分路灯固定时间开关、恒定亮度运行导致的一系列问题，构建了一套 **“感知—计算—控制—交互”** 的系统。该系统目前处于试验阶段，理想模型构建并测试成功，距投入至实际路政建设还有一定距离。

## 解决方案
| 层次 | 功能 | 对应技术 |
|------|------|------|
| **感知层** | 每日爬取珲春市日出/日落时间及实时 AQI，网络失败或某一数值不存在则返回空值 | Python |
| **计算层** | 优先调用真实数据；失败时使用数学模型估算日出日落，AQI 使用最近成功值。根据数学模型和 AQI 因子修正（雾霾越重亮度越亮）计算PWM值，发送至串口 | C++ |
| **控制层** | 接收 PWM 值并输出至 LED（使用 MOSFET 传感器控制 12V 灯带）；红外传感器检测行人通过，提亮，人离开后恢复 | Arduino |
| **交互层** | 在项目展示中，为有效展示成果，设计本地网页控制台，模拟时间、AQI，直观展示算法效果 | Web |

## 技术依赖
- **Python >= 3.8 ( 推荐 3.11+ )**：爬虫，获取数据
- **C++11及以上 ( MinGW-w64 )**：核心算法与串口通信
- **Arduino UNO**：传感器读取与 LED 驱动
- **Web**：Vue + Node.js（ Bun 运行时）

## 快速开始

> 注意：本次开发全程在 Windows 环境下进行，因此教程按照 Windows 下的命令演示，其他平台请自行修改。

### 1. 克隆仓库
```bash
git clone https://github.com/cuixy09/smart-light-system.git
cd smart-light-system
```

### 2. Python 环境准备
使用 **uv** 管理依赖，也可用 pip 替代。
```bash
# 方式1: 使用 uv（推荐）
uv sync
.venv\Scripts\activate

# 方式2: 使用 pip
pip install -e .
```

### 附：Python API List
函数：`grab_data(data_str: str)`  
参数：`data_str`，格式 `"YYYY-MM-DD"`，例如 `"2026-04-15"`  
返回值：`(sunrise_time:str, sunset_time:str, AQI:int)`  
若任一数据为 `None`，则返回 `(None, None, None)`。

调用示例：
```python
from weather_api import grab_data
sunrise, sunset, aqi = grab_data("2026-04-15")
```

>Written by Senvlin previously.

### 3. C++ 编译
确保已安装 MinGW-w64 且 Python 路径正确（给出开发人员 cuixy09 使用的路径，请按实际修改）。
```bash
g++ main.cpp -O2 -o main.exe -I"C:\Users\CUI\AppData\Local\Programs\Python\Python313\include" -I. -L"C:\Users\CUI\AppData\Local\Programs\Python\Python313\libs" -lpython313 -lws2_32

main.exe
```
>Written by cui_xiaoyu previously.

### 4. Arduino 部署
1. VS Code 安装 PlatformIO IDE 插件。
2. 打开 `arduino\` 目录，等待自动识别工程。
3. 编译并上传到板子。

### 5. Web 控制（可选）
使用 Bun 管理前端与后端：
```bash
# 安装依赖（推荐）
bun install
bun run build

# 或分别进入子包
cd packages\web && bun run build
cd packages\web-server && bun run build
```
复制编译文件至执行文件目录下 `web` 文件夹中，然后：
```bash
server.exe
```
打开浏览器，访问 `localhost:3000`，通过滑块调节。
>Written by 你觉得我是谁 previously.

## 贡献人员（致谢名单）
- **崔晓宇（cuixy09，我）**：C++ 端开发，部分硬件采购
- **Senvlin_**：Python 端开发
- **shujia-sja（你觉得我是谁）**：Arduino 端开发，大部分硬件采购和电路设计搭建
- **zoala、金晓玲**：初期数据收集（虽然但是后面使用的爬虫），数学模型构建
- 特别感谢 **zoala** 提供路演和博览会环境搭建支持
- **苍天悠悠**：文档整理，~~难道是吉祥物~~
- 还有大模型。在实际开发中，由于我刚从 OIer 转型，部分框架是由 AI 辅助生成的，关键的算法设计和程序设计由我完成。