cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
include($ENV{TAPASCO_HOME}/cmake/Tapasco.cmake NO_POLICY_SCOPE)
project (benchmark-latency)

find_package(TapascoTLKM REQUIRED)
find_package(TapascoCommon REQUIRED)
find_package(TapascoPlatform REQUIRED)
find_package(Tapasco REQUIRED)

add_executable(benchmark-latency benchmark-latency.c)
set_tapasco_defaults(benchmark-latency)
target_link_libraries(benchmark-latency rt pthread tapasco platform atomic)

add_executable(benchmark-latency++ benchmark-latency.cpp)
set_tapasco_defaults(benchmark-latency++)
target_link_libraries(benchmark-latency++ m pthread  atomic tapasco platform atomic)

install(TARGETS benchmark-latency
        ARCHIVE  DESTINATION share/Tapasco/bin/
        LIBRARY  DESTINATION share/Tapasco/bin/
        RUNTIME  DESTINATION share/Tapasco/bin/)
