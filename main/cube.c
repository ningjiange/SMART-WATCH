// main/cube.c — 3D 实心立方体
#include "cube.h"
#include <math.h>
#include <string.h>

#define CUBE_SIZE   40
#define CANVAS_W    180
#define CANVAS_H    130
#define CENTER_X    (CANVAS_W / 2)
#define CENTER_Y    (CANVAS_H / 2)
#define FOV         300

static lv_obj_t *canvas = NULL;
static lv_color_t cbuf[CANVAS_W * CANVAS_H];

// 8 个顶点
static const float vertices[8][3] = {
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE},
};

// 6 个面（每个面 4 个顶点，逆时针）
static const int faces[6][4] = {
    {0, 1, 2, 3},  // 后面
    {4, 5, 6, 7},  // 前面
    {0, 1, 5, 4},  // 下面
    {2, 3, 7, 6},  // 上面
    {0, 3, 7, 4},  // 左面
    {1, 2, 6, 5},  // 右面
};

// 面颜色（深浅不同）

static inline void set_pixel(int x, int y, lv_color_t color) {
    if (x >= 0 && x < CANVAS_W && y >= 0 && y < CANVAS_H) {
        cbuf[y * CANVAS_W + x] = color;
    }
}

// 扫描线填充三角形
static void fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, lv_color_t color) {
    // 按 y 排序
    if (y0 > y1) { int16_t t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    if (y0 > y2) { int16_t t; t=x0; x0=x2; x2=t; t=y0; y0=y2; y2=t; }
    if (y1 > y2) { int16_t t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; }

    for (int16_t y = y0; y <= y2; y++) {
        int16_t xa, xb;
        if (y < y1) {
            xa = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            xb = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        } else {
            if (y1 == y2) {
                xa = x1;
                xb = x2;
            } else {
                xa = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                xb = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
            }
        }
        if (xa > xb) { int16_t t = xa; xa = xb; xb = t; }
        for (int16_t x = xa; x <= xb; x++) {
            set_pixel(x, y, color);
        }
    }
}

// 填充四边形（分成两个三角形）
static void fill_quad(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, int16_t x3, int16_t y3, lv_color_t color) {
    fill_triangle(x0, y0, x1, y1, x2, y2, color);
    fill_triangle(x0, y0, x2, y2, x3, y3, color);
}

// 旋转并投影
static void project_vertex(float x, float y, float z,
                           float cos_p, float sin_p,
                           float cos_r, float sin_r,
                           int16_t *out_x, int16_t *out_y) {
    float y1 = y * cos_p - z * sin_p;
    float z1 = y * sin_p + z * cos_p;
    float x2 = x * cos_r + z1 * sin_r;
    float z2 = -x * sin_r + z1 * cos_r;
    float scale = (float)FOV / (FOV + z2);
    *out_x = CENTER_X + (int16_t)(x2 * scale);
    *out_y = CENTER_Y + (int16_t)(y1 * scale);
}

// Bresenham 画线
static void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, lv_color_t color) {
    int16_t dx = abs(x1 - x0), dy = abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;
    while (1) {
        set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void cube_init(lv_obj_t *parent) {
    canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_W, CANVAS_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, 30);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
}

void cube_update(float pitch, float roll) {
    if (!canvas) return;

    memset(cbuf, 0, sizeof(cbuf));

    float p = pitch * M_PI / 180.0f;
    float r = roll * M_PI / 180.0f;
    float cos_p = cosf(p), sin_p = sinf(p);
    float cos_r = cosf(r), sin_r = sinf(r);

    // 投影顶点
    int16_t proj[8][2];
    for (int i = 0; i < 8; i++) {
        project_vertex(vertices[i][0], vertices[i][1], vertices[i][2],
                       cos_p, sin_p, cos_r, sin_r, &proj[i][0], &proj[i][1]);
    }

    // 计算法向量，判断面是否可见
    // 简化：根据 z 坐标平均值排序，远的先画
    float face_z[6];
    for (int i = 0; i < 6; i++) {
        float avg_z = 0;
        for (int j = 0; j < 4; j++) {
            int vi = faces[i][j];
            float x = vertices[vi][0], y = vertices[vi][1], z = vertices[vi][2];
            float z1 = y * sin_p + z * cos_p;
            float z2 = -x * sin_r + z1 * cos_r;
            avg_z += z2;
        }
        face_z[i] = avg_z / 4.0f;
    }

    // 简单排序（冒泡）
    int order[6] = {0, 1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5 - i; j++) {
            if (face_z[order[j]] > face_z[order[j + 1]]) {
                int t = order[j]; order[j] = order[j + 1]; order[j + 1] = t;
            }
        }
    }

    // 画面（从远到近）
    lv_color_t colors[] = {
        lv_color_make(0, 80, 0),    // 后面 - 深绿
        lv_color_make(0, 100, 0),   // 前面
        lv_color_make(0, 60, 0),    // 下面
        lv_color_make(0, 140, 0),   // 上面
        lv_color_make(0, 70, 0),    // 左面
        lv_color_make(0, 120, 0),   // 右面
    };

    for (int i = 0; i < 6; i++) {
        int fi = order[i];
        int v0 = faces[fi][0], v1 = faces[fi][1], v2 = faces[fi][2], v3 = faces[fi][3];
        fill_quad(proj[v0][0], proj[v0][1],
                  proj[v1][0], proj[v1][1],
                  proj[v2][0], proj[v2][1],
                  proj[v3][0], proj[v3][1],
                  colors[fi]);
    }

    // 画边框（亮绿色）
    lv_color_t edge = lv_color_make(0, 255, 0);
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            int v0 = faces[i][j], v1 = faces[i][(j + 1) % 4];
            draw_line(proj[v0][0], proj[v0][1], proj[v1][0], proj[v1][1], edge);
        }
    }

    lv_obj_invalidate(canvas);
}
