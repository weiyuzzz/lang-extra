set(LANGTEXT_TINYXML_DIR "${LANGTEXT_VENDOR_DIR}/tinyxml")

add_library(langtext_core STATIC
    src/Model.cpp
    src/Settings.cpp
    src/Xml.cpp
    src/Scanner.cpp
    src/Filter.cpp
    src/Extract.cpp
    src/TableWriter.cpp
    "${LANGTEXT_TINYXML_DIR}/tinystr.cpp"
    "${LANGTEXT_TINYXML_DIR}/tinyxml.cpp"
    "${LANGTEXT_TINYXML_DIR}/tinyxmlerror.cpp"
    "${LANGTEXT_TINYXML_DIR}/tinyxmlparser.cpp"
)

target_include_directories(langtext_core
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${LANGTEXT_TINYXML_DIR}"
)
target_compile_features(langtext_core PUBLIC cxx_std_17)
langtext_target_options(langtext_core)

if(LANGTEXT_WITH_XLSX)
    target_link_libraries(langtext_core PRIVATE xlsxwriter)
    target_include_directories(langtext_core PRIVATE
        "${LANGTEXT_VENDOR_DIR}/libxlsxwriter/include")
endif()
