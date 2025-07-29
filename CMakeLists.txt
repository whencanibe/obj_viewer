cmake_minimum_required(VERSION 3.30)
project(obj_viewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 설치 위치를 수동으로 알려줌
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGL
        OpenGLWidgets
        REQUIRED)

add_executable(obj_viewer src/main.cpp
        src/Renderer/GLWidget.cpp
        src/Renderer/GLWidget.h
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/core/ModelLoader.cpp
        src/core/ModelLoader.h
        src/res/res.qrc
)

file(COPY ${CMAKE_SOURCE_DIR}/src/res/models
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res)

include_directories(${CMAKE_SOURCE_DIR}/src/ext/glm)
include_directories(${CMAKE_SOURCE_DIR}/src/ext)


target_link_libraries(obj_viewer
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)

