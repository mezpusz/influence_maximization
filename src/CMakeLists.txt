cmake_minimum_required (VERSION 3.6)

add_executable(inflmax inputparser.cpp
                       inputparser.h
                       main.cpp
                       node.h
                       optionparser.cpp
                       optionparser.h
                    )
include_directories("${PROJECT_SOURCE_DIR}")
