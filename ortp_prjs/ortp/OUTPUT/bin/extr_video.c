#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

#ifndef AV_WB32
#define AV_WB32(p, val) do {                 \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

static int alloc_and_copy(AVPacket *out,
                          const uint8_t *in, uint32_t in_size)
{
    uint32_t offset         = out->size;
    uint8_t nal_header_size = 4;
    int err;

    err = av_grow_packet(out, in_size + nal_header_size);
    if (err < 0)
        return err;

    memcpy(out->data + nal_header_size + offset, in, in_size);
	
    AV_WB32(out->data, 1);

    return 0;
}

int process_h264(const AVPacket *in, FILE *dst_fd)
{
    AVPacket *out = NULL;

    int len;
    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size    = 0;
    const uint8_t *buf;
    const uint8_t *buf_end;
    int            buf_size;
    int ret = 0, i, count = 0;
	
	out = av_packet_alloc();

    buf      = in->data;
    buf_size = in->size;
    buf_end  = in->data + in->size;

    do {
        ret= AVERROR(EINVAL);
        if (buf + 4 > buf_end)
            goto fail;

        for (nal_size = 0, i = 0; i<4; i++)
            nal_size = (nal_size << 8) | buf[i];

        buf += 4;
        unit_type = *buf & 0x1f;

		printf("nalu_type = %d, nalu_size = %d, while count = %d\n", 
			unit_type, nal_size, count++);
        if (nal_size > buf_end - buf || nal_size < 0)
            goto fail;

        if ((ret=alloc_and_copy(out, buf, nal_size)) < 0)
            goto fail;

        len = fwrite( out->data, 1, out->size, dst_fd);
        if(len != out->size){
            av_log(NULL, AV_LOG_WARNING, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
                    len,
                    out->size);
        }
        fflush(dst_fd);
		av_packet_unref(out);

		// next_nal
        buf        += nal_size;
        cumul_size += nal_size + 4;
    } while (cumul_size < buf_size);

fail:
    av_packet_free(&out);

    return ret;
}

static const char *help="usage: ./extra_video src_filename dst_filename\n";

int main(int argc, char *argv[])
{
    int ret_code;		// 函数返回值
    char errors[1024];	// 存储av_strerror(error code)得到的信息

    char *src_filename = NULL;
    char *dst_filename = NULL;

    FILE *dst_fd = NULL;

    int video_stream_index = -1;

    AVFormatContext *fmt_ctx = NULL;
    AVPacket pkt;

    av_log_set_level(AV_LOG_INFO);

    if(argc < 3){
        av_log(NULL, AV_LOG_INFO, "%s", help);
        return -1;
    }

    src_filename = argv[1];
    dst_filename = argv[2];

    if(src_filename == NULL || dst_filename == NULL){
        av_log(NULL, AV_LOG_ERROR, "src or dts file is null, check them!\n");
        return -1;
    }

    dst_fd = fopen(dst_filename, "wb");
    if (!dst_fd) {
        av_log(NULL, AV_LOG_ERROR, "Could not open destination file %s\n", dst_filename);
        return -1;
    }

	av_register_all();

    /*open input media file, and allocate format context*/
    if((ret_code = avformat_open_input(&fmt_ctx, src_filename, NULL, NULL)) < 0){
        av_strerror(ret_code, errors, 1024);
        av_log(NULL, AV_LOG_ERROR, "Could not open source file: %s, %d(%s)\n",
               src_filename,
               ret_code,
               errors);
        return -1;
    }

    /*dump input information*/
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    /*initialize packet*/
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /*find best video stream*/
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_index < 0){
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
               src_filename);
        return AVERROR(EINVAL);
    }

    /*read frames from media file*/
    while(av_read_frame(fmt_ctx, &pkt) >=0 ){
        if(pkt.stream_index == video_stream_index){
            process_h264(&pkt, dst_fd);
        }

        av_packet_unref(&pkt);
    }

    /*close input media file*/
    avformat_close_input(&fmt_ctx);
    if(dst_fd) {
        fclose(dst_fd);
    }

    return 0;
}
