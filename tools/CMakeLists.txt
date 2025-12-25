# 工具目录的CMakeLists.txt

# 创建生成目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

# 创建构建工具 - 这需要链接mqjs_stdlib.c和mquickjs_build.c
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/mqjs_stdlib
    COMMAND ${CMAKE_C_COMPILER} -I${CMAKE_SOURCE_DIR}/src/utils -I${CMAKE_SOURCE_DIR}/src/core -I${CMAKE_SOURCE_DIR}/src/repl -I${CMAKE_SOURCE_DIR}/examples
        ${CMAKE_SOURCE_DIR}/src/repl/mqjs_stdlib.c
        ${CMAKE_SOURCE_DIR}/examples/mquickjs_build.c 
        ${CMAKE_SOURCE_DIR}/src/utils/cutils.c 
        -lm -o ${CMAKE_BINARY_DIR}/mqjs_stdlib
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building stdlib generation tool"
)

# 生成必要的头文件
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
add_custom_target(generate_headers
    DEPENDS 
        ${CMAKE_BINARY_DIR}/generated/mquickjs_atom.h
        ${CMAKE_BINARY_DIR}/generated/mqjs_stdlib.h
)

# 添加新工具目标
add_executable(mqjs_js_module_gen mqjs_js_module_gen.c)

# 设置头文件路径
target_include_directories(mqjs_js_module_gen PRIVATE
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/examples
    ${CMAKE_SOURCE_DIR}/src/libc
)

# 链接到主库
target_link_libraries(mqjs_js_module_gen mquickjs_lib m)

# 这个目录是空的，因为构建工具现在在examples目录中
# 保留此文件以满足目录结构
