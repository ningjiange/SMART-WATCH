// main/cube.h
#ifndef CUBE_H
#define CUBE_H

#include "lvgl.h"

// 初始化 3D 立方体 UI
void cube_init(lv_obj_t *parent);

// 更新立方体旋转角度
void cube_update(float pitch, float roll);

#endif
