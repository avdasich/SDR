cmake_minimum_required(VERSION 3.16)
project(PlutoLink C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SoapySDR REQUIRED)

add_executable(sdr main.c)
target_include_directories(sdr PRIVATE ${SoapySDR_INCLUDE_DIRS})
target_link_libraries(sdr PRIVATE ${SoapySDR_LIBRARIES} m)