#pragma once

#include "type.h"

typedef byte block_t;
typedef u32 face_t;

typedef enum {
  FACE_NORTH = 0, // +x
  FACE_SOUTH, // -x
  FACE_EAST, // +z
  FACE_WEST, // -z
  FACE_UP, // +y
  FACE_DOWN, // -y
  FACE_LAST = FACE_DOWN
} face_direction_t;

typedef struct {
  bool transparent;
} face_data_t;

typedef struct {
  face_t faces[FACE_LAST + 1];
} block_data_t;

typedef struct {
  face_data_t *faces;
  usize face_count;

  block_data_t *blocks;
  usize block_count;
} block_registry_t;

void block_registry_load(block_registry_t *registry, char const *filepath);
void block_registry_destroy(block_registry_t *registry);

face_t block_registry_get_face(block_registry_t const *registry, block_t block, face_direction_t direction);
bool block_registry_is_face_transparent(block_registry_t const *registry, face_t face);

