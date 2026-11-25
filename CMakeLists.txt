cmake_minimum_required(VERSION 3.16)
project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(${PROJECT_NAME}
    ${SOURCES} 
    resources.qrc
)




target_link_libraries(untitled PRIVATE Qt6::Widgets)
target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Widgets
)
