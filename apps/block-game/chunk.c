#include "chunk.h"
#include "log.h"

#include <stdlib.h>

#define COORD_BITS 7

void chunk_diff(chunk_t const *from, chunk_t const *to, chunk_t *out)
{
  for (byte x = 0; x < CHUNK_FULL; x++) {
    for (byte y = 0; y < CHUNK_FULL; y++) {
      for (byte z = 0; z < CHUNK_FULL; z++) {
        if (from->blocks[x][y][z] != to->blocks[x][y][z]) {
          out->blocks[x][y][z] = to->blocks[x][y][z];
        } else {
          out->blocks[x][y][z] = 0;
        }
      }
    }
  }
}

static void __compressed_chunk_put(compressed_chunk_t *chunk, usize k, usize bits, byte v)
{
  while (bits) {
    usize i = k / 8, j = k % 8;
    chunk->data[i] |= (v & 1) << j;
    v >>= 1;

    k++;
    bits--;
  }
}

static byte __compressed_chunk_get(compressed_chunk_t const *chunk, usize k, usize bits)
{
  byte v = 0;
  for (usize pos = 0; pos < bits; pos++) {
    usize i = k / 8, j = k % 8;
    v |= ((chunk->data[i] >> j) & 1) << pos;
    
    k++;
  }
  return v;
}

static usize __compress_chunk_strip_count(block_t const *strip)
{
  usize strip_count = 1;

  for (byte z = 1; z < CHUNK_FULL; z++) {
    if (strip[z - 1] != strip[z]) {
      strip_count++;
    }
  }

  return strip_count;
}

void compress_chunk(chunk_t const *chunk, compressed_chunk_t *compressed)
{
  ASSERT(sizeof(block_t) == 1, "Only blocks of 1 byte can be compressed!");
  ASSERT(COORD_BITS <= 8, "Only 8 bit coordinates can be compressed!");
  
  usize bits = 0;
  for (byte x = 0; x < CHUNK_FULL; x++) {
    for (byte y = 0; y < CHUNK_FULL; y++) {
      usize strip_count = __compress_chunk_strip_count(chunk->blocks[x][y]);

      if (strip_count != 1) {
        bits += strip_count * (sizeof(block_t) * 8 + COORD_BITS) + 2 * COORD_BITS;
      }
    }
  }

  usize bytes = bits / 8 + (bits % 8 != 0);

  compressed->data = malloc(bytes);
  compressed->bits = 0;

  ASSERT(compressed->data != NULL, "Failed to allocate!");

  for (byte x = 0; x < CHUNK_FULL; x++) {
    for (byte y = 0; y < CHUNK_FULL; y++) {
      usize strip_count = __compress_chunk_strip_count(chunk->blocks[x][y]);

      if (strip_count != 1) {
        __compressed_chunk_put(compressed, compressed->bits, COORD_BITS, x);
        compressed->bits += COORD_BITS;

        __compressed_chunk_put(compressed, compressed->bits, COORD_BITS, y);
        compressed->bits += COORD_BITS;

        block_t const *strip = chunk->blocks[x][y];
        for (byte z = 1; z < CHUNK_FULL; z++) {
          if (strip[z - 1] != strip[z]) {
            __compressed_chunk_put(compressed, compressed->bits, sizeof(block_t) * 8, strip[z - 1]);
            compressed->bits += sizeof(block_t) * 8;

            __compressed_chunk_put(compressed, compressed->bits, COORD_BITS, z);
            compressed->bits += COORD_BITS;
          }
        }

        __compressed_chunk_put(compressed, compressed->bits, sizeof(block_t) * 8, strip[CHUNK_FULL - 1]);
        compressed->bits += sizeof(block_t) * 8;

        __compressed_chunk_put(compressed, compressed->bits, COORD_BITS, CHUNK_FULL);
        compressed->bits += COORD_BITS;
      }
    }
  }
}

