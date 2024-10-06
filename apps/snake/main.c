#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#define WIDTH 512
#define HEIGHT 512

#define SNAKE_DIR_BUFF_SIZE 4

#define SRC_FILE(file) SRC_FOLDER file

static char* read_file(const char* path) {
  FILE* file = fopen(path, "r");

  fseek(file, 0, SEEK_END);
  long length = ftell(file);

  char* content = (char*) malloc(length);
  assert(content != 0 && "Failed to allocate memory for file read!");

  fseek(file, 0, SEEK_SET);
  length = fread(content, 1, length, file);
  content[length] = '\0';

  fclose(file);

  return content;
}

static GLuint create_sh(const char* file, GLenum type) {
  GLuint shader = glCreateShader(type);
  
  GLchar* src = read_file(file);
  glShaderSource(shader, 1, &src, NULL);

  glCompileShader(shader);
  free((void*) src);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    GLchar* message = (GLchar*) malloc(length);
    assert(message != 0 && "Failed to allocate memory for shader compile status message!");

    glGetShaderInfoLog(shader, length, NULL, message);

    fprintf(stderr, "Failed to compile shader: %s\n", message);
    free((void*) message);

    glDeleteShader(shader);
    
    assert(0);
  }

  return shader;
}

static GLuint create_prog(const char* vertex_file, const char* fragment_file) {
  GLuint vertex = create_sh(vertex_file, GL_VERTEX_SHADER);
  GLuint fragment = create_sh(fragment_file, GL_FRAGMENT_SHADER);

  GLuint program = glCreateProgram();

  glAttachShader(program, vertex);
  glAttachShader(program, fragment);

  glLinkProgram(program);

  glDetachShader(program, vertex);
  glDetachShader(program, fragment);

  glDeleteShader(vertex);
  glDeleteShader(fragment);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);

  if (status != GL_TRUE) {
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    GLchar* message = (GLchar*) malloc(length);
    assert(message != 0 && "Failed to allocate memory for program link status message!");

    glGetProgramInfoLog(program, length, NULL, message);

    fprintf(stderr, "Failed to link program: %s\n", message);
    free((void*) message);

    glDeleteProgram(program);

    assert(0);
  }

  return program;
}

static int clamp(int x, int a, int b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

typedef struct cell_t {
  int x, y;
};

enum key_status_t {
  KEY_STATUS_PRESSED = (1 << 0),
  KEY_STATUS_JUST_PRESSED = (1 << 1),
  KEY_STATUS_JUST_RELEASED = (1 << 2)
};

typedef struct window_info_t {
  enum key_status_t keys[GLFW_KEY_LAST + 1];
};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  struct window_info_t* info = (struct window_info_t*) glfwGetWindowUserPointer(window);
  
  switch (action) {
  case GLFW_PRESS:
    if ((info->keys[key] & KEY_STATUS_PRESSED) == 0) {
      info->keys[key] |= KEY_STATUS_JUST_PRESSED;
      info->keys[key] |= KEY_STATUS_PRESSED;
    }
    break;
  case GLFW_RELEASE:
    if ((info->keys[key] & KEY_STATUS_PRESSED) != 0) {
      info->keys[key] |= KEY_STATUS_JUST_RELEASED;
      info->keys[key] &= ~KEY_STATUS_PRESSED;
    }
    break;
  }
}

