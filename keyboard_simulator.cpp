#include "keyboard_simulator.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>

KeyboardSimulator::KeyboardSimulator()
    : m_inputDelayMs(100)
    , m_running(false)
    , m_active(false)
    , m_paused(false)
    , m_shouldExit(false)
#ifdef _WIN32
    , m_lastLeftMouseState(0)
    , m_lastRightMouseState(0)
#elif __linux__
    , m_lastLeftMouseState(false)
    , m_lastRightMouseState(false)
    , m_display(nullptr)
#endif
    , m_randomGenerator(std::random_device{}())
{
#ifdef __linux__
    // 初始化X11线程支持（必须在XOpenDisplay之前调用）
    XInitThreads();
    
    // 初始化X11显示连接
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        std::cerr << "错误: 无法连接到X服务器" << std::endl;
    } else {
        // 检查XTest扩展是否可用
        int event_base, error_base, major, minor;
        if (!XTestQueryExtension(m_display, &event_base, &error_base, &major, &minor)) {
            std::cerr << "警告: XTest扩展不可用，键盘模拟可能无法正常工作" << std::endl;
        }
    }
#endif
}

KeyboardSimulator::~KeyboardSimulator() {
    stop();
    // Display已经在stop()中关闭，这里不需要再次关闭
}

void KeyboardSimulator::setInputText(const std::string& text) {
    m_inputTexts.clear();
    m_inputTexts.push_back(text);
}

void KeyboardSimulator::addInputText(const std::string& text) {
    m_inputTexts.push_back(text);
}

void KeyboardSimulator::clearInputTexts() {
    m_inputTexts.clear();
}

std::string KeyboardSimulator::getRandomInputText() {
    if (m_inputTexts.empty()) {
        return "";
    }
    
    std::uniform_int_distribution<size_t> dist(0, m_inputTexts.size() - 1);
    size_t index = dist(m_randomGenerator);
    return m_inputTexts[index];
}

void KeyboardSimulator::setInputFrequency(double frequency) {
    if (frequency > 0) {
        m_inputDelayMs = static_cast<int>(1000.0 / frequency);
    }
}

void KeyboardSimulator::setInputDelay(int delayMs) {
    m_inputDelayMs = delayMs;
}

void KeyboardSimulator::start() {
    if (m_running) {
        return;
    }
    
#ifdef __linux__
    if (!m_display) {
        std::cerr << "错误: X11显示连接未初始化" << std::endl;
        return;
    }
#endif
    
    m_running = true;
    m_active = false;
    m_paused = false;
    m_shouldExit = false;
#ifdef _WIN32
    m_lastLeftMouseState = GetAsyncKeyState(VK_LBUTTON);
    m_lastRightMouseState = GetAsyncKeyState(VK_RBUTTON);
#elif __linux__
    m_lastLeftMouseState = isMouseLeftButtonClicked();
    m_lastRightMouseState = isMouseRightButtonClicked();
#endif
    
    // 启动监听线程（鼠标和键盘）
    m_monitorThread = std::thread(&KeyboardSimulator::inputMonitorThread, this);
    
    std::cout << "键盘模拟器已启动，等待鼠标左键点击..." << std::endl;
}

void KeyboardSimulator::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    m_active = false;
    m_paused = false;
    m_shouldExit = true;
    
    // 等待所有线程退出
    if (m_inputThread.joinable()) {
        try {
            m_inputThread.join();
        } catch (...) {
            // 忽略join异常
        }
    }
    
    if (m_monitorThread.joinable()) {
        try {
            m_monitorThread.join();
        } catch (...) {
            // 忽略join异常
        }
    }
    
#ifdef __linux__
    // 在关闭Display之前，确保所有X11操作都已完成
    try {
        // 先获取锁，确保没有其他线程在使用Display
        {
            std::lock_guard<std::mutex> lock(m_displayMutex);
            if (m_display) {
                XSync(m_display, False);  // 同步所有待处理的X11请求
            }
        }
        
        // 等待一小段时间，确保所有操作完成
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 关闭Display（在确保所有线程都退出后）
        {
            std::lock_guard<std::mutex> lock(m_displayMutex);
            if (m_display) {
                XCloseDisplay(m_display);
                m_display = nullptr;
            }
        }
    } catch (...) {
        // 如果关闭Display时出错，至少确保指针被清空
        std::lock_guard<std::mutex> lock(m_displayMutex);
        m_display = nullptr;
    }