void decompress_chunk(compressed_chunk_t const *chunk, chunk_t *out)
{
  ASSERT(sizeof(block_t) == 1, "Only blocks of 1 byte can be decompressed!");
  ASSERT(COORD_BITS <= 8, "Only 8 bit coordinates can be decompressed!");

  for (usize x = 0; x < CHUNK_FULL; x++) {
    for (usize y = 0; y < CHUNK_FULL; y++) {
      for (usize z = 0; z < CHUNK_FULL; z++) {
        out->blocks[x][y][z] = 0;
      }
    }
  }

  usize start = 0;
  while (start < chunk->bits) {
    byte x = __compressed_chunk_get(chunk, start, COORD_BITS);
    start += COORD_BITS;

    byte y = __compressed_chunk_get(chunk, start, COORD_BITS);
    start += COORD_BITS;

    block_t block;
    usize zs = 0, zf;

    do {
      block = __compressed_chunk_get(chunk, start, sizeof(block_t) * 8);
      start += sizeof(block_t) * 8;

      zf = __compressed_chunk_get(chunk, start, COORD_BITS);
      start += COORD_BITS;

      for (usize z = zs; z < zf; z++) {
        out->blocks[x][y][z] = block;
      }

      zs = zf;
    } while (zs != CHUNK_FULL);
  }
}

static bool __is_little_endian(void)
{
  static u16 dummy = 1;
  return *(byte *)&dummy == 1;
}

static void __swap(byte *x, byte *y)
{
  byte t = *x;
  *x = *y;
  *y = t;
}

chunk_result_t chunk_write(compressed_chunk_t const *chunk, char const *filepath)
{
  FILE *file = fopen(filepath, "wb");
  if (file == NULL) {
    return CHUNK_FILE_NOT_FOUND;
  }

  usize bits = chunk->bits;
  byte *bytes = (byte *)&bits;

  if (__is_little_endian()) {
    for (usize i = 0; i + i + 1 < sizeof(usize); i++) {
      __swap(bytes + i, bytes + sizeof(usize) - i - 1);
    }
  }

  if (fwrite(bytes, 1, sizeof(usize), file) != sizeof(usize)) {
    fclose(file);
    return CHUNK_FILE_WRITE_ERROR;
  }

  usize full_bytes = chunk->bits / 8 + (chunk->bits % 8 != 0);
  if (fwrite(chunk->data, 1, full_bytes, file) != full_bytes) {
    fclose(file);
    return CHUNK_FILE_WRITE_ERROR;
  }

  fclose(file);
  return CHUNK_OK;
}

chunk_result_t chunk_read(compressed_chunk_t *chunk, char const *filepath)
{
  FILE *file = fopen(filepath, "rb");
  if (file == NULL) {
    return CHUNK_FILE_NOT_FOUND;
  }

  usize bits;
  byte *bytes = (byte *)&bits;

  if (fread(bytes, 1, sizeof(usize), file) != sizeof(usize)) {
    fclose(file);
    return CHUNK_FILE_READ_ERROR;
  }

  if (__is_little_endian()) {
    for (usize i = 0; i + i + 1 < sizeof(usize); i++) {
      __swap(bytes + i, bytes + sizeof(usize) - i - 1);
    }
  }

  usize full_bytes = bits / 8 + (bits % 8 != 0);
  chunk->bits = bits;
  chunk->data = malloc(full_bytes);

  ASSERT(chunk->data != NULL, "Failed to allocate!");

  if (fread(chunk->data, 1, full_bytes, file) != full_bytes) {
    fclose(file);
    return CHUNK_FILE_READ_ERROR;
  }

  fclose(file);
  return CHUNK_OK;
}

typedef struct {
  face_t face;
  byte direction;
} face_attribs_t;

static bool __face_cmp(face_attribs_t x, face_attribs_t y)
{
  return x.face == y.face && x.direction == y.direction;
}

static face_attribs_t __face_at(block_registry_t const *registry, face_t front, face_t back)
{
  face_attribs_t attribs;
  
  bool front_transparent = block_registry_is_face_transparent(registry, front);
  bool back_transparent = block_registry_is_face_transparent(registry, back);

  if (front_transparent && !back_transparent) {
    attribs.face = back;
    attribs.direction = 1;
  } else if (!front_transparent && back_transparent) {
    attribs.face = front;
    attribs.direction = 0;
  } else {
    attribs.face = 0;
  }

  return attribs;
}

static face_attribs_t __face_at_x(block_registry_t const *registry,
                                  chunk_t const *chunk,
                                  byte x, byte y, byte z)
{
  face_t front = block_registry_get_face(registry, chunk->blocks[x][y][z], FACE_NORTH);
  face_t back = block_registry_get_face(registry, chunk->blocks[x + 1][y][z], FACE_SOUTH);

  return __face_at(registry, front, back);
}