int main(int argc, const char** argv) {
  srand(time(NULL));

  int i, j;

  assert(glfwInit() != GLFW_FALSE && "Failed to initialize GLFW!");

  struct GLFWwindow* window = glfwCreateWindow(512, 512, "Snake", NULL, NULL);
  assert(window != NULL && "Failed to create GLFW window!");

  struct window_info_t window_info = { 0 };

  glfwSetWindowUserPointer(window, (void*) &window_info);
  glfwSetKeyCallback(window, key_callback);

  glfwMakeContextCurrent(window);
  assert(gladLoadGL((GLADloadfunc) glfwGetProcAddress) != 0 && "Failed to load OpenGL!");

  glfwSwapInterval(1);

  GLuint program = create_prog(SRC_FILE("shader.vert"), SRC_FILE("shader.frag"));

  unsigned int* lines = (unsigned int*) malloc(sizeof(unsigned int) * 32);
  assert(lines != 0 && "Failed to allocate lines!");

  for (i = 0; i < 32; i++)
    lines[i] = 0;

  struct cell_t* snake = (struct cell_t*) malloc(sizeof(struct cell_t) * 32 * 32);
  assert(snake != 0 && "Failed to allocate snake!");
  
  int snake_length = 4;
  float snake_speed = 7.5f;
  struct cell_t snake_dir[SNAKE_DIR_BUFF_SIZE] = { 0 };
  for (i = 0; i < SNAKE_DIR_BUFF_SIZE; i++) {
    snake_dir[i].x = 0;
    snake_dir[i].y = 1;
  }
  int cur_snake_dir = 0, snake_dir_top = 0;

  struct cell_t food = { rand() % 32, rand() % 32 };

  snake[0].x = 0; snake[0].y = 3;
  snake[1].x = 0; snake[1].y = 2;
  snake[2].x = 0; snake[2].y = 1;
  snake[3].x = 0; snake[3].y = 0;

  float last_time = (float) glfwGetTime();
  while (glfwWindowShouldClose(window) == 0) {
    for (i = 0; i < 32; i++)
      lines[i] = 0;

    for (i = 0; i < snake_length; i++)
      lines[snake[i].y] |= (1 << snake[i].x);

    lines[food.y] |= (1 << food.x);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    GLint u_size = glGetUniformLocation(program, "u_size");
    assert(u_size != -1 && "Failed to find uniform u_size!");

    GLint u_lines = glGetUniformLocation(program, "u_lines");
    assert(u_lines != -1 && "Failed to find uniform u_lines!");

    glUniform2ui(u_size, WIDTH, HEIGHT);
    glUniform1uiv(u_lines, 32, lines);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);

    for (i = 0; i <= GLFW_KEY_LAST; i++) {
      window_info.keys[i] &= ~KEY_STATUS_JUST_PRESSED;
      window_info.keys[i] &= ~KEY_STATUS_JUST_RELEASED;
    }

    glfwPollEvents();

    if ((window_info.keys[GLFW_KEY_D] & KEY_STATUS_JUST_PRESSED) && snake_dir[snake_dir_top].x == 0) {
      if (++snake_dir_top >= SNAKE_DIR_BUFF_SIZE)
        snake_dir_top = 0;

      snake_dir[snake_dir_top].x = 1;
      snake_dir[snake_dir_top].y = 0;
    }

    if ((window_info.keys[GLFW_KEY_A] & KEY_STATUS_JUST_PRESSED) && snake_dir[snake_dir_top].x == 0) {
      if (++snake_dir_top >= SNAKE_DIR_BUFF_SIZE)
        snake_dir_top = 0;

      snake_dir[snake_dir_top].x = -1;
      snake_dir[snake_dir_top].y = 0;
    }

    if ((window_info.keys[GLFW_KEY_W] & KEY_STATUS_JUST_PRESSED) && snake_dir[snake_dir_top].y == 0) {
      if (++snake_dir_top >= SNAKE_DIR_BUFF_SIZE)
        snake_dir_top = 0;

      snake_dir[snake_dir_top].x = 0;
      snake_dir[snake_dir_top].y = 1;
    }

    if ((window_info.keys[GLFW_KEY_S] & KEY_STATUS_JUST_PRESSED) && snake_dir[snake_dir_top].y == 0) {
      if (++snake_dir_top >= SNAKE_DIR_BUFF_SIZE)
        snake_dir_top = 0;

      snake_dir[snake_dir_top].x = 0;
      snake_dir[snake_dir_top].y = -1;
    }

    float time = glfwGetTime();
    if (((time - last_time) * snake_speed) >= 1.0f) {
      last_time = time;

      for (i = snake_length - 1; i >= 1; i--)
        snake[i] = snake[i - 1];

      snake[0].x += snake_dir[cur_snake_dir].x;
      snake[0].y += snake_dir[cur_snake_dir].y;

      if (cur_snake_dir != snake_dir_top) {
        if (++cur_snake_dir >= SNAKE_DIR_BUFF_SIZE)
          cur_snake_dir = 0;
      }

      if (snake[0].x == food.x && snake[0].y == food.y) {
        snake[snake_length] = snake[snake_length - 1];
        snake_length++;
        
        do {
          food.y = rand() % 32;
        } while (lines[food.y] == UINT32_MAX);

        do {
          food.x = rand() % 32;
        } while (lines[food.y] & (1 << food.x));
      }

      int game_over = 0;
      if (snake[0].x < 0 || snake[0].x >= 32 || snake[0].y < 0 || snake[0].y >= 32)
        game_over = 1;

      for (i = 1; i < snake_length; i++)
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
          game_over = 1;

      if (game_over) {
        printf("Game over!\n");
        printf("Score: %d\n", snake_length);
        glfwSetWindowShouldClose(window, 1);
      }
    }
  }

  free((void*) snake);
  free((void*) lines);

  glDeleteProgram(program);

  glfwDestroyWindow(window);
  glfwTerminate();
}
