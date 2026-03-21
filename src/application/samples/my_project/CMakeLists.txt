# 收集所有源文件
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/meeting.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_mainstart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lcd_ili9341.c
    ${CMAKE_CURRENT_SOURCE_DIR}/images/User.c
    ${CMAKE_CURRENT_SOURCE_DIR}/images/Man.c
    ${CMAKE_CURRENT_SOURCE_DIR}/images/Woman.c
)

# 头文件目录
set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}
    # LVGL 头文件路径，让你的代码能找到 lvgl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../open_source/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../open_source/lvgl/src
)

# 向上传递给父级 CMake
set(SOURCES       "${SOURCES}"        PARENT_SCOPE)
set(PUBLIC_HEADER "${PUBLIC_HEADER}"  PARENT_SCOPE)