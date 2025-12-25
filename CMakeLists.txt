cmake_minimum_required(VERSION 3.1)
project(mquickjs)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 检查编译器
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(COMPILER_GCC_CLANG 1)
endif()

# 编译器特定的标志
if(COMPILER_GCC_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -MMD -D_GNU_SOURCE -fno-math-errno -fno-trapping-math")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
    # 在嵌入式系统中使用-fsingle-precision-constant
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsingle-precision-constant")
endif()

# 检查架构
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TARGET_BITS 64)
else()
    set(TARGET_BITS 32)
endif()

# 调试/优化标志
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
endif()

# 添加源码目录
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tools)

# 安装规则
install(TARGETS mqjs example mqjs_js_module_gen
    RUNTIME DESTINATION bin)
