#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

typedef struct {
  GLFWwindow *window;
} window_t;

void window_create(window_t *window);
void window_destroy(window_t *window);

