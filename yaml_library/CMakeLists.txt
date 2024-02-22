add_library(yaml INTERFACE)
add_library(yaml::yaml ALIAS yaml)

target_compile_features(yaml INTERFACE cxx_std_23)

target_compile_options(yaml INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_include_directories(yaml INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)