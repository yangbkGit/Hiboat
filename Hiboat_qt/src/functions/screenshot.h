#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "../configs/global.h"

/**
 * @brief SaveFrame 截图函数
 * @param pFrame    图像的数据
 * @param width     图像的宽度
 * @param height    图像的高度
 * @param index     图像的编号
 */
void SaveFrame(AVFrame *pFrame, int width, int height,int index);

#endif // SCREENSHOT_H