#endif
    
    std::cout << "键盘模拟器已停止" << std::endl;
}

bool KeyboardSimulator::isRunning() const {
    return m_running;
}

bool KeyboardSimulator::isActive() const {
    return m_active;
}

bool KeyboardSimulator::shouldExit() const {
    return m_shouldExit;
}

void KeyboardSimulator::simulateKeyInput(char key) {
#ifdef _WIN32
    INPUT input[2] = {};
    
    // 按下键
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = key;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;
    
    // 释放键
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = key;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    input[1].ki.time = 0;
    input[1].ki.dwExtraInfo = 0;
    
    SendInput(2, input, sizeof(INPUT));
#elif __linux__
    try {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        if (!m_display) {
            return;
        }
        
        // 对于Linux，使用XTestFakeKeyEvent来模拟按键
        // 首先尝试将字符转换为KeySym
        KeySym keysym = NoSymbol;
        
        // 对于可打印ASCII字符（32-126），直接使用字符值作为KeySym
        if (key >= 32 && key <= 126) {
            keysym = static_cast<KeySym>(key);
        } else {
            // 对于特殊字符，尝试查找对应的KeySym
            // 注意：XStringToKeysym需要字符串名称，对于单个字符可能不适用
            // 所以对于特殊字符，我们直接使用字符值
            keysym = static_cast<KeySym>(key);
        }
        
        // 获取键码
        KeyCode keycode = XKeysymToKeycode(m_display, keysym);
        
        if (keycode != 0) {
            // 按下键
            XTestFakeKeyEvent(m_display, keycode, True, 0);
            XFlush(m_display);
            
            // 释放键
            XTestFakeKeyEvent(m_display, keycode, False, 0);
            XFlush(m_display);
        }
    } catch (...) {
        // 忽略X11操作中的异常，避免程序崩溃
    }
#endif
}

void KeyboardSimulator::simulateStringInput(const std::string& text) {
    for (char c : text) {
        simulateKeyInput(c);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 字符间小延迟
    }
}

void KeyboardSimulator::inputMonitorThread() {
    while (m_running && !m_shouldExit) {
        // 检查ESC键
        if (isEscKeyPressed()) {
            m_shouldExit = true;
            m_running = false;
            std::cout << "\n检测到ESC键，退出程序..." << std::endl;
            break;
        }
        
        // 检查鼠标左键
#ifdef _WIN32
        DWORD currentLeftMouseState = GetAsyncKeyState(VK_LBUTTON);
        bool leftWasPressed = (m_lastLeftMouseState & 0x8000) != 0;
        bool leftIsPressed = (currentLeftMouseState & 0x8000) != 0;
#elif __linux__
        bool currentLeftMouseState = isMouseLeftButtonClicked();
        bool leftWasPressed = m_lastLeftMouseState;
        bool leftIsPressed = currentLeftMouseState;
#endif
        
        // 如果鼠标左键刚被点击（从按下到释放）
        if (leftWasPressed && !leftIsPressed) {
            if (!m_active) {
                // 首次激活
                m_active = true;
                m_paused = false;
                std::cout << "检测到鼠标左键点击，开始输入..." << std::endl;
                
                // 启动输入线程
                if (!m_inputThread.joinable()) {
                    m_inputThread = std::thread(&KeyboardSimulator::inputThread, this);
                }
            } else if (m_paused) {
                // 恢复输入
                m_paused = false;
                std::cout << "检测到鼠标左键点击，恢复输入..." << std::endl;
            }
        }
        
        // 检查鼠标右键
#ifdef _WIN32
        DWORD currentRightMouseState = GetAsyncKeyState(VK_RBUTTON);
        bool rightWasPressed = (m_lastRightMouseState & 0x8000) != 0;
        bool rightIsPressed = (currentRightMouseState & 0x8000) != 0;
#elif __linux__
        bool currentRightMouseState = isMouseRightButtonClicked();
        bool rightWasPressed = m_lastRightMouseState;
        bool rightIsPressed = currentRightMouseState;
#endif
        
        // 如果鼠标右键刚被点击（从按下到释放）
        if (rightWasPressed && !rightIsPressed && m_active && !m_paused) {
            m_paused = true;
            std::cout << "检测到鼠标右键点击，暂停输入..." << std::endl;
        }
        
        m_lastLeftMouseState = currentLeftMouseState;
        m_lastRightMouseState = currentRightMouseState;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10ms检查间隔
    }
}

