# 定义utils模块的源文件
set(UTILS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/cutils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/readline.c
    ${CMAKE_CURRENT_SOURCE_DIR}/readline_tty.c
    PARENT_SCOPE
)

# 安装头文件
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/cutils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/list.h
    ${CMAKE_CURRENT_SOURCE_DIR}/readline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/readline_tty.h
    ${CMAKE_CURRENT_SOURCE_DIR}/softfp_template.h
    ${CMAKE_CURRENT_SOURCE_DIR}/softfp_template_icvt.h
    DESTINATION include
)