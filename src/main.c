#include <math.h>
#include <stdio.h>
#include <time.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define FRAME_W 256
#define FRAME_H 256
#define NUM_NODES 256

#define COMP_Y    1
#define COMP_YA   2
#define COMP_RGB  3
#define COMP_RGBA 4

_Static_assert(NUM_NODES >= FRAME_W, "invariant");

typedef struct { int x, y; } Vec2;

// ---

uint8_t pixels[FRAME_W * COMP_RGBA * FRAME_H];

static int vec_len(Vec2 v) {
  return (int) sqrt(v.x * v.x + v.y * v.y);
}

static Vec2 vec_sub(Vec2 a, Vec2 b) {
  return (Vec2) { .x = a.x - b.x, .y = a.y - b.y };
}

static Vec2 vec_add(Vec2 a, Vec2 b) {
  return (Vec2) { .x = a.x + b.x, .y = a.y + b.y };
}

static Vec2 vec_scale(Vec2 v, double scale) {
  return (Vec2) { .x = (int) v.x * scale, .y = (int) v.y * scale };
}

static Vec2 point_offsets[9] = {
  { .x = -1, .y = -1 },
  { .x =  0, .y = -1 },
  { .x =  1, .y = -1 },
  { .x = -1, .y =  0 },
  { .x =  0, .y =  0 },
  { .x =  1, .y =  0 },
  { .x = -1, .y =  1 },
  { .x =  0, .y =  1 },
  { .x =  1, .y =  1 },
};

static void draw_point(Vec2 point) {
  for (int i = 0; i < ARR_LEN(point_offsets); i++) {
    Vec2 offset = vec_add(point, point_offsets[i]);
    if (offset.x < 0 || offset.y < 0 || offset.x >= FRAME_W || offset.y >= FRAME_H) continue;

    int n = FRAME_W * COMP_RGBA * offset.y + COMP_RGBA * offset.x;

    pixels[n]     = 0;   // R
    pixels[n + 1] = 0;   // G
    pixels[n + 2] = 0;   // B
    pixels[n + 3] = 255; // A
  }
}

static void draw_line(Vec2 basis, Vec2 direction) {
  Vec2 v = vec_sub(direction, basis);
  int len = vec_len(v);

  for (int i = 0; i < len; i++) {
    double t = (double) i / len;
    draw_point(vec_add(basis, vec_scale(v, t)));
  }
}

static void draw_rect(stbrp_rect r) {
  Vec2 top_left  = { .x = r.x,       .y = r.y       };
  Vec2 top_right = { .x = r.x + r.w, .y = r.y       };
  Vec2 bot_left  = { .x = r.x,       .y = r.y + r.h };
  Vec2 bot_right = { .x = r.x + r.w, .y = r.y + r.h };

  draw_line(top_left, top_right);
  draw_line(top_left, bot_left);
  draw_line(bot_left, bot_right);
  draw_line(top_right, bot_right);
}

int main() {
  srand(time(0)); // seed the RNG

  for (int i = 0; i < FRAME_W * FRAME_H; i++) {
    pixels[i * COMP_RGBA] =     255; // R
    pixels[i * COMP_RGBA + 1] = 255; // G
    pixels[i * COMP_RGBA + 2] = 255; // B
    pixels[i * COMP_RGBA + 3] = 255; // A
  }

  stbrp_context ctx;
  stbrp_node nodes[NUM_NODES];
  stbrp_rect rects[100];

  for (int i = 0; i < 100; i++) {
    rects[i].id = i + 1;
    rects[i].w = rand() % 14 + 10;
    rects[i].h = rand() % 14 + 10;
  }

  stbrp_init_target(&ctx, FRAME_W, FRAME_H, nodes, NUM_NODES);
  stbrp_pack_rects(&ctx, rects, ARR_LEN(rects));

  for (int i = 0; i < ARR_LEN(rects); i++) {
    if (rects[i].was_packed) {
      printf("id: %d [%dx%d] packed at (%d, %d)\n", rects[i].id, rects[i].w, rects[i].h, rects[i].x, rects[i].y);
      draw_rect(rects[i]);
    } else {
      printf("id: %d [%dx%d] unable to pack\n", rects[i].id, rects[i].w, rects[i].h);
    }
  }

  if (!stbi_write_png("packed.png", FRAME_W, FRAME_H, COMP_RGBA, pixels, FRAME_W * COMP_RGBA)) {
    printf("failed to write image output to packed.png\n");
    return 1;
  }

  return 0;
}
