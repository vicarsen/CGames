
add_executable(
  blockgame

  type.h

  log.h
  log.c

  io.h
  io.c

  block.h
  block.c

  chunk.h
  chunk.c

  main.c
)

target_link_libraries(blockgame PRIVATE glfw glad)
target_compile_definitions(blockgame PRIVATE "SRC_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}/\"")

