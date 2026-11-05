file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/fixtures" LANGTEXT_FIXTURES_PATH)

add_executable(langtext_tests test_main.cpp)
target_link_libraries(langtext_tests PRIVATE langtext_core)
langtext_target_options(langtext_tests)
target_compile_definitions(langtext_tests PRIVATE
    LANGTEXT_FIXTURES_DIR="${LANGTEXT_FIXTURES_PATH}")

add_test(NAME langtext_unit COMMAND langtext_tests)
