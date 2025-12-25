# 查找父级作用域中的头文件目录
get_property(MQUICKJS_INCLUDE_DIRS DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY MQUICKJS_INCLUDE_DIRS)

# 创建构建工具 - 需要先构建example_stdlib工具来生成头文件
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib
    COMMAND ${CMAKE_C_COMPILER} 
        -I${CMAKE_SOURCE_DIR}/src/utils 
        -I${CMAKE_SOURCE_DIR}/src/core 
        -I${CMAKE_SOURCE_DIR}/src/repl 
        -I${CMAKE_SOURCE_DIR}/tools
        ${CMAKE_SOURCE_DIR}/examples/example_stdlib.c
        ${CMAKE_SOURCE_DIR}/tools/mquickjs_build.c 
        ${CMAKE_SOURCE_DIR}/src/utils/cutils.c 
        -lm -o ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building example stdlib generation tool"
)

# 生成example_stdlib.h头文件到build目录中
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib.h
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib > ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib
    COMMENT "Generating example_stdlib.h"
)

# 添加一个自定义目标来确保头文件被生成
add_custom_target(generate_example_headers
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/example_stdlib.h
)

# 添加示例可执行文件
add_executable(example 
    ${CMAKE_CURRENT_SOURCE_DIR}/example.c
)

# 确保在编译example前生成头文件
add_dependencies(example generate_example_headers)

# 链接mquickjs库到example
target_link_libraries(example mquickjs_lib m)
target_include_directories(example PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}  # 添加生成的头文件目录
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/libc
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools  # 添加tools目录到包含路径
)