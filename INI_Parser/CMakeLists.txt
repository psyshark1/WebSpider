cmake_minimum_required (VERSION 3.8)

project(iniparser)

include_directories(INI_Parser)

set(INI INI_parser.h INI_parser.cpp)

add_library(iniparser SHARED ${INI})

target_compile_definitions(iniparser PRIVATE PARSER_DLL)

target_link_libraries(iniparser exceptini)