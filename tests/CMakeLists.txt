# --- Сборка тестов ---
file(GLOB_RECURSE TEST_SRCS CONFIGURE_DEPENDS *.cpp)

foreach(test_src ${TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name}
        PRIVATE
            fmt::fmt
            GTest::gtest
            GTest::gtest_main
            pthread
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
