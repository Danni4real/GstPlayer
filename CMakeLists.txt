cmake_minimum_required(VERSION 3.5)

project(MediaPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("/usr/include/glib-2.0"
                    "/usr/include/gstreamer-1.0"
                    "/usr/lib/x86_64-linux-gnu/glib-2.0/include")

add_executable(client tools.cpp
                      client.cpp
                      socket.cpp
                      player.cpp)

add_executable(server tools.cpp
                      server.cpp
                      socket.cpp
                      player.cpp)

target_link_libraries(client pthread ${GST_VIDEO_LIBRARIES})
target_link_libraries(server pthread ${GST_VIDEO_LIBRARIES})
