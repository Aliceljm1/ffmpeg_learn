#pragma once

/**
* @projectName   07-05-decode_audio
* @brief         ������Ƶ����Ҫ�Ĳ��Ը�ʽaac��mp3
* @author        Liao Qingfu
* @date          2020-01-16
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/frame.h>

#include <libavcodec/avcodec.h>

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static char err_buf[128] = { 0 };
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}

static void print_sample_format(const AVFrame* frame)
{
    printf("ar-samplerate: %uHz\n", frame->sample_rate);
    printf("ac-channel: %u\n", frame->channels);
    printf("f-format: %u\n", frame->format);// ��ʽ��Ҫע�⣬ʵ�ʴ洢�������ļ�ʱ�Ѿ��ĳɽ���ģʽ
}

static void decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame,
    FILE* outfile)
{
    int i, ch;
    int ret, per_sample_szie;
    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret == AVERROR(EAGAIN))
    {
        fprintf(stderr, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
    }
    else if (ret < 0)
    {
        fprintf(stderr, "Error submitting the packet to the decoder, err:%s, pkt_size:%d\n",
            av_get_err(ret), pkt->size);
        //        exit(1);
        return;
    }

    /* read all the output frames (infile general there may be any number of them */
    while (ret >= 0)
    {
        // ����frame, avcodec_receive_frame�ڲ�ÿ�ζ��ȵ��� av_frame_unref
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)//EAGAIN ��Ҫ�������ݲ������֡�� AVERROR_EOF ����ʣ���֡���Ѿ������û�и����֡�ɹ������ 
            return;
        else if (ret < 0)
        {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        per_sample_szie = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (per_sample_szie < 0)
        {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        static int s_print_format = 0;
        if (s_print_format == 0)
        {
            s_print_format = 1;
            print_sample_format(frame);
        }
        /**
            P��ʾPlanar��ƽ�棩�������ݸ�ʽ���з�ʽΪ :
            LLLLLLRRRRRRLLLLLLRRRRRRLLLLLLRRRRRRL...��ÿ��LLLLLLRRRRRRΪһ����Ƶ֡��
            ������P�����ݸ�ʽ�����������У����з�ʽΪ��
            LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...��ÿ��LRΪһ����Ƶ������
            ��Ӧ��AV_SAMPLE_FMT_FLTP
         ���ŷ�����   ffplay -ar 48000 -ac 2 -f f32le believe.pcm
          */
        int datasize=frame->nb_samples* frame->channels * per_sample_szie;
        printf("this frame datasize:%d\n", datasize);
        for (i = 0; i < frame->nb_samples; i++)
        {
            for (ch = 0; ch < dec_ctx->channels; ch++)  // ����ķ�ʽд��, �󲿷�float�ĸ�ʽ���
                fwrite(frame->data[ch] + per_sample_szie * i, 1, per_sample_szie, outfile);
        }
    }
}
// ���ŷ�����   ffplay -ar 48000 -ac 2 -f f32le believe.pcm
int decode_audio(int argc, char** argv)
{
    const char* outfilename;
    const char* filename;
    const AVCodec* codec;
    AVCodecContext* codec_ctx = NULL;
    AVCodecParserContext* parser = NULL;
    int len = 0;
    int ret = 0;
    FILE* infile = NULL;
    FILE* outfile = NULL;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data = NULL;//׼������������
    size_t   data_size = 0;//׼�����������ݴ�С
    AVPacket* pkt = NULL;
    AVFrame* decoded_frame = NULL;

    if (argc <= 2)
    {
        //fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        //exit(0);
        filename = "..\\res\\believe.mp3";
        outfilename = "..\\res\\believe.pcm";
    }
    else {
        filename = argv[1];
        outfilename = argv[2];
}
    pkt = av_packet_alloc();
    enum AVCodecID audio_codec_id = AV_CODEC_ID_AAC;
    if (strstr(filename, "aac") != NULL)
    {
        audio_codec_id = AV_CODEC_ID_AAC;
    }
    else if (strstr(filename, "mp3") != NULL)
    {
        audio_codec_id = AV_CODEC_ID_MP3;
    }
    else
    {
        printf("default codec id:%d\n", audio_codec_id);
    }

    // ���ҽ�����
    codec = avcodec_find_decoder(audio_codec_id);  // AV_CODEC_ID_AAC
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    // ��ȡ�����Ľ����� AVCodecParserContext(����)  +  AVCodecParser(����)
    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "Parser not found\n");
        exit(1);
    }
    // ����codec������
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    // ���������ͽ����������Ľ��й���
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    // �������ļ�
    infile = fopen(filename, "rb");
    if (!infile) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    // ������ļ�
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(codec_ctx);
        exit(1);
    }

    // ��ȡ�ļ����н���
    data = inbuf;
    data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, infile);

    while (data_size > 0)
    {
        if (!decoded_frame)
        {
            if (!(decoded_frame = av_frame_alloc()))
            {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        }
        //����ݽ����������ͣ�������һ��������packet�����ľ���ȷ��data����һ��֡�ı߽磬
        //��ʱ��pkt����data��size��ֵ�������Ķ���0 ���ҵ�һ�ε���ʱpkt->data == data�� ���������µ��ڴ棩��pkt.size���ǵ�һ֡�Ĵ�С
        ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size,
            data, data_size,
            AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0)
        {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }
        data += ret;   // �����Ѿ�����������
        data_size -= ret;   // ��Ӧ�Ļ����СҲ����Ӧ��С

        if (pkt->size)
            decode(codec_ctx, pkt, decoded_frame, outfile);

        if (data_size < AUDIO_REFILL_THRESH)    // ��������������ٴζ�ȡ
        {
            memmove(inbuf, data, data_size);    // ��֮ǰʣ�����ݿ�����buffer����ʼλ��
            data = inbuf;
            // ��ȡ���� ����: AUDIO_INBUF_SIZE - data_size ,ȷ���ܳ�����Ȼ��AUDIO_INBUF_SIZE
            len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, infile);
            if (len > 0)
                data_size += len;
        }
    }

    /* ��ˢ������  ����������ײ��� */
    pkt->data = NULL;   // �������drain mode
    pkt->size = 0;
    //�������п��ܻ���һЩ���ݣ���Ҫͨ������һ�������ݰ���������߽�����û�и�������������ˣ�������Ӧ�ÿ�ʼ�������ʣ���֡
    decode(codec_ctx, pkt, decoded_frame, outfile);

    fclose(outfile);
    fclose(infile);

    avcodec_free_context(&codec_ctx);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    printf("main finish, please enter Enter and exit\n");
    return 0;
}
