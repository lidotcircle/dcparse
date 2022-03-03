
file(GLOB_RECURSE cparser_SOURCES CONFIGURE_DEPENDS ./lib/**.cpp)
add_library(cparser STATIC ${cparser_SOURCES})
set_property(TARGET cparser PROPERTY CXX_STANDARD 17)
target_include_directories(cparser PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(cparser private dcparse)

