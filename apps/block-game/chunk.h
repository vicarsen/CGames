#pragma once

#include "block.h"

#define CHUNK_SIZE 64
#define CHUNK_PADDING 1
#define CHUNK_FULL (CHUNK_SIZE + 2 * CHUNK_PADDING)

#define CHUNK_START CHUNK_PADDING
#define CHUNK_END (CHUNK_PADDING + CHUNK_SIZE)

typedef u16 packed_chunk_coord_t;

typedef struct {
  block_t blocks[CHUNK_FULL][CHUNK_FULL][CHUNK_FULL];
} chunk_t;

typedef enum {
  CHUNK_OK = 0,
  CHUNK_FILE_NOT_FOUND,
  CHUNK_FILE_WRITE_ERROR,
  CHUNK_FILE_READ_ERROR
} chunk_result_t;

typedef struct {
  byte *data;
  usize bits;
} compressed_chunk_t;

void chunk_diff(chunk_t const *from, chunk_t const *to, chunk_t *out);
void compress_chunk(chunk_t const *chunk, compressed_chunk_t *out);
void decompress_chunk(compressed_chunk_t const *chunk, chunk_t *out);

chunk_result_t chunk_write(compressed_chunk_t const *chunk, char const *filepath);
chunk_result_t chunk_read(compressed_chunk_t *chunk, char const *filepath);

typedef u32 packed_vertex_t;

typedef struct {
  packed_vertex_t *mesh[3];
  usize vertices[3];
} chunk_mesh_t;

void chunk_mesh_create(block_registry_t const *registry, chunk_t const *chunk, chunk_mesh_t *mesh);
void chunk_mesh_destroy(chunk_mesh_t *mesh);

