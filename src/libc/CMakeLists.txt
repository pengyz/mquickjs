# 定义libc模块的源文件
set(LIBC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/libm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dtoa.c
    PARENT_SCOPE
)

# 安装头文件
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/libm.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dtoa.h
    DESTINATION include
)