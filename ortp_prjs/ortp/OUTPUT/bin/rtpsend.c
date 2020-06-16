/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
 *
 * This file is part of oRTP.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>

#ifndef _WIN32 
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
#include "libavutil/log.h"

#define VIDEO_TYPE	96
#define AUDIO_TYPE	97
#define MAX_RTP_PKT_LEN 1400
#define DefaultTimestampIncrement 3750 //(90000/24)

int runcond=1;
static uint32_t user_ts=0;

// for sps/pps nalu
#define SPS_SIZE 10
#define PPS_SIZE 4
static uint8_t sps[SPS_SIZE] = {0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8};
static uint8_t pps[PPS_SIZE] = {0x68, 0xce, 0x38, 0x80};

void stophandler(int signum)
{
	runcond=0;
}

int process_packet2rtp(RtpSession *session, AVPacket *packet)
{
    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size = 0;
    uint8_t *buf;
    const uint8_t *buf_end;
    int            buf_size;
    int i, count = 0;
	uint8_t fua_indicator, fua_header;
	
    buf      = packet->data;
    buf_size = packet->size;
    buf_end  = packet->data + packet->size;

    do {
        if (buf + 4 > buf_end)
            return -1;

        for (nal_size = 0, i = 0; i<4; i++)
            nal_size = (nal_size << 8) | buf[i];

        buf += 4;
        unit_type = *buf & 0x1f;
		
		fua_indicator = (buf[0] & 0x60) | 0x1C;	// FU-A indicator
		fua_header    = (buf[0] & 0x1F);		// FU-A header

		printf("nalu_type = %d, nalu_size = %d, while count = %d\n", 
			unit_type, nal_size, count++);
        if (nal_size > buf_end - buf || nal_size < 0)
            return -2;

		if(unit_type == 5){
			// for no sps/pps before I frame, todo
			
		}

		if(nal_size <= MAX_RTP_PKT_LEN) {
	        rtp_session_send_with_ts(session, buf, nal_size, user_ts);

	    } else if(nal_size > MAX_RTP_PKT_LEN){
			int circle, remain, pos = 0;
			buf -= 1;	// for FU-A indicator

			circle = nal_size/MAX_RTP_PKT_LEN;
			remain = nal_size%MAX_RTP_PKT_LEN;
			if(remain != 0)
				circle++;

			// length = MAX_RTP_PKT_LEN;
			for(i=0; i<circle; i++) {
				buf[pos]   = fua_indicator;
				buf[pos+1] = fua_header;
			
				if(i == 0){
					buf[pos+1] |= 0x80;
					
				} else if(i == circle-1){
					// remain length
					buf[pos+1]  = fua_header | 0x40;
					rtp_session_send_with_ts(session, &buf[pos], remain+1, user_ts);
					break;
				}

				rtp_session_send_with_ts(session, &buf[pos], MAX_RTP_PKT_LEN+1, user_ts);
				pos += MAX_RTP_PKT_LEN;
			}
			
		}

		user_ts += DefaultTimestampIncrement;
		usleep(30*1000);

		// next_nal
        buf        += nal_size;
        cumul_size += nal_size + 4;
    } while (cumul_size < buf_size);

    return cumul_size;
}

static const char *help="usage: rtpsend	filename dest_ip4addr dest_port [ --with-clockslide <value> ] [ --with-jitter <milliseconds>]\n";

int main(int argc, char *argv[])
{
	RtpSession *session;
	int i;
	char *ssrc;
	int clockslide=0;
	int jitter=0;

    int ret_code;		// 函数返回值
    char errors[1024];	// 存储av_strerror(error code)得到的信息
    int video_stream_index = -1;
    AVFormatContext *fmt_ctx = NULL;
    AVPacket *packet = NULL;

    av_log_set_level(AV_LOG_INFO);
	
	if (argc<4){
		printf("%s", help);
		return -1;
	}
	for(i=4;i<argc;i++){
		if (strcmp(argv[i],"--with-clockslide")==0){
			i++;
			if (i>=argc) {
				printf("%s", help);
				return -1;
			}
			clockslide=atoi(argv[i]);
			ortp_message("Using clockslide of %i milisecond every 50 packets.",clockslide);
		}else if (strcmp(argv[i],"--with-jitter")==0){
			ortp_message("Jitter will be added to outgoing stream.");
			i++;
			if (i>=argc) {
				printf("%s", help);
				return -1;
			}
			jitter=atoi(argv[i]);
		}
	}
	
	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(NULL, ORTP_DEBUG|ORTP_TRACE|ORTP_MESSAGE
									|ORTP_WARNING|ORTP_ERROR|BCTBX_LOG_FATAL);
	session=rtp_session_new(RTP_SESSION_SENDONLY);	
	
	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 0);
	//rtp_session_set_connected_mode(session, TRUE);
	rtp_session_set_remote_addr(session, argv[2], atoi(argv[3]));
	rtp_session_set_payload_type(session, VIDEO_TYPE);
	
	ssrc=getenv("SSRC");
	if (ssrc!=NULL) {
		printf("using SSRC=%i.\n",atoi(ssrc));
		rtp_session_set_ssrc(session,atoi(ssrc));
	}


	av_register_all();

	/*open input media file, and allocate format context*/
    if((ret_code = avformat_open_input(&fmt_ctx, argv[1], NULL, NULL)) < 0){
        av_strerror(ret_code, errors, 1024);
        av_log(NULL, AV_LOG_ERROR, "Could not open source file: %s, %d(%s)\n",
               argv[1],
               ret_code,
               errors);
        return -1;
    }
	
	/*dump input information*/
    av_dump_format(fmt_ctx, 0, argv[1], 0);

	packet = av_packet_alloc();

	/*find best video stream*/
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_index < 0){
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
               argv[1]);
        return AVERROR(EINVAL);
    }


	signal(SIGINT,stophandler);
	while(runcond && av_read_frame(fmt_ctx, packet) ==0)
	{
		if(packet->stream_index != video_stream_index)
			continue;

		process_packet2rtp(session, packet);

		av_packet_unref(packet);
	}

	/*close input media file*/
    avformat_close_input(&fmt_ctx);

	av_packet_free(&packet);


	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();

	return 0;
}
