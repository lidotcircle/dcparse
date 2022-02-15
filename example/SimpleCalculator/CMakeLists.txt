
file(GLOB_RECURSE scalc_SOURCES CONFIGURE_DEPENDS ./src/**.cpp)
add_executable(scalc ${scalc_SOURCES})
set_property(TARGET scalc PROPERTY CXX_STANDARD 17)
target_link_libraries(scalc PRIVATE dcparse)

