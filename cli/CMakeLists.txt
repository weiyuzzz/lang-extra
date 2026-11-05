add_executable(langtext_cli main.cpp)
target_link_libraries(langtext_cli PRIVATE langtext_core)
if(WIN32)
    # CommandLineToArgvW()/GetCommandLineW()/LocalFree() live in these import
    # libraries. MinGW links shell32 implicitly; MSVC does not.
    target_link_libraries(langtext_cli PRIVATE shell32 kernel32)
endif()
langtext_target_options(langtext_cli)
set_target_properties(langtext_cli PROPERTIES OUTPUT_NAME "langtext")