void KeyboardSimulator::inputThread() {
    while (m_running && m_active && !m_shouldExit) {
        // 如果暂停，等待恢复
        if (m_paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        
        if (!m_inputTexts.empty() && m_inputDelayMs > 0) {
            // 随机选择一个字符组
            std::string inputText = getRandomInputText();
            
            if (!inputText.empty()) {
                size_t textLength = inputText.length();
                
                // 计算每个字符之间的间隔时间（毫秒）
                // 将整个输入周期平均分配给每个字符
                // 例如：100ms周期，4个字符 -> 每个字符间隔25ms
                int charInterval = m_inputDelayMs / static_cast<int>(textLength);
                
                // 如果字符数大于延迟时间，至少间隔1ms
                if (charInterval < 1) {
                    charInterval = 1;
                }
                
                // 记录周期开始时间
                auto cycleStart = std::chrono::steady_clock::now();
                
                // 输入每个字符，字符之间均匀分配时间
                for (size_t i = 0; i < textLength; i++) {
                    // 检查是否暂停或退出
                    if (m_paused || !m_running || m_shouldExit) {
                        break;
                    }
                    
                    // 输入字符
                    simulateKeyInput(inputText[i]);
                    
                    // 如果不是最后一个字符，等待间隔时间
                    if (i < textLength - 1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(charInterval));
                    }
                }
                
                // 计算已用时间
                auto cycleEnd = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(cycleEnd - cycleStart).count();
                
                // 如果还有剩余时间，等待到周期结束
                int remainingTime = m_inputDelayMs - static_cast<int>(elapsed);
                if (remainingTime > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(remainingTime));
                }
            }
        } else if (m_inputTexts.empty()) {
            // 如果没有输入内容，等待一小段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            // 如果延迟为0，直接输入（随机选择一个字符组）
            std::string inputText = getRandomInputText();
            if (!inputText.empty()) {
                simulateStringInput(inputText);
            }
        }
    }
}

bool KeyboardSimulator::isMouseLeftButtonClicked() {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
#elif __linux__
    try {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        if (!m_display) {
            return false;
        }
        
        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        
        if (XQueryPointer(m_display, DefaultRootWindow(m_display), 
                          &root, &child, &root_x, &root_y, 
                          &win_x, &win_y, &mask)) {
            return (mask & Button1Mask) != 0;
        }
    } catch (...) {
        // 忽略X11操作中的异常
    }
    return false;
#endif
}

bool KeyboardSimulator::isMouseRightButtonClicked() {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
#elif __linux__
    try {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        if (!m_display) {
            return false;
        }
        
        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        
        if (XQueryPointer(m_display, DefaultRootWindow(m_display), 
                          &root, &child, &root_x, &root_y, 
                          &win_x, &win_y, &mask)) {
            return (mask & Button3Mask) != 0;
        }
    } catch (...) {
        // 忽略X11操作中的异常
    }
    return false;
#endif
}

bool KeyboardSimulator::isEscKeyPressed() {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
#elif __linux__
    try {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        if (!m_display) {
            return false;
        }
        
        char keys[32];
        KeyCode keycode = XKeysymToKeycode(m_display, XK_Escape);
        if (keycode == 0) {
            return false;
        }
        
        XQueryKeymap(m_display, keys);
        return (keys[keycode / 8] & (1 << (keycode % 8))) != 0;
    } catch (...) {
        // 忽略X11操作中的异常
    }
    return false;
#endif
}

