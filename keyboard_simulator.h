#ifndef KEYBOARD_SIMULATOR_H
#define KEYBOARD_SIMULATOR_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <random>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <cstdint>
#endif

/**
 * 键盘模拟器类
 * 用于模拟键盘输入，支持可配置的频率和内容
 */
class KeyboardSimulator {
public:
    KeyboardSimulator();
    ~KeyboardSimulator();

    // 设置要输入的内容（单个文本）
    void setInputText(const std::string& text);
    
    // 添加输入文本组（支持多个字符组）
    void addInputText(const std::string& text);
    
    // 清空所有输入文本组
    void clearInputTexts();
    
    // 设置输入频率（每秒输入次数）
    void setInputFrequency(double frequency);
    
    // 设置延迟时间（毫秒，每次输入之间的间隔）
    void setInputDelay(int delayMs);
    
    // 开始监听鼠标点击并准备输入
    void start();
    
    // 停止输入
    void stop();
    
    // 检查是否正在运行
    bool isRunning() const;
    
    // 检查是否已激活（鼠标点击后）
    bool isActive() const;
    
    // 检查是否应该退出（ESC键）
    bool shouldExit() const;

private:
    // 模拟键盘输入
    void simulateKeyInput(char key);
    void simulateStringInput(const std::string& text);
    
    // 鼠标和键盘监听线程
    void inputMonitorThread();
    
    // 输入线程
    void inputThread();
    
    // 随机选择一个输入文本组
    std::string getRandomInputText();
    
    // 检查鼠标左键是否被点击
    bool isMouseLeftButtonClicked();
    
    // 检查鼠标右键是否被点击
    bool isMouseRightButtonClicked();
    
    // 检查ESC键是否被按下
    bool isEscKeyPressed();

private:
    std::vector<std::string> m_inputTexts;    // 输入文本组列表（支持多个字符组）
    int m_inputDelayMs;                       // 输入延迟（毫秒）
    std::atomic<bool> m_running;              // 是否正在运行
    std::atomic<bool> m_active;               // 是否已激活（鼠标点击后）
    std::atomic<bool> m_paused;               // 是否暂停（右键暂停）
    std::atomic<bool> m_shouldExit;           // 是否应该退出（ESC键）
    std::thread m_monitorThread;              // 鼠标和键盘监听线程
    std::thread m_inputThread;                // 输入线程
#ifdef _WIN32
    DWORD m_lastLeftMouseState;               // 上次左键鼠标状态
    DWORD m_lastRightMouseState;              // 上次右键鼠标状态
#elif __linux__
    bool m_lastLeftMouseState;                // 上次左键鼠标状态
    bool m_lastRightMouseState;                // 上次右键鼠标状态
    Display* m_display;                        // X11显示连接
    std::mutex m_displayMutex;                 // X11显示连接互斥锁（X11不是线程安全的）
#endif
    std::mt19937 m_randomGenerator;           // 随机数生成器
};

#endif // KEYBOARD_SIMULATOR_H

