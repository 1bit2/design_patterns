cmake_minimum_required(VERSION 3.16)
project(DesignPatterns LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(builder creational/builder/builder.cpp)
add_executable(composite structural/composite/composite.cpp)
add_executable(bridge structural/bridge/bridge.cpp)
add_executable(mediator behavioral/mediator/mediator.cpp)
