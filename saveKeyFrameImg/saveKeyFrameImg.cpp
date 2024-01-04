// saveKeyFrameImg.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <windows.h>
//#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
};

void save_frame_as_bitmap(AVFrame* pFrame, int width, int height, int iFrame) {
    FILE* pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "%d.bmp", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;

    // Write header
    BITMAPFILEHEADER bmpheader;
    BITMAPINFOHEADER bmpinfo;
    bmpheader.bfType = 0x4D42; //'BM'
    bmpheader.bfReserved1 = 0;
    bmpheader.bfReserved2 = 0;
    bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpheader.bfSize = bmpheader.bfOffBits + pFrame->linesize[0] * height;

    bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo.biWidth = width;
    bmpinfo.biHeight = -height;  //Negative to create a top-down DIB
    bmpinfo.biPlanes = 1;
    bmpinfo.biBitCount = 24;
    bmpinfo.biCompression = BI_RGB;
    bmpinfo.biSizeImage = 0;
    bmpinfo.biXPelsPerMeter = 0;
    bmpinfo.biYPelsPerMeter = 0;
    bmpinfo.biClrUsed = 0;
    bmpinfo.biClrImportant = 0;

    fwrite(&bmpheader, sizeof(BITMAPFILEHEADER), 1, pFile);
    fwrite(&bmpinfo, sizeof(BITMAPINFOHEADER), 1, pFile);

    // Write pixel data
    for (y = 0; y < height; y++) {
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
    }

    // Close file
    fclose(pFile);
}

const char* get_ffmpeg_error(int errnum) {
    static char buffer[AV_ERROR_MAX_STRING_SIZE];
    if (av_strerror(errnum, buffer, AV_ERROR_MAX_STRING_SIZE) < 0)
        return "unknow ffmpeg error";
    return buffer;
}

//创建一个空的AVFrame，用于保存解码后的数据，需要自己释放
AVFrame* createEmptyFrame(int width, int height, AVPixelFormat pixformat,int align)
{
    AVFrame* pFrameRGB = av_frame_alloc();
    pFrameRGB->format = pixformat;//存放的媒体是图片，
    pFrameRGB->width = width;
    pFrameRGB->height = height;

    av_frame_get_buffer(pFrameRGB,align);//依据媒体格式 初始化内存，
    
    av_frame_make_writable(pFrameRGB);
    return pFrameRGB;

    if (false) {//手动申请内存的方法，因为知道是图片所以直接依据规则申请内存，挂接到avframe中
        int buffersize = av_image_get_buffer_size(pixformat,
            width, height, align);//不同格式，不同对齐有不同的buffersize
        uint8_t* buffer = (uint8_t*)av_malloc(buffersize);

        av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
            buffer, pixformat,
            width, height, align);//需要把buffer和pFrameRGB关联起来,否则数据默认是空的

        return pFrameRGB;
    }
}

int getkeyFrameToBitmap()
{
    av_register_all();

    AVFormatContext* ifmt_ctx = NULL;
    if (avformat_open_input(&ifmt_ctx, "test.h264", 0, 0) != 0)
        return -1;
    if (avformat_find_stream_info(ifmt_ctx, NULL) < 0)
        return -2;

    int videoStream = -1;//找到视频流index，
    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    if (videoStream == -1)
        return -3; // Didn't find a video stream

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
        return -4;

    //拷贝编码参数到上下文中，核心是pCodecCtx->extradata赋值
    avcodec_parameters_to_context(pCodecCtx, ifmt_ctx->streams[videoStream]->codecpar);

    AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    //关联pCodec和pCodecCtx ，此时 pCodecCtx->codec 关联到pCodec了。后续不再使用codec
    avcodec_open2(pCodecCtx, pCodec, NULL);
    AVFrame* pFrame = av_frame_alloc();
    if (pFrame == NULL)
        return -5;

    AVFrame* pFrameRGB = createEmptyFrame(pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, 32);


    //初始化sws上下文，
    SwsContext* sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    int frameindex = 0;
    AVPacket packet;
    int ret = -1;
    int i = 0;
    while (av_read_frame(ifmt_ctx, &packet) >= 0) {
        if (packet.stream_index == videoStream) {
            ret = avcodec_send_packet(pCodecCtx, &packet);
            ret = avcodec_receive_frame(pCodecCtx, pFrame);
            if (ret == 0 && pFrame->key_frame) {
                //解码成功，并且是关键帧，则转换数据为RGB，保存到bitmap
                //参数，上下文，数据数组，步长数组，源图y位置，结束y位置，
                ret = sws_scale(sws_ctx,
                    pFrame->data,
                    pFrame->linesize,
                    0,
                    pCodecCtx->height,
                    pFrameRGB->data,
                    pFrameRGB->linesize
                );
                save_frame_as_bitmap(pFrameRGB, pCodecCtx->width,
                    pCodecCtx->height, i++);
            }
            else {
                printf("decode error:%s\n", get_ffmpeg_error(ret));
            }
        }
    }

    av_packet_unref(&packet);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);

    avcodec_close(pCodecCtx);
    sws_freeContext(sws_ctx);

    avformat_close_input(&ifmt_ctx);
}

/**
*测试AVFrame的内存初始化，浅拷贝，深拷贝
* */
void testAVFrameMem() 
{
    AVFrame* frame = av_frame_alloc();
    frame->nb_samples = 1024;//1024个采样点
    frame->format = AV_SAMPLE_FMT_S16;//每采样2字节
    frame->channel_layout = AV_CH_LAYOUT_MONO;//单声道, 希腊语monos,单一的
    AV_CH_LAYOUT_STEREO;//双声道
   
    int ret = av_frame_get_buffer(frame,0);//初始化内存，
    int buf_ref_count = -1;
    if (frame->buf && frame->buf[0]) {
        buf_ref_count = av_buffer_get_ref_count(frame->buf[0]);
        printf("1 frame->buf.size=%d,buf.ref.count=%d \n", frame->buf[0]->size, buf_ref_count);
    }

    AVFrame* newAVFrame = av_frame_alloc();
    av_frame_ref(newAVFrame, frame);//newAVFrame引用frame的buffer数据区，refcount=2, 数据共享可读。

    if (frame->buf && frame->buf[0]) {
        buf_ref_count = av_buffer_get_ref_count(frame->buf[0]);//此时refcount=2;
        printf("1.1 frame->buf.size=%d,buf.ref.count=%d \n", frame->buf[0]->size, buf_ref_count);
    }

    av_frame_make_writable(newAVFrame);//初始化新的buffer数据区


    ret = av_frame_make_writable(frame);
    buf_ref_count = av_buffer_get_ref_count(frame->buf[0]);
    printf("2 frame->buf.size=%d,buf.ref.count=%d \n", frame->buf[0]->size, buf_ref_count);
    
    av_frame_unref(frame);
    if (frame->buf && frame->buf[0]) {
        buf_ref_count = av_buffer_get_ref_count(frame->buf[0]);
        printf("3 frame->buf.size=%d,buf.ref.count=%d \n", frame->buf[0]->size, buf_ref_count);
    }
    
    av_frame_free(&frame);

}

//从h264文件中提取关键帧,保存为bitmap图片
int main()
{
    //getkeyFrameToBitmap();
    testAVFrameMem();

    std::cout << "Hello World!\n";
}
