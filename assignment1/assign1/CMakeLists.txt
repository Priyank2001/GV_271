cmake_minimum_required(VERSION 3.10)
project(GV271_MeshViewer C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# freeglut has no standard CMake config; find it manually
find_path(FREEGLUT_INCLUDE_DIR GL/freeglut.h)
find_library(FREEGLUT_LIBRARY glut)

if(NOT FREEGLUT_INCLUDE_DIR OR NOT FREEGLUT_LIBRARY)
    message(FATAL_ERROR "freeglut not found — install with: sudo apt install freeglut3-dev")
endif()

set(SOURCES
    main.cpp
)

add_executable(mesh_viewer ${SOURCES})

target_include_directories(mesh_viewer PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${FREEGLUT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_link_libraries(mesh_viewer PRIVATE
    OpenGL::GL
    GLEW::GLEW
    ${FREEGLUT_LIBRARY}
)