static bool __can_expand_x_z(block_registry_t const *registry,
                             chunk_t const *chunk,
                             bool checked[CHUNK_FULL][CHUNK_FULL],
                             face_attribs_t target,
                             byte x, byte y, byte z)
{
  face_attribs_t face = __face_at_x(registry, chunk, x, y, z);
  return !checked[y][z] && __face_cmp(face, target);
}

static void __expand_x_z(bool checked[CHUNK_FULL][CHUNK_FULL],
                         byte x, byte y, byte z)
{
  checked[y][z] = TRUE;
}

static bool __can_expand_x_y(block_registry_t const *registry,
                             chunk_t const *chunk,
                             bool checked[CHUNK_FULL][CHUNK_FULL],
                             face_attribs_t target,
                             byte x, byte y, byte z1, byte z2)
{
  for (byte z = z1; z < z2; z++) {
    face_attribs_t face = __face_at_x(registry, chunk, x, y, z);
    if (checked[y][z] ||
        !__face_cmp(face, target)) {
      return FALSE;
    }
  }

  return TRUE;
}

static void __expand_x_y(bool checked[CHUNK_FULL][CHUNK_FULL],
                         byte x, byte y, byte z1, byte z2)
{
  for (byte z = z1; z < z2; z++) {
    checked[y][z] = TRUE;
  }
}

static face_attribs_t __face_at_y(block_registry_t const *registry,
                                  chunk_t const *chunk,
                                  byte x, byte y, byte z)
{
  face_t front = block_registry_get_face(registry, chunk->blocks[x][y][z], FACE_UP);
  face_t back = block_registry_get_face(registry, chunk->blocks[x][y + 1][z], FACE_DOWN);

  return __face_at(registry, front, back);
}

static bool __can_expand_y_z(block_registry_t const *registry,
                             chunk_t const *chunk,
                             bool checked[CHUNK_FULL][CHUNK_FULL],
                             face_attribs_t target,
                             byte x, byte y, byte z)
{
  face_attribs_t face = __face_at_y(registry, chunk, x, y, z);
  return !checked[x][z] && __face_cmp(face, target);
}

static void __expand_y_z(bool checked[CHUNK_FULL][CHUNK_FULL],
                         byte x, byte y, byte z)
{
  checked[x][z] = TRUE;
}

static bool __can_expand_y_x(block_registry_t const *registry,
                         chunk_t const *chunk,
                         bool checked[CHUNK_FULL][CHUNK_FULL],
                         face_attribs_t target,
                         byte x, byte y, byte z1, byte z2)
{
  for (byte z = z1; z < z2; z++) {
    face_attribs_t face = __face_at_y(registry, chunk, x, y, z);
    if (checked[x][z] ||
        !__face_cmp(face, target)) {
      return FALSE;
    }
  }

  return TRUE;
}

static void __expand_y_x(bool checked[CHUNK_FULL][CHUNK_FULL],
                         byte x, byte y, byte z1, byte z2)
{
  for (byte z = z1; z < z2; z++) {
    checked[x][z] = TRUE;
  }
}

static face_attribs_t __face_at_z(block_registry_t const *registry,
                          chunk_t const *chunk,
                          byte x, byte y, byte z)
{
  face_t front = block_registry_get_face(registry, chunk->blocks[x][y][z], FACE_EAST);
  face_t back = block_registry_get_face(registry, chunk->blocks[x][y][z + 1], FACE_WEST);

  return __face_at(registry, front, back);
}

static bool __can_expand_z_y(block_registry_t const *registry,
                         chunk_t const *chunk,
                         bool checked[CHUNK_FULL][CHUNK_FULL],
                         face_attribs_t target,
                         byte x, byte y, byte z)
{
  face_attribs_t face = __face_at_z(registry, chunk, x, y, z);
  return !checked[x][y] && __face_cmp(face, target);
}

static void __expand_z_y(bool checked[CHUNK_FULL][CHUNK_FULL],
                         byte x, byte y, byte z)
{
  checked[x][y] = TRUE;
}

