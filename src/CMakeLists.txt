# 添加子目录
add_subdirectory(core)
add_subdirectory(libc)
add_subdirectory(repl)
add_subdirectory(utils)

# 创建一个总的mquickjs库
add_library(mquickjs_lib
    ${CORE_SOURCES}
    ${LIBC_SOURCES}
    ${UTILS_SOURCES}
)

# 确保在编译核心源码前生成头文件
add_dependencies(mquickjs_lib generate_headers)

target_include_directories(mquickjs_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/libc
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# 添加生成的头文件目录
target_include_directories(mquickjs_lib PRIVATE
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_SOURCE_DIR}/tools  # 添加tools目录到包含路径
)

# 创建mqjs可执行文件
add_executable(mqjs ${REPL_SOURCES})

# 链接库到mqjs
target_link_libraries(mqjs mquickjs_lib m)

# 确保在编译mqjs前生成头文件
add_dependencies(mqjs generate_headers)

# 为mqjs可执行文件添加生成的头文件目录
target_include_directories(mqjs PRIVATE
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_SOURCE_DIR}/tools  # 添加tools目录到包含路径
)

# 设置头文件目录
set(MQUICKJS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/libc
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    PARENT_SCOPE
)