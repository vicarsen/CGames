#include "type.h"
#include "io.h"
#include "log.h"
#include "chunk.h"

#include <stdlib.h>

#include <glad/gl.h>

typedef struct {
  window_t window;
} game_state_t;

static void init(game_state_t *state);
static void run(game_state_t *state);
static void terminate(game_state_t *state, i32 code);

static void init(game_state_t *state)
{
  window_create(&state->window);
  
  chunk_t *chunk = malloc(sizeof(chunk_t));

  for (byte x = 0; x <= CHUNK_SIZE; x++) {
    for (byte y = 0; y <= CHUNK_SIZE; y++) {
      for (byte z = 0; z <= CHUNK_SIZE; z++) {
        chunk->blocks[x][y][z] = rand() % 3;
      }
    }
  }

  compressed_chunk_t compressed;

  chunk_t *chunk_copy = malloc(sizeof(chunk_t));
  chunk_read(&compressed, "chunk.bin");
  decompress_chunk(&compressed, chunk_copy);

  free(compressed.data);

  bool eq = TRUE;
  for (byte x = 0; x <= CHUNK_SIZE; x++) {
    for (byte y = 0; y <= CHUNK_SIZE; y++) {
      for (byte z = 0; z <= CHUNK_SIZE; z++) {
        if (chunk->blocks[x][y][z] != chunk_copy->blocks[x][y][z]) {
          eq = FALSE;
        }
      }
    }
  }

  if (eq) {
    INFO("Chunks are equal!");
  } else {
    INFO("Chunks are different!");
  }

  free(chunk);
  free(chunk_copy);
}

static void update(game_state_t *state, f32 delta_time);
static void render(game_state_t *state);

static void run(game_state_t *state)
{
  f32 prev_time = glfwGetTime();
  while (!glfwWindowShouldClose(state->window.window)) {
    glfwPollEvents();

    f32 current_time = glfwGetTime();
    f32 delta_time = current_time - prev_time;
    prev_time = current_time;

    update(state, delta_time);

    glClearColor(0.1f, 0.2f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    render(state);

    glfwSwapBuffers(state->window.window);
  }
}

static void update(game_state_t *state, f32 delta_time)
{
}

static void render(game_state_t *state)
{
}

static void terminate(game_state_t *state, i32 code)
{
  window_destroy(&state->window);
  exit(code);
}

i32 main(void)
{
  game_state_t game_state;

  init(&game_state);
  run(&game_state);
  terminate(&game_state, EXIT_SUCCESS);
}

