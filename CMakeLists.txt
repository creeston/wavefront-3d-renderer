cmake_minimum_required(VERSION 3.10)
project(My3DApp C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

add_executable(app main.c gtk_gui.c canvas_drawing.c renderer.c)
target_link_libraries(app ${GTK3_LIBRARIES} m)