static bool __can_expand_z_x(block_registry_t const *registry,
                             chunk_t const *chunk,
                             bool checked[CHUNK_FULL][CHUNK_FULL],
                             face_attribs_t target,
                             byte x, byte y1, byte y2, byte z)
{
  for (byte y = y1; y < y2; y++) {
    face_attribs_t face = __face_at_z(registry, chunk, x, y, z);
    if (checked[x][y] ||
        !__face_cmp(face, target)) {
      return FALSE;
    }
  }

  return TRUE;
}

static void __expand_z_x(bool checked[CHUNK_FULL][CHUNK_FULL],
                         byte x, byte y1, byte y2, byte z)
{
  for (byte y = y1; y < y2; y++) {
    checked[x][y] = TRUE;
  }
}

static usize __count_vertices_x(block_registry_t const *registry,
                                chunk_t const *chunk,
                                bool checked[CHUNK_FULL][CHUNK_FULL])
{
  usize vertices = 0;
  for (byte x = CHUNK_START; x < CHUNK_END; x++) {
    for (byte y = CHUNK_START; y < CHUNK_END; y++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        checked[y][z] = FALSE;
      }
    }

    for (byte y = CHUNK_START; y < CHUNK_END; y++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        if (!checked[y][z]) {
          face_attribs_t face = __face_at_x(registry, chunk, x, y, z);
          if (face.face == 0) {
            checked[y][z] = TRUE;
            continue;
          }

          byte y2 = y, z2 = z;

          while (z2 < CHUNK_SIZE &&
                 __can_expand_x_z(registry, chunk, checked, face, x, y2, z2 + 1)) {
            __expand_x_z(checked, x, y2, z2 + 1);
            z2++;
          }

          while (y2 < CHUNK_SIZE &&
                 __can_expand_x_y(registry, chunk, checked, face, x, y2 + 1, z, z2)) {
            __expand_x_y(checked, x, y2 + 1, z, z2);
            y2++;
          }

          vertices++;
        }
      }
    }
  }

  return vertices;
}

static usize __count_vertices_y(block_registry_t const *registry,
                                chunk_t const *chunk,
                                bool checked[CHUNK_FULL][CHUNK_FULL])
{
  usize vertices = 0;
  for (byte y = CHUNK_START; y < CHUNK_END; y++) {
    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        checked[y][z] = FALSE;
      }
    }

    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        if (!checked[x][z]) {
          face_attribs_t face = __face_at_y(registry, chunk, x, y, z);
          if (face.face == 0) {
            checked[x][z] = TRUE;
            continue;
          }

          byte x2 = x, z2 = z;

          while (z2 < CHUNK_SIZE &&
                 __can_expand_y_z(registry, chunk, checked, face, x2, y, z2 + 1)) {
            __expand_y_z(checked, x2, y, z2 + 1);
            z2++;
          }

          while (x2 < CHUNK_SIZE &&
                 __can_expand_y_x(registry, chunk, checked, face, x2 + 1, y, z, z2)) {
            __expand_y_x(checked, x2 + 1, y, z, z2);
            x2++;
          }

          vertices++;
        }
      }
    }
  }

  return vertices;
}

static usize __count_vertices_z(block_registry_t const *registry,
                                chunk_t const *chunk,
                                bool checked[CHUNK_FULL][CHUNK_FULL])
{
  usize vertices = 0;
  for (byte z = CHUNK_START; z < CHUNK_END; z++) {
    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte y = CHUNK_START; y < CHUNK_END; y++) {
        checked[x][y] = FALSE;
      }
    }

    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte y = CHUNK_START; y < CHUNK_END; y++) {
        if (!checked[x][y]) {
          face_attribs_t face = __face_at_z(registry, chunk, x, y, z);
          if (face.face == 0) {
            checked[x][y] = TRUE;
            continue;
          }

          byte x2 = x, y2 = y;

          while (y2 < CHUNK_SIZE &&
                 __can_expand_z_y(registry, chunk, checked, face, x2, y2 + 1, z)) {
            __expand_z_y(checked, x2, y2 + 1, z);
            y2++;
          }

          while (x2 < CHUNK_SIZE &&
                 __can_expand_z_x(registry, chunk, checked, face, x2 + 1, y, y2, z)) {
            __expand_z_x(checked, x2 + 1, y, y2, z);
            x2++;
          }

          vertices++;
        }
      }
    }
  }

  return vertices;
}

