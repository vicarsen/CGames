
add_executable(snake main.c)

target_link_libraries(snake PRIVATE glfw glad)
target_compile_definitions(snake PRIVATE "SRC_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}/\"")
