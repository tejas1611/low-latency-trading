cmake_minimum_required(VERSION 3.0)

project(LowLatencyApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

file(GLOB SOURCES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR})

add_library(libcommon STATIC ${SOURCES})

list(APPEND LIBS libcommon)
list(APPEND LIBS pthread)

# add_executable(thread_example thread_example.cpp)
# target_link_libraries(thread_example PUBLIC ${LIBS})

add_executable(mempool_example mempool_example.cpp)
target_link_libraries(mempool_example PUBLIC ${LIBS})

add_executable(lf_queue_example lf_queue_example.cpp)
target_link_libraries(lf_queue_example PUBLIC ${LIBS})

add_executable(logger_example logger_example.cpp)
target_link_libraries(logger_example PUBLIC ${LIBS})

# add_executable(socket_example socket_example.cpp)
# target_link_libraries(socket_example PUBLIC ${LIBS})