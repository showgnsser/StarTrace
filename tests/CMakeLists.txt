# startrace/tests/CMakeLists.txt

# 查找 GTest 包 (通常通过系统的包管理器安装，或者 FetchContent 获取)
find_package(GTest REQUIRED)

# 查找所有的测试源文件
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# 生成测试可执行文件
add_executable(startrace_tests ${TEST_SOURCES})

# 链接核心库和 GTest
# 因为链接了 startrace_core，CMake 会自动把 startrace/src 加入到 startrace_tests 的 Include 路径中！
target_link_libraries(startrace_tests PRIVATE
        startrace_core
        GTest::gtest_main
)

# 注册测试，让 CTest 能够识别
include(GoogleTest)
gtest_discover_tests(startrace_tests)