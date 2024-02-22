find_package(Catch2 3 REQUIRED)

add_executable(tests test.cpp)

message("building yaml tests..")

target_link_libraries(tests 
    PRIVATE
        yaml::yaml
        Catch2::Catch2WithMain
)

target_compile_features(tests PUBLIC cxx_std_23)

target_compile_options(tests PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

include(CTest)
include(Catch)
catch_discover_tests(tests)
enable_testing()