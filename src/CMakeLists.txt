cmake_minimum_required (VERSION 3.6)

add_executable(inflmax optionparser.cpp
                       optionparser.h
                       main.cpp
                    )
include_directories("${PROJECT_SOURCE_DIR}")
