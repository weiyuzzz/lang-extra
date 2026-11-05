# Windows GUI. DuiLib is vendored GBK-encoded MSVC code, so it is compiled as
# its own static library without /utf-8; our own gui/ sources are UTF-8 and do
# get /utf-8 through langtext_target_options().
set(DUILIB_DIR "${LANGTEXT_VENDOR_DIR}/DuiLib")

file(GLOB_RECURSE DUILIB_SOURCES CONFIGURE_DEPENDS "${DUILIB_DIR}/*.cpp")
list(FILTER DUILIB_SOURCES EXCLUDE REGEX "/Build/")

add_library(duilib_vendor STATIC ${DUILIB_SOURCES})
target_include_directories(duilib_vendor PUBLIC "${DUILIB_DIR}")
target_compile_definitions(duilib_vendor PUBLIC
    UILIB_STATIC WIN32 UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_compile_features(duilib_vendor PUBLIC cxx_std_17)
if(MSVC)
    # Vendored code predates modern warnings-as-errors hygiene.
    target_compile_options(duilib_vendor PRIVATE /W1 /D_HAS_STD_BYTE=0)
endif()
target_link_libraries(duilib_vendor PUBLIC
    comctl32 comdlg32 gdi32 gdiplus imm32 msimg32 ole32 oleaut32
    shell32 shlwapi user32 uuid winmm)

add_executable(langtext_gui WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    WinUtils.cpp
    WinUtils.h
)
target_link_libraries(langtext_gui PRIVATE langtext_core duilib_vendor shell32 comdlg32 ole32)
langtext_target_options(langtext_gui)
set_target_properties(langtext_gui PROPERTIES OUTPUT_NAME "langTextExtra")

# Runtime assets must sit next to the executable: the GUI resolves skins and
# settings relative to the exe directory, not the working directory.
add_custom_command(TARGET langtext_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/settings.xml" "$<TARGET_FILE_DIR:langtext_gui>/settings.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.xml" "$<TARGET_FILE_DIR:langtext_gui>/config.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/out/skin" "$<TARGET_FILE_DIR:langtext_gui>/skin"
    COMMENT "Copying settings.xml, config.xml and skin/ next to the GUI executable"
    VERBATIM
)
