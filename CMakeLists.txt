cmake_minimum_required(VERSION 3.16)
project(DesignPatterns LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(builder creational/builder/builder.cpp)
add_executable(composite structural/composite/composite.cpp)
add_executable(bridge structural/bridge/bridge.cpp)
add_executable(mediator behavioral/mediator/mediator.cpp)
add_executable(iterator_demo behavioral/iterator/iterator_storage_demo.cpp)
add_executable(memento behavioral/memento/memento.cpp)
add_executable(visitor behavioral/visitor/visitor.cpp)
add_executable(command behavioral/command/command.cpp)