static packed_vertex_t __pack_vertex(face_t face, byte x, byte y, byte z)
{
  return ((packed_vertex_t)face << COORD_BITS * 3) |
         ((packed_vertex_t)z << COORD_BITS * 2) |
         ((packed_vertex_t)y << COORD_BITS) |
         ((packed_vertex_t)x);
}

static void __construct_face_x(packed_vertex_t *verts, usize *n,
                               face_attribs_t face,
                               byte x, byte y1, byte y2, byte z1, byte z2)
{
  if (face.direction == 0) {
    verts[(*n)++] = __pack_vertex(face.face, x, y1, z1);
    verts[(*n)++] = __pack_vertex(face.face, x, y1, z2);
    verts[(*n)++] = __pack_vertex(face.face, x, y2, z2);
    verts[(*n)++] = __pack_vertex(face.face, x, y2, z1);
  } else {
    verts[(*n)++] = __pack_vertex(face.face, x, y1, z1);
    verts[(*n)++] = __pack_vertex(face.face, x, y2, z1);
    verts[(*n)++] = __pack_vertex(face.face, x, y2, z2);
    verts[(*n)++] = __pack_vertex(face.face, x, y1, z2);
  }
}

static usize __construct_mesh_x(block_registry_t const *registry,
                                chunk_t const *chunk,
                                bool checked[CHUNK_FULL][CHUNK_FULL],
                                packed_vertex_t *verts)
{
  usize vertices = 0;
  for (byte x = CHUNK_START; x < CHUNK_END; x++) {
    for (byte y = CHUNK_START; y < CHUNK_END; y++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        checked[y][z] = FALSE;
      }
    }

    for (byte y = CHUNK_START; y < CHUNK_END; y++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        if (!checked[y][z]) {
          face_attribs_t face = __face_at_x(registry, chunk, x, y, z);
          if (face.face == 0) {
            checked[y][z] = TRUE;
            continue;
          }

          byte y2 = y, z2 = z;

          while (z2 < CHUNK_SIZE &&
                 __can_expand_x_z(registry, chunk, checked, face, x, y2, z2 + 1)) {
            __expand_x_z(checked, x, y2, z2 + 1);
            z2++;
          }

          while (y2 < CHUNK_SIZE &&
                 __can_expand_x_y(registry, chunk, checked, face, x, y2 + 1, z, z2)) {
            __expand_x_y(checked, x, y2 + 1, z, z2);
            y2++;
          }

          __construct_face_x(verts, &vertices, face, x, y, y2, z, z2);
        }
      }
    }
  }

  return vertices;
}

static void __construct_face_y(packed_vertex_t *verts, usize *n,
                               face_attribs_t face,
                               byte x1, byte x2, byte y, byte z1, byte z2)
{
  if (face.direction == 0) {
    verts[(*n)++] = __pack_vertex(face.face, x1, y, z1);
    verts[(*n)++] = __pack_vertex(face.face, x2, y, z1);
    verts[(*n)++] = __pack_vertex(face.face, x2, y, z2);
    verts[(*n)++] = __pack_vertex(face.face, x1, y, z2);
  } else {
    verts[(*n)++] = __pack_vertex(face.face, x1, y, z1);
    verts[(*n)++] = __pack_vertex(face.face, x1, y, z2);
    verts[(*n)++] = __pack_vertex(face.face, x2, y, z2);
    verts[(*n)++] = __pack_vertex(face.face, x2, y, z1);
  }
}

static usize __construct_mesh_y(block_registry_t const *registry,
                                chunk_t const *chunk,
                                bool checked[CHUNK_FULL][CHUNK_FULL],
                                packed_vertex_t *verts)
{
  usize vertices = 0;
  for (byte y = CHUNK_START; y < CHUNK_END; y++) {
    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        checked[x][z] = FALSE;
      }
    }

    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte z = CHUNK_START; z < CHUNK_END; z++) {
        if (!checked[x][z]) {
          face_attribs_t face = __face_at_y(registry, chunk, x, y, z);
          if (face.face == 0) {
            checked[x][z] = TRUE;
            continue;
          }

          byte x2 = x, z2 = z;

          while (z2 < CHUNK_SIZE &&
                 __can_expand_y_z(registry, chunk, checked, face, x2, y, z2 + 1)) {
            __expand_y_z(checked, x2, y, z2 + 1);
            z2++;
          }

          while (x2 < CHUNK_SIZE &&
                 __can_expand_y_x(registry, chunk, checked, face, x2 + 1, y, z, z2)) {
            __expand_y_x(checked, x2 + 1, y, z, z2);
            x2++;
          }

          __construct_face_y(verts, &vertices, face, x, x2, y, z, z2);
        }
      }
    }
  }

  return vertices;
}

