# 定义repl模块的源文件
set(REPL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mqjs.c
    PARENT_SCOPE
)

# 安装头文件
install(FILES 
    ${CMAKE_BINARY_DIR}/generated/mqjs_stdlib.h
    DESTINATION include
)