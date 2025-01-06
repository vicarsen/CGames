#include "io.h"
#include "log.h"

#include <glad/gl.h>

void window_create(window_t *window)
{
  ASSERT(glfwInit() != 0, "Failed to initialize GLFW!");

  window->window = glfwCreateWindow(1920, 1080, "Block Game", NULL, NULL);
  ASSERT(window != NULL, "Failed to create GLFW window!");

  glfwMakeContextCurrent(window->window);

  ASSERT(gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0, "Failed to load OpenGL!");
}

void window_destroy(window_t *window)
{
  if (window->window != NULL) {
    glfwDestroyWindow(window->window);
    window->window = NULL;
  }

  glfwTerminate();
}