static void __construct_face_z(packed_vertex_t *verts, usize *n,
                               face_attribs_t face,
                               byte x1, byte x2, byte y1, byte y2, byte z)
{
  if (face.direction == 0) {
    verts[(*n)++] = __pack_vertex(face.face, x1, y1, z);
    verts[(*n)++] = __pack_vertex(face.face, x2, y1, z);
    verts[(*n)++] = __pack_vertex(face.face, x2, y2, z);
    verts[(*n)++] = __pack_vertex(face.face, x1, y2, z);
  } else {
    verts[(*n)++] = __pack_vertex(face.face, x1, y1, z);
    verts[(*n)++] = __pack_vertex(face.face, x1, y2, z);
    verts[(*n)++] = __pack_vertex(face.face, x2, y2, z);
    verts[(*n)++] = __pack_vertex(face.face, x2, y1, z);
  }
}

static usize __construct_mesh_z(block_registry_t const *registry,
                                chunk_t const *chunk,
                                bool checked[CHUNK_FULL][CHUNK_FULL],
                                packed_vertex_t *verts)
{
  usize vertices = 0;
  for (byte z = CHUNK_START; z < CHUNK_END; z++) {
    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte y = CHUNK_START; y < CHUNK_END; y++) {
        checked[x][y] = FALSE;
      }
    }

    for (byte x = CHUNK_START; x < CHUNK_END; x++) {
      for (byte y = CHUNK_START; y < CHUNK_END; y++) {
        if (!checked[x][y]) {
          face_attribs_t face = __face_at_z(registry, chunk, x, y, z);
          if (face.face == 0) {
            checked[x][y] = TRUE;
            continue;
          }

          byte x2 = x, y2 = y;

          while (y2 < CHUNK_SIZE &&
                 __can_expand_z_y(registry, chunk, checked, face, x2, y2 + 1, z)) {
            __expand_z_y(checked, x2, y2 + 1, z);
            y2++;
          }

          while (x2 < CHUNK_SIZE &&
                 __can_expand_z_x(registry, chunk, checked, face, x2 + 1, y, y2, z)) {
            __expand_z_x(checked, x2 + 1, y, y2, z);
            x2++;
          }

          __construct_face_z(verts, &vertices, face, x, x2, y, y2, z);
        }
      }
    }
  }

  return vertices;
}

void chunk_mesh_create(block_registry_t const *registry, chunk_t const *chunk, chunk_mesh_t *mesh)
{
  static bool checked[CHUNK_FULL][CHUNK_FULL];

  mesh->vertices[0] = __count_vertices_x(registry, chunk, checked);
  mesh->vertices[1] = __count_vertices_y(registry, chunk, checked);
  mesh->vertices[2] = __count_vertices_z(registry, chunk, checked);

  for (usize i = 0; i < 3; i++) {
    mesh->mesh[i] = malloc(sizeof(packed_vertex_t) * mesh->vertices[i]);
    ASSERT(mesh->mesh[i] != NULL, "Failed to allocate!");
  }

  ASSERT(__construct_mesh_x(registry, chunk, checked, mesh->mesh[0]) == mesh->vertices[0],
         "Counted and constructed vertex counts don't match!");
  
  ASSERT(__construct_mesh_y(registry, chunk, checked, mesh->mesh[1]) == mesh->vertices[1],
         "Counted and constructed vertex counts don't match!");
  
  ASSERT(__construct_mesh_z(registry, chunk, checked, mesh->mesh[2]) == mesh->vertices[2],
         "Counted and constructed vertex counts don't match!");
}

void chunk_mesh_destroy(chunk_mesh_t *mesh)
{
  for (usize i = 0; i < 3; i++) {
    if (mesh->mesh[i] != NULL) {
      free(mesh->mesh[i]);
      mesh->mesh[i] = NULL;
      mesh->vertices[i] = 0;
    }
  }
}

