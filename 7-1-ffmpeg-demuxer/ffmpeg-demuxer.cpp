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
    //打开网络流。这里如果只需要读取本地媒体文件，不需要用到网络功能，可以不用加上这一句
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

    //AVFormatContext是描述一个媒体文件或媒体流的构成和基本信息的结构体
    AVFormatContext* ifmt_ctx = NULL;           // 输入文件的demux

    int videoindex = -1;        // 视频索引
    int audioindex = -1;        // 音频索引

    AVPacket* pkt = av_packet_alloc();

    int pkt_count = 0;
    int print_max_count = 10;

    // 打开文件，主要是探测协议类型，如果是网络文件则创建网络链接
    int ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL);
    if (ret < 0)  //如果打开媒体文件失败，打印失败原因
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("open %s failed:%s\n", in_filename, buf);
        goto failed;
    }

    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0)  //如果打开媒体文件失败，打印失败原因
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("avformat_find_stream_info %s failed:%s\n", in_filename, buf);
        goto failed;
    }

    //打开媒体文件成功
    printf_s("\n==== av_dump_format in_filename:%s ===\n", in_filename);
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf_s("\n==== av_dump_format finish =======\n\n");

    // url: 调用avformat_open_input读取到的媒体文件的路径/名字
    printf("media name:%s\n", ifmt_ctx->url);
    // nb_streams: nb_streams媒体流数量
    printf("stream number:%d\n", ifmt_ctx->nb_streams);
    // bit_rate: 媒体文件的码率,单位为bps
    printf("media average ratio:%lldkbps\n", (int64_t)(ifmt_ctx->bit_rate / 1024));


    // 时间
    int total_seconds, hour, minute, second;
    // duration: 媒体文件时长，单位微妙
    total_seconds = (ifmt_ctx->duration) / AV_TIME_BASE;  // 1000us = 1ms, 1000ms = 1秒
    hour = total_seconds / 3600;
    minute = (total_seconds % 3600) / 60;
    second = (total_seconds % 60);
    //通过上述运算，可以得到媒体文件的总时长


    printf("total duration: %02d:%02d:%02d\n", hour, minute, second);
    printf("\n");
    /*
     * 老版本通过遍历的方式读取媒体文件视频和音频的信息
     * 新版本的FFmpeg新增加了函数av_find_best_stream，也可以取得同样的效果
     */
    for (uint32_t i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream* in_stream = ifmt_ctx->streams[i];// 音频流、视频流、字幕流
        //如果是音频流，则打印音频的信息
        if (AVMEDIA_TYPE_AUDIO == in_stream->codecpar->codec_type)
        {
            printf("----- Audio info:\n");
            // index: 每个流成分在ffmpeg解复用分析后都有唯一的index作为标识
            printf("index:%d\n", in_stream->index);
            // sample_rate: 音频编解码器的采样率，单位为Hz
            printf("samplerate:%dHz\n", in_stream->codecpar->sample_rate);
            // codecpar->format: 音频采样格式
            if (AV_SAMPLE_FMT_FLTP == in_stream->codecpar->format)
            {
                printf("sampleformat:AV_SAMPLE_FMT_FLTP\n");
            }
            else if (AV_SAMPLE_FMT_S16P == in_stream->codecpar->format)
            {
                printf("sampleformat:AV_SAMPLE_FMT_S16P\n");
            }
            // channels: 音频信道数目
            printf("channel number:%d\n", in_stream->codecpar->ch_layout.nb_channels);
            // codec_id: 音频压缩编码格式
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
            // 音频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
            if (in_stream->duration != AV_NOPTS_VALUE)
            {
                int duration_audio = (in_stream->duration) * av_q2d(in_stream->time_base);
                //将音频总时长转换为时分秒的格式打印到控制台上
                printf("audio duration: %02d:%02d:%02d\n",
                    duration_audio / 3600, (duration_audio % 3600) / 60, (duration_audio % 60));
            }
            else
            {
                printf("audio duration unknown");
            }

            printf("\n");

            audioindex = i; // 获取音频的索引
        }
        else if (AVMEDIA_TYPE_VIDEO == in_stream->codecpar->codec_type)  //如果是视频流，则打印视频的信息
        {
            printf("----- Video info:\n");
            printf("index:%d\n", in_stream->index);
            // avg_frame_rate: 视频帧率,单位为fps，表示每秒出现多少帧
            printf("fps:%lffps\n", av_q2d(in_stream->avg_frame_rate));
            if (AV_CODEC_ID_MPEG4 == in_stream->codecpar->codec_id) //视频压缩编码格式
            {
                printf("video codec:MPEG4\n");
            }
            else if (AV_CODEC_ID_H264 == in_stream->codecpar->codec_id) //视频压缩编码格式
            {
                printf("video codec:H264\n");
            }
            else
            {
                printf("video codec_id:%d\n", in_stream->codecpar->codec_id);
            }
            // 视频帧宽度和帧高度
            printf("width:%d height:%d\n", in_stream->codecpar->width,
                in_stream->codecpar->height);
            //视频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
            if (in_stream->duration != AV_NOPTS_VALUE)
            {
                int duration_video = (in_stream->duration) * av_q2d(in_stream->time_base);
                printf("video duration: %02d:%02d:%02d\n",
                    duration_video / 3600,
                    (duration_video % 3600) / 60,
                    (duration_video % 60)); //将视频总时长转换为时分秒的格式打印到控制台上
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

        av_packet_unref(pkt);// 每次读取完一个packet数据后，都需要释放pkt的内存， 否则会造成内存泄漏
    }

    if (pkt)
        av_packet_free(&pkt);
failed:
    if (ifmt_ctx)
        avformat_close_input(&ifmt_ctx);


}

//////////////////////////////////saveAAC/////////////////////////////////////////

#define ADTS_HEADER_LEN  7;
const int sampling_frequencies[] = {
    96000,  // 0x0
    88200,  // 0x1
    64000,  // 0x2
    48000,  // 0x3
    44100,  // 0x4
    32000,  // 0x5
    24000,  // 0x6
    22050,  // 0x7
    16000,  // 0x8
    12000,  // 0x9
    11025,  // 0xa
    8000   // 0xb
    // 0xc d e f是保留的
};

int adts_header(char* const p_adts_header, const int data_length,
    const int profile, const int samplerate,
    const int channels)
{

    int sampling_frequency_index = 3; // 默认使用48000hz
    int adtsLen = data_length + 7;

    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for (i = 0; i < frequencies_size; i++)
    {
        if (sampling_frequencies[i] == samplerate)
        {
            sampling_frequency_index = i;
            break;
        }
    }
    if (i >= frequencies_size)
    {
        printf("unsupport samplerate:%d\n", samplerate);
        return -1;
    }

    p_adts_header[0] = 0xff;         //syncword:0xfff                          高8bits
    p_adts_header[1] = 0xf0;         //syncword:0xfff                          低4bits
    p_adts_header[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    p_adts_header[1] |= (0 << 1);    //Layer:0                                 2bits
    p_adts_header[1] |= 1;           //protection absent:1                     1bit

    p_adts_header[2] = (profile) << 6;            //profile:profile               2bits
    p_adts_header[2] |= (sampling_frequency_index & 0x0f) << 2; //sampling frequency index:sampling_frequency_index  4bits
    p_adts_header[2] |= (0 << 1);             //private bit:0                   1bit
    p_adts_header[2] |= (channels & 0x04) >> 2; //channel configuration:channels  高1bit

    p_adts_header[3] = (channels & 0x03) << 6; //channel configuration:channels 低2bits
    p_adts_header[3] |= (0 << 5);               //original：0                1bit
    p_adts_header[3] |= (0 << 4);               //home：0                    1bit
    p_adts_header[3] |= (0 << 3);               //copyright id bit：0        1bit
    p_adts_header[3] |= (0 << 2);               //copyright id start：0      1bit
    p_adts_header[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

    p_adts_header[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    p_adts_header[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    p_adts_header[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    p_adts_header[6] = 0xfc;      //‭11111100‬       //buffer fullness:0x7ff 低6bits
    // number_of_raw_data_blocks_in_frame：
    //    表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。

    return 0;
}

int saveAACFromMediaFile(const char* mediafile)
{
    int ret = -1;
    char errors[1024];

    const char* in_filename = NULL;
    const char* aac_filename = NULL;

    FILE* aac_fd = NULL;

    int audio_index = -1;
    int len = 0;


    AVFormatContext* ifmt_ctx = NULL;
    AVPacket pkt;

    // 设置打印级别
    av_log_set_level(AV_LOG_DEBUG);


    in_filename = mediafile;  // 输入文件
        aac_filename = "out.aac";   // 输出文件

    if (in_filename == NULL || aac_filename == NULL)
    {
        av_log(NULL, AV_LOG_DEBUG, "src or dts file is null, plz check them!\n");
        return -1;
    }

    aac_fd = fopen(aac_filename, "wb");
    if (!aac_fd)
    {
        av_log(NULL, AV_LOG_DEBUG, "Could not open destination file %s\n", aac_filename);
        return -1;
    }

    // 打开输入文件
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL)) < 0)
    {
        av_strerror(ret, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "Could not open source file: %s, %d(%s)\n",
            in_filename,
            ret,
            errors);
        return -1;
    }

    // 获取解码器信息
    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
    {
        av_strerror(ret, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "failed to find stream information: %s, %d(%s)\n",
            in_filename,
            ret,
            errors);
        return -1;
    }

    // dump媒体信息
    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    // 初始化packet
    av_init_packet(&pkt);

    // 查找audio对应的steam index
    audio_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audio_index < 0)
    {
        av_log(NULL, AV_LOG_DEBUG, "Could not find %s stream in input file %s\n",
            av_get_media_type_string(AVMEDIA_TYPE_AUDIO),
            in_filename);
        return AVERROR(EINVAL);
    }

    // 打印AAC级别
    printf("audio profile:%d, FF_PROFILE_AAC_LOW:%d\n",
        ifmt_ctx->streams[audio_index]->codecpar->profile,
        FF_PROFILE_AAC_LOW);

    if (ifmt_ctx->streams[audio_index]->codecpar->codec_id != AV_CODEC_ID_AAC)
    {
        printf("the media file no contain AAC stream, it's codec_id is %d\n",
            ifmt_ctx->streams[audio_index]->codecpar->codec_id);
        goto failed;
    }
    // 读取媒体文件，并把aac数据帧写入到本地文件
    while (av_read_frame(ifmt_ctx, &pkt) >= 0)
    {
        if (pkt.stream_index == audio_index)
        {
            char adts_header_buf[7] = { 0 };
            adts_header(adts_header_buf, pkt.size,
                ifmt_ctx->streams[audio_index]->codecpar->profile,
                ifmt_ctx->streams[audio_index]->codecpar->sample_rate,
                ifmt_ctx->streams[audio_index]->codecpar->channels);
            fwrite(adts_header_buf, 1, 7, aac_fd);  // 写adts header , ts流不适用，ts流分离出来的packet带了adts header
            len = fwrite(pkt.data, 1, pkt.size, aac_fd);   // 写adts data
            if (len != pkt.size)
            {
                av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
                    len,
                    pkt.size);
            }
        }
        av_packet_unref(&pkt);
    }

failed:
    // 关闭输入文件
    if (ifmt_ctx)
    {
        avformat_close_input(&ifmt_ctx);
    }
    if (aac_fd)
    {
        fclose(aac_fd);
    }
}


int main(int argc, char** argv)
{
    const char* default_filename = "..\\res\\believe.mp4";

    saveAACFromMediaFile(default_filename);
    showInfoFromMediaFile(argc,argv);

    getchar();
    return 0;
}
