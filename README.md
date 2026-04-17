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

3. 复制vue编译文件至执行文件目录下web文件夹中
4. 运行服务器
```bash
./server
```

### Arduino
1. vscode 安装`PlatformIO IDE`插件
2. 导入项目

 > *Written by 你觉得我是谁*