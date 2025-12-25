# 定义core模块的源文件
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mquickjs.c
    PARENT_SCOPE
)

# 安装头文件
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/mquickjs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mquickjs_opcode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mquickjs_priv.h
    ${CMAKE_BINARY_DIR}/generated/mquickjs_atom.h
    DESTINATION include
)
