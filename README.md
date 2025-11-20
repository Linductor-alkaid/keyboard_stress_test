# 键盘输入压力测试工具

一个用于软件压力测试的C++工具，可以模拟键盘输入，支持可配置的输入内容和频率。

## 功能特性

- ✅ 可配置的输入内容（支持多个字符组）
- ✅ 每个周期随机选择一个字符组输入
- ✅ 可配置的输入频率（每秒输入次数）
- ✅ 字符均匀分布在输入周期内
- ✅ 鼠标左键点击触发机制
- ✅ 鼠标右键暂停/左键继续功能
- ✅ ESC键退出程序
- ✅ 支持Unicode字符输入
- ✅ UTF-8控制台输出支持（PowerShell友好）
- ✅ 命令行参数配置

## 编译要求

- CMake 3.10 或更高版本
- C++17 兼容的编译器（MSVC、GCC、Clang）
- **Windows**: 需要Windows API支持
- **Linux**: 需要X11开发库（libX11-dev, libXtst-dev）
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libx11-dev libxtst-dev
  
  # Fedora/RHEL
  sudo dnf install libX11-devel libXtst-devel
  
  # Arch Linux
  sudo pacman -S libx11 libxtst
  ```

## 编译方法

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

编译后的可执行文件位于 `build/bin/` 目录下。

## 使用方法

### 基本用法

```bash
# Windows - 使用默认设置（文本: "test", 频率: 10次/秒）
KeyboardStressTest.exe

# Linux - 使用默认设置（文本: "test", 频率: 10次/秒）
./KeyboardStressTest

# 指定输入文本
KeyboardStressTest.exe -t "Hello World"  # Windows
./KeyboardStressTest -t "Hello World"    # Linux

# 指定多个字符组（每个周期随机选择一个）
KeyboardStressTest.exe -t "test1" -t "test2" -t "test3"  # Windows
./KeyboardStressTest -t "test1" -t "test2" -t "test3"     # Linux

# 指定输入频率（每秒20次）
KeyboardStressTest.exe -f 20  # Windows
./KeyboardStressTest -f 20    # Linux

# 指定输入延迟（50毫秒）
KeyboardStressTest.exe -d 50  # Windows
./KeyboardStressTest -d 50    # Linux

# 组合使用
KeyboardStressTest.exe -t "test123" -t "abc" -t "xyz" -f 15  # Windows
./KeyboardStressTest -t "test123" -t "abc" -t "xyz" -f 15   # Linux
```

### 命令行参数

- `-t, --text <文本>`: 要输入的文本内容（默认: "test"）
  - 可以多次使用此选项添加多个字符组
  - 每个输入周期会随机选择一个字符组进行输入
  - 示例: `-t "test1" -t "test2" -t "test3"`
- `-f, --frequency <频率>`: 输入频率，每秒输入次数（默认: 10）
- `-d, --delay <延迟>`: 输入延迟，毫秒（默认: 100）
- `-h, --help`: 显示帮助信息

### 操作流程

1. 运行程序
2. 程序会等待鼠标左键点击
3. 在目标软件的输入框中点击鼠标左键开始输入
4. 点击鼠标右键可以暂停输入
5. 再次点击鼠标左键可以继续输入
6. 按 `ESC` 键退出程序

### 控制说明

- **鼠标左键**: 开始输入 / 继续输入（暂停后）
- **鼠标右键**: 暂停输入
- **ESC键**: 退出程序

## 注意事项

⚠️ **重要提示**:
- 此工具仅用于合法的软件测试目的
- 使用前请确保有权限对目标软件进行测试
- 建议在测试环境中使用，避免影响生产环境
- 某些安全软件可能会拦截键盘模拟操作

## 技术实现

### Windows平台
- 使用Windows `SendInput` API进行键盘模拟
- 使用`GetAsyncKeyState`监听鼠标点击事件和键盘事件

### Linux平台
- 使用X11 `XTest`扩展进行键盘模拟
- 使用X11 `XQueryPointer`和`XQueryKeymap`监听鼠标和键盘事件
- 需要X服务器运行环境（通常在图形界面下使用）

### 通用特性
- 多线程架构：鼠标/键盘监听线程 + 输入线程
- 支持ASCII字符输入（Linux）和Unicode字符输入（Windows）
- 使用`std::mt19937`随机数生成器实现字符组随机选择
- 字符均匀分布在输入周期内，确保精确的时间控制
- 跨平台支持：Windows和Linux

## 许可证

本项目仅供学习和测试使用。

