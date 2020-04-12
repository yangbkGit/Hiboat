#include "screenshot.h"

void SaveFrame(AVFrame *pFrame, int width, int height,int index)
{
    FILE *pFile;
    char szFilename[32];
    int  x, y;
    uint8_t *pTemp;

    sprintf(szFilename, "debug/frame%d.ppm", index);
    pFile=fopen(szFilename, "wb");

    if(pFile == NULL)
        return;

    //写文件头, ppm格式: https://blog.csdn.net/MACMACip/article/details/105378600
    fprintf(pFile, "P6 %d %d 255", width, height);

    /**
     * 写像素数据
     * 使用for(y=0; y<height; y++){
     *     fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
     * } 也可, 如果你的RGB排布刚好是正确的像素的话~
     */
    for(y=0; y<height; y++) {
        pTemp = pFrame->data[0] + y*pFrame->linesize[0];
        for(x=0; x<width; x++) {
            fwrite(pTemp+2, 1, 1, pFile);
            fwrite(pTemp+0, 1, 1, pFile);
            fwrite(pTemp+1, 1, 1, pFile);
            pTemp += 3;
        }
    }

    fclose(pFile);
}





