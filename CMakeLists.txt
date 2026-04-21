# StarTrace/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)

# 定义项目名称、版本和使用的语言
project(StarTrace VERSION 0.1.0 LANGUAGES CXX)

# 设置全局 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 开启 CTest 测试支持
enable_testing()

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

add_executable(product_sig main.cpp)
target_link_libraries(product_sig startrace_core)