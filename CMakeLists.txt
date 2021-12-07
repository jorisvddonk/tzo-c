cmake_minimum_required(VERSION 3.11)

project(Tzo VERSION 1.0
                  DESCRIPTION "Tzo runtime VM"
                  LANGUAGES C)

add_library(tzo STATIC tzo.c)

add_executable(main main.c)
target_link_libraries(main PUBLIC tzo)

add_executable(test test.c)
target_link_libraries(test PUBLIC tzo)

# needs dos-like:
#add_executable(questmark questmark.c)
#target_link_libraries(questmark PUBLIC tzo)

# needs dos-like, libcurl, lode-png and physfs
#add_executable(convo convo.c)
#target_link_libraries(convo PUBLIC tzo)