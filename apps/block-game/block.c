#include "block.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

void block_registry_load(block_registry_t *registry, char const *filepath)
{
  FILE *file = fopen(filepath, "r");
  ASSERT(file != NULL, "Failed to open file!");

  

  fclose(file);
}

void block_registry_destroy(block_registry_t *registry)
{
  free(registry->faces);
  free(registry->blocks);

  registry->faces = NULL;
  registry->blocks = NULL;

  registry->face_count = 0;
  registry->block_count = 0;
}

face_t block_registry_get_face(block_registry_t const *registry, block_t block, face_direction_t direction)
{
  return registry->blocks[block].faces[direction];
}

bool block_registry_is_face_transparent(block_registry_t const *registry, face_t face)
{
  return registry->faces[face].transparent;
}

