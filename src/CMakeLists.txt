# startrace/src/CMakeLists.txt
# 递归查找 src 目录下所有的源文件
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# 生成核心静态库 (或者 SHARED 动态库)
add_library(startrace_core STATIC ${CORE_SOURCES})

# 【关键修复】暴露头文件目录
# PUBLIC 表示：
# 1. startrace_core 自己编译时，能在当前目录找头文件
# 2. 任何 target_link_libraries 链接了 startrace_core 的目标，也能在当前目录找头文件
target_include_directories(startrace_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${CMAKE_CURRENT_SOURCE_DIR}/signal/common
)