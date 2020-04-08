#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

#include <stdio.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
#include "libavutil/log.h"
}

/**
 * 错误提示字符串
 */
#define ERR_STR_LEN 1024
static char err_buf[ERR_STR_LEN];

/**
 * 用户自定义区
 */
#define START_FRAME 200                 // 避免视频前面是黑的, 这有点坑人~
const char *file_path = "possible.mkv"; // 视频文件的路径, 支持linux/windows格式.

#define MYDEBUG  qDebug() <<"[FILE:" <<__FILE__ <<",FUNC:" <<__FUNCTION__ <<",LINE:" <<__LINE__ <<"] "


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

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int ret = -1, index = 0, image_size = -1;
    int videoStream = -1, got_picture = -1, numBytes= -1;
    uint8_t *out_buffer = NULL;

    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVPacket *packet = NULL;
    AVFrame *pFrame = NULL, *pFrameRGB = NULL;
    struct SwsContext *img_convert_ctx = NULL;

    av_register_all();
    pFormatCtx = avformat_alloc_context();
    if(NULL == pFormatCtx){
        MYDEBUG <<"avformat_alloc_context() failed.";
        return -1;
    }

    // 1. 查找视频文件;
    ret = avformat_open_input(&pFormatCtx, file_path, NULL, NULL);
    if(ret < 0){
        av_strerror(ret, err_buf, ERR_STR_LEN);
        MYDEBUG <<err_buf;
        avformat_free_context(pFormatCtx);
        return -2;
    }

    // 2. 读取视频文件信息;
    ret = avformat_find_stream_info(pFormatCtx, NULL);
    if (ret < 0) {
        av_strerror(ret, err_buf, ERR_STR_LEN);
        MYDEBUG <<err_buf;
        goto __EXIT_ERROR;
    }

    // 3. 获取视频流
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        MYDEBUG <<"coun't find a video stream.";
        goto __EXIT_ERROR;
    }

    // 4. 根据上面得到的视频流类型打开对应的解码器
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        MYDEBUG <<"Decoder not found.";
        goto __EXIT_ERROR;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        MYDEBUG <<"Ddecoder couldn't open.";
        goto __EXIT_ERROR;
    }

    // 5. 分配并初始化一个视频packet
    image_size = pCodecCtx->width * pCodecCtx->height;
    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    if(!packet){
        MYDEBUG <<"malloc() failed.";
        goto __EXIT_QUIT;
    }
    ret = av_new_packet(packet, image_size);
    if(ret < 0){
        av_strerror(ret, err_buf, ERR_STR_LEN);
        MYDEBUG <<err_buf;
        goto __EXIT_QUIT;
    }

    // 6. 分配两个frame分别存放yuv和rgb数据
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    if(!pFrame || !pFrameRGB){
        MYDEBUG <<"pFrame:" <<pFrame
               <<", pFrameRGB:" <<pFrameRGB
              <<" av_frame_alloc() failed";
        goto __EXIT_QUIT;
    }

    // 7. 分配一个struct SwsContext结构体, 填充源图像和目标图像信息(为接下来的转换做准备)
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    // 8. pFrameRGB和out_buffer都是已经申请到的一段内存, 会将pFrameRGB的数据按RGB24格式自动"关联(转换并放置)"到out_buffer。
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height);
    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    ret = avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB24,
                pCodecCtx->width, pCodecCtx->height);
    if (!img_convert_ctx || numBytes<0 || !out_buffer || ret<0) {
        MYDEBUG <<"img_convert_ctx:" <<img_convert_ctx
               <<", numBytes:" <<numBytes
              <<", out_buffer:" <<out_buffer
             <<", ret:" <<ret;
        goto __EXIT_QUIT;
    }

    while(1){
        if (av_read_frame(pFormatCtx, packet) < 0) {
            break;
        }

        if (packet->stream_index == videoStream) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);
            if (ret < 0) {
                printf("decode error.");
                return -1;
            }
        }

        if (got_picture) {
            sws_scale(img_convert_ctx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                    pFrameRGB->linesize);

            if(index++ > START_FRAME){
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index-START_FRAME-1);
            }
        }
        av_free_packet(packet);

        if(index >START_FRAME+10){
            break;
        }
    }

__EXIT_QUIT:
    avcodec_close(pCodecCtx);
__EXIT_ERROR:
    avformat_close_input(&pFormatCtx);
    sws_freeContext(img_convert_ctx);

    if(packet){
        free(packet);
    }
    if(pFrame){
        av_frame_free(&pFrame);
    }
    if(pFrameRGB){
        av_frame_free(&pFrameRGB);
    }
    if(out_buffer){
        av_free(out_buffer);
    }

    // MainWindow w;
    // w.show();
    //return a.exec();
    return 0;
}
