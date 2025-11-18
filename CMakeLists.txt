cmake_minimum_required(VERSION 3.10)
project(KeyboardStressTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置UTF-8编码支持（MSVC）
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    keyboard_simulator.cpp
    keyboard_simulator.h
)

# Windows特定设置
if(WIN32)
    # 链接Windows库
    target_link_libraries(${PROJECT_NAME} 
        PRIVATE 
        user32
    )
    
    # 设置Windows子系统（如果需要控制台窗口）
    # set_target_properties(${PROJECT_NAME} PROPERTIES
    #     WIN32_EXECUTABLE FALSE
    # )
endif()

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

