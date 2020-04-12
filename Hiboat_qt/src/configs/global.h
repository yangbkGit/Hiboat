#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include <QDebug>
#define MYDEBUG  qDebug() <<"[FILE:" <<__FILE__ <<",FUNC:" <<__FUNCTION__ <<",LINE:" <<__LINE__ <<"] "


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

#endif // GLOBAL_H
