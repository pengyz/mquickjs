# 工具目录的CMakeLists.txt
# 包含构建系统使用的通用工具和库

# 创建生成目录，用于存放自动生成的文件
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

# 创建mquickjs_build库，这是一个通用的构建工具库
# 用于将JavaScript代码转换为C结构体，以静态化方式集成到引擎中
add_library(mquickjs_build_tool
    ${CMAKE_SOURCE_DIR}/tools/mquickjs_build.c
)

# 设置库的包含路径
target_include_directories(mquickjs_build_tool PUBLIC
    ${CMAKE_SOURCE_DIR}/tools
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/core
)

# 创建构建工具 - 用于生成标准库头文件
# 这个工具结合了标准库定义和构建工具，生成C头文件
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/mqjs_stdlib
    COMMAND ${CMAKE_C_COMPILER} 
        -I${CMAKE_SOURCE_DIR}/src/utils 
        -I${CMAKE_SOURCE_DIR}/src/core 
        -I${CMAKE_SOURCE_DIR}/src/repl 
        -I${CMAKE_SOURCE_DIR}/tools
        ${CMAKE_SOURCE_DIR}/src/repl/mqjs_stdlib.c
        ${CMAKE_SOURCE_DIR}/tools/mquickjs_build.c 
        ${CMAKE_SOURCE_DIR}/src/utils/cutils.c 
        -lm -o ${CMAKE_BINARY_DIR}/mqjs_stdlib
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building stdlib generation tool"
)

# 生成必要的头文件
# mquickjs_atom.h - 包含引擎原子定义
# mqjs_stdlib.h - 包含标准库对象结构定义
add_custom_command(
    OUTPUT 
        ${CMAKE_BINARY_DIR}/generated/mquickjs_atom.h
        ${CMAKE_BINARY_DIR}/generated/mqjs_stdlib.h
    COMMAND ${CMAKE_BINARY_DIR}/mqjs_stdlib -a > ${CMAKE_BINARY_DIR}/generated/mquickjs_atom.h
    COMMAND ${CMAKE_BINARY_DIR}/mqjs_stdlib > ${CMAKE_BINARY_DIR}/generated/mqjs_stdlib.h
    DEPENDS ${CMAKE_BINARY_DIR}/mqjs_stdlib
    COMMENT "Generating header files"
)

# 创建一个目标来生成头文件
# 其他目标可以依赖这个目标以确保头文件已生成
add_custom_target(generate_headers
    DEPENDS 
        ${CMAKE_BINARY_DIR}/generated/mquickjs_atom.h
        ${CMAKE_BINARY_DIR}/generated/mqjs_stdlib.h
)