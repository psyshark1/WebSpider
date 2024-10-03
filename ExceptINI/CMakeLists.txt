cmake_minimum_required (VERSION 3.8)

project(exceptini)

set(EXCEPTION Exception.h Exception.cpp)

add_library(exceptini STATIC ${EXCEPTION})

#target_compile_definitions(exceptini PRIVATE EX_DLL)