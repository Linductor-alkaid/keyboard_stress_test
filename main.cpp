#include "keyboard_simulator.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <windows.h>

void printUsage(const char* programName) {
    std::cout << "键盘输入压力测试工具" << std::endl;
    std::cout << "用法: " << programName << " [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -t, --text <文本>        要输入的文本内容（默认: \"test\"）" << std::endl;
    std::cout << "                           可以多次使用此选项添加多个字符组" << std::endl;
    std::cout << "                           每个周期会随机选择一个字符组输入" << std::endl;
    std::cout << "  -f, --frequency <频率>   输入频率（每秒输入次数，默认: 10）" << std::endl;
    std::cout << "  -d, --delay <延迟>       输入延迟（毫秒，默认: 100）" << std::endl;
    std::cout << "  -h, --help               显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << programName << " -t \"Hello World\" -f 20" << std::endl;
    std::cout << "  " << programName << " --text \"test123\" --delay 50" << std::endl;
    std::cout << "  " << programName << " -t \"test1\" -t \"test2\" -t \"test3\"" << std::endl;
    std::cout << std::endl;
    std::cout << "操作说明:" << std::endl;
    std::cout << "  1. 运行程序后，程序会等待鼠标左键点击" << std::endl;
    std::cout << "  2. 在目标输入框中点击鼠标左键开始输入" << std::endl;
    std::cout << "  3. 点击鼠标右键暂停输入，再次点击左键继续" << std::endl;
    std::cout << "  4. 按 ESC 键退出程序" << std::endl;
}

bool parseArguments(int argc, char* argv[], std::vector<std::string>& texts, double& frequency, int& delay) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            return false;
        } else if (arg == "-t" || arg == "--text") {
            if (i + 1 < argc) {
                texts.push_back(argv[++i]);
            } else {
                std::cerr << "错误: -t 选项需要参数" << std::endl;
                return false;
            }
        } else if (arg == "-f" || arg == "--frequency") {
            if (i + 1 < argc) {
                frequency = std::stod(argv[++i]);
            } else {
                std::cerr << "错误: -f 选项需要参数" << std::endl;
                return false;
            }
        } else if (arg == "-d" || arg == "--delay") {
            if (i + 1 < argc) {
                delay = std::stoi(argv[++i]);
            } else {
                std::cerr << "错误: -d 选项需要参数" << std::endl;
                return false;
            }
        } else {
            std::cerr << "未知选项: " << arg << std::endl;
            return false;
        }
    }
    
    return true;
}

// 设置控制台UTF-8编码
void setupConsoleUTF8() {
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    // 设置控制台字体以支持UTF-8
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

int main(int argc, char* argv[]) {
    // 设置控制台UTF-8编码
    setupConsoleUTF8();
    
    std::vector<std::string> inputTexts = {"test"};  // 默认字符组
    double frequency = 10.0;  // 每秒10次
    int delay = 100;          // 100毫秒
    
    // 解析命令行参数
    if (!parseArguments(argc, argv, inputTexts, frequency, delay)) {
        printUsage(argv[0]);
        return 1;
    }
    
    // 如果设置了频率，计算延迟
    if (frequency > 0) {
        delay = static_cast<int>(1000.0 / frequency);
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "   键盘输入压力测试工具" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "输入字符组数量: " << inputTexts.size() << std::endl;
    for (size_t i = 0; i < inputTexts.size(); i++) {
        std::cout << "  字符组 " << (i + 1) << ": \"" << inputTexts[i] << "\"" << std::endl;
    }
    std::cout << "输入频率: " << std::fixed << std::setprecision(2) << frequency << " 次/秒" << std::endl;
    std::cout << "输入延迟: " << delay << " 毫秒" << std::endl;
    std::cout << "随机模式: 每个周期随机选择一个字符组" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    KeyboardSimulator simulator;
    // 添加所有字符组
    for (const auto& text : inputTexts) {
        simulator.addInputText(text);
    }
    simulator.setInputDelay(delay);
    
    // 设置控制台处理程序，用于捕获 Ctrl+C
    SetConsoleCtrlHandler([](DWORD dwCtrlType) -> BOOL {
        if (dwCtrlType == CTRL_C_EVENT) {
            return TRUE;
        }
        return FALSE;
    }, TRUE);
    
    simulator.start();
    
    std::cout << "程序运行中... 按 ESC 键退出" << std::endl;
    std::cout << "提示: 左键开始/继续，右键暂停" << std::endl;
    
    // 主循环
    while (simulator.isRunning() && !simulator.shouldExit()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    simulator.stop();
    
    return 0;
}

