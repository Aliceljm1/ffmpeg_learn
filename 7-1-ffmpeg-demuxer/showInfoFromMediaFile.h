#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <windows.h>
#include <iostream>

#ifdef __cplusplus

extern "C" {
#endif

#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif
#pragma warning(disable : 4996)


void showInfoFromMediaFile(int argc, char** argv)
{
    //�����������������ֻ��Ҫ��ȡ����ý���ļ�������Ҫ�õ����繦�ܣ����Բ��ü�����һ��
//    avformat_network_init();

    char path[256];
    GetCurrentDirectoryA(256, path);
    printf("Current path is :%s \n", path);

    const char* default_filename = "..\\res\\believe.mp4";

    const char* in_filename = NULL;

    if (argv[1] == NULL)
    {
        in_filename = default_filename;
    }
    else
    {
        in_filename = argv[1];
    }
    printf("in_filename = %s\n", in_filename);

    //AVFormatContext������һ��ý���ļ���ý�����Ĺ��ɺͻ�����Ϣ�Ľṹ��
    AVFormatContext* ifmt_ctx = NULL;           // �����ļ���demux

    int videoindex = -1;        // ��Ƶ����
    int audioindex = -1;        // ��Ƶ����

    AVPacket* pkt = av_packet_alloc();

    int pkt_count = 0;
    int print_max_count = 10;

    // ���ļ�����Ҫ��̽��Э�����ͣ�����������ļ��򴴽���������
    int ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL);
    if (ret < 0)  //�����ý���ļ�ʧ�ܣ���ӡʧ��ԭ��
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("open %s failed:%s\n", in_filename, buf);
        goto failed;
    }

    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0)  //�����ý���ļ�ʧ�ܣ���ӡʧ��ԭ��
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("avformat_find_stream_info %s failed:%s\n", in_filename, buf);
        goto failed;
    }

    //��ý���ļ��ɹ�
    printf_s("\n==== av_dump_format in_filename:%s ===\n", in_filename);
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf_s("\n==== av_dump_format finish =======\n\n");

    // url: ����avformat_open_input��ȡ����ý���ļ���·��/����
    printf("media name:%s\n", ifmt_ctx->url);
    // nb_streams: nb_streamsý��������
    printf("stream number:%d\n", ifmt_ctx->nb_streams);
    // bit_rate: ý���ļ�������,��λΪbps
    printf("media average ratio:%lldkbps\n", (int64_t)(ifmt_ctx->bit_rate / 1024));


    // ʱ��
    int total_seconds, hour, minute, second;
    // duration: ý���ļ�ʱ������λ΢��
    total_seconds = (ifmt_ctx->duration) / AV_TIME_BASE;  // 1000us = 1ms, 1000ms = 1��
    hour = total_seconds / 3600;
    minute = (total_seconds % 3600) / 60;
    second = (total_seconds % 60);
    //ͨ���������㣬���Եõ�ý���ļ�����ʱ��


    printf("total duration: %02d:%02d:%02d\n", hour, minute, second);
    printf("\n");
    /*
     * �ϰ汾ͨ�������ķ�ʽ��ȡý���ļ���Ƶ����Ƶ����Ϣ
     * �°汾��FFmpeg�������˺���av_find_best_stream��Ҳ����ȡ��ͬ����Ч��
     */
    for (uint32_t i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream* in_stream = ifmt_ctx->streams[i];// ��Ƶ������Ƶ������Ļ��
        //�������Ƶ�������ӡ��Ƶ����Ϣ
        if (AVMEDIA_TYPE_AUDIO == in_stream->codecpar->codec_type)
        {
            printf("----- Audio info:\n");
            // index: ÿ�����ɷ���ffmpeg�⸴�÷�������Ψһ��index��Ϊ��ʶ
            printf("index:%d\n", in_stream->index);
            // sample_rate: ��Ƶ��������Ĳ����ʣ���λΪHz
            printf("samplerate:%dHz\n", in_stream->codecpar->sample_rate);
            // codecpar->format: ��Ƶ������ʽ
            if (AV_SAMPLE_FMT_FLTP == in_stream->codecpar->format)
            {
                printf("sampleformat:AV_SAMPLE_FMT_FLTP\n");
            }
            else if (AV_SAMPLE_FMT_S16P == in_stream->codecpar->format)
            {
                printf("sampleformat:AV_SAMPLE_FMT_S16P\n");
            }
            // channels: ��Ƶ�ŵ���Ŀ
            printf("channel number:%d\n", in_stream->codecpar->ch_layout.nb_channels);
            // codec_id: ��Ƶѹ�������ʽ
            if (AV_CODEC_ID_AAC == in_stream->codecpar->codec_id)
            {
                printf("audio codec:AAC\n");
            }
            else if (AV_CODEC_ID_MP3 == in_stream->codecpar->codec_id)
            {
                printf("audio codec:MP3\n");
            }
            else
            {
                printf("audio codec_id:%d\n", in_stream->codecpar->codec_id);
            }
            // ��Ƶ��ʱ������λΪ�롣ע������ѵ�λ�Ŵ�Ϊ�������΢���Ƶ��ʱ������Ƶ��ʱ����һ����ȵ�
            if (in_stream->duration != AV_NOPTS_VALUE)
            {
                int duration_audio = (in_stream->duration) * av_q2d(in_stream->time_base);
                //����Ƶ��ʱ��ת��Ϊʱ����ĸ�ʽ��ӡ������̨��
                printf("audio duration: %02d:%02d:%02d\n",
                    duration_audio / 3600, (duration_audio % 3600) / 60, (duration_audio % 60));
            }
            else
            {
                printf("audio duration unknown");
            }

            printf("\n");

            audioindex = i; // ��ȡ��Ƶ������
        }
        else if (AVMEDIA_TYPE_VIDEO == in_stream->codecpar->codec_type)  //�������Ƶ�������ӡ��Ƶ����Ϣ
        {
            printf("----- Video info:\n");
            printf("index:%d\n", in_stream->index);
            // avg_frame_rate: ��Ƶ֡��,��λΪfps����ʾÿ����ֶ���֡
            printf("fps:%lffps\n", av_q2d(in_stream->avg_frame_rate));
            if (AV_CODEC_ID_MPEG4 == in_stream->codecpar->codec_id) //��Ƶѹ�������ʽ
            {
                printf("video codec:MPEG4\n");
            }
            else if (AV_CODEC_ID_H264 == in_stream->codecpar->codec_id) //��Ƶѹ�������ʽ
            {
                printf("video codec:H264\n");
            }
            else
            {
                printf("video codec_id:%d\n", in_stream->codecpar->codec_id);
            }
            // ��Ƶ֡��Ⱥ�֡�߶�
            printf("width:%d height:%d\n", in_stream->codecpar->width,
                in_stream->codecpar->height);
            //��Ƶ��ʱ������λΪ�롣ע������ѵ�λ�Ŵ�Ϊ�������΢���Ƶ��ʱ������Ƶ��ʱ����һ����ȵ�
            if (in_stream->duration != AV_NOPTS_VALUE)
            {
                int duration_video = (in_stream->duration) * av_q2d(in_stream->time_base);
                printf("video duration: %02d:%02d:%02d\n",
                    duration_video / 3600,
                    (duration_video % 3600) / 60,
                    (duration_video % 60)); //����Ƶ��ʱ��ת��Ϊʱ����ĸ�ʽ��ӡ������̨��
            }
            else
            {
                printf("video duration unknown");
            }

            printf("\n");
            videoindex = i;
        }
    }

    printf("\n-----av_read_frame start\n");
    while (1)
    {
        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0)
        {
            printf("av_read_frame end\n");
            break;
        }

        if (pkt_count++ < print_max_count)
        {
            if (pkt->stream_index == audioindex)
            {
                printf("audio pts: %lld\n", pkt->pts);
                printf("audio dts: %lld\n", pkt->dts);
                printf("audio size: %d\n", pkt->size);
                printf("audio pos: %lld\n", pkt->pos);
                printf("audio duration: %lf\n\n",
                    pkt->duration * av_q2d(ifmt_ctx->streams[audioindex]->time_base));
            }
            else if (pkt->stream_index == videoindex)
            {
                printf("video pts: %lld\n", pkt->pts);
                printf("video dts: %lld\n", pkt->dts);
                printf("video size: %d\n", pkt->size);
                printf("video pos: %lld\n", pkt->pos);
                printf("video duration: %lf\n\n",
                    pkt->duration * av_q2d(ifmt_ctx->streams[videoindex]->time_base));
            }
            else
            {
                printf("unknown stream_index:\n", pkt->stream_index);
            }
        }

        av_packet_unref(pkt);// ÿ�ζ�ȡ��һ��packet���ݺ󣬶���Ҫ�ͷ�pkt���ڴ棬 ���������ڴ�й©
    }

    if (pkt)
        av_packet_free(&pkt);
failed:
    if (ifmt_ctx)
        avformat_close_input(&ifmt_ctx);


}
