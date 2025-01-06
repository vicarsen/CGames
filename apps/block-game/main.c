#include "type.h"
#include "io.h"

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

