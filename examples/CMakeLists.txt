# 查找父级作用域中的头文件目录
get_property(MQUICKJS_INCLUDE_DIRS DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY MQUICKJS_INCLUDE_DIRS)


# 添加示例可执行文件
add_executable(example 
    ${CMAKE_CURRENT_SOURCE_DIR}/example.c
)

# 链接mquickjs库到example
target_link_libraries(example mquickjs_lib m)
target_include_directories(example PRIVATE 
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/libc
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils
)