#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include "libavformat/avformat.h"
#include <libavcodec/avcodec.h>

}
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#pragma warning(disable : 4996)

static char err_buf[128] = { 0 };
static char* av_get_err(int errnum)
{
	av_strerror(errnum, err_buf, 128);
	return err_buf;
}

void logfferror(const char* info,int ret)
{
	char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
	av_strerror(ret, buf, AV_ERROR_MAX_STRING_SIZE);
	printf("%s, fferror=%s",info, buf);
}


static void print_sample_format(const AVFrame* frame)
{
	printf("ar-samplerate: %uHz\n", frame->sample_rate);
	printf("ac-channel: %u\n", frame->channels);
	printf("f-format: %u\n", frame->format);// 格式需要注意，实际存储到本地文件时已经改成交错模式
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
		// 对于frame, avcodec_receive_frame内部每次都先调用 av_frame_unref
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)//EAGAIN 需要更多数据才能输出帧， AVERROR_EOF 所有剩余的帧都已经输出，没有更多的帧可供输出了 
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
			P表示Planar（平面），其数据格式排列方式为 :
			LLLLLLRRRRRRLLLLLLRRRRRRLLLLLLRRRRRRL...（每个LLLLLLRRRRRR为一个音频帧）
			而不带P的数据格式（即交错排列）排列方式为：
			LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...（每个LR为一个音频样本）
			对应：AV_SAMPLE_FMT_FLTP
		 播放范例：   ffplay -ar 48000 -ac 2 -f f32le believe.pcm
		  */
		int datasize = frame->nb_samples * frame->channels * per_sample_szie;
		printf("this frame datasize:%d\n", datasize);
		for (i = 0; i < frame->nb_samples; i++)
		{
			for (ch = 0; ch < dec_ctx->channels; ch++)  // 交错的方式写入, 大部分float的格式输出
				fwrite(frame->data[ch] + per_sample_szie * i, 1, per_sample_szie, outfile);
		}
	}
}
#define BUF_SIZE 20480

/**
ffmpeg 会在avformat_open_input时候，调用此函数，我们需要在buf中填充数据，给ffmpeg使用
也就是说read_packet名字是相对于ffmpeg而言的，ffmpeg是主体。
**/
static int read_packet(void* opaque, uint8_t* buf, int buf_size)
{
	FILE* in_file = (FILE*)opaque;//buf_size 是我们在avio_alloc_context设置的
	int read_size = fread(buf, 1, buf_size, in_file);
	if (read_size <= 0) {
		return AVERROR_EOF;
	}
	return read_size;
}

void decode_audio_to_file(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame,FILE* outfile)
{
	int ret = avcodec_send_packet(codec_ctx, pkt);				  
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(codec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
		int pre_sample_size = av_get_bytes_per_sample(codec_ctx->sample_fmt);
		//        print_sample_format(frame);
		//LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...（每个LR为一个音频样本）
		// 这里的写法不是通用，通用要调用重采样的函数去实现
		// 这里只是针对解码出来是planar格式的转换
		for (int i = 0; i < frame->nb_samples; i++) {
			for (int ch = 0; ch < codec_ctx->channels; ch++) {
				fwrite(frame->data[ch] + pre_sample_size * i, 1, pre_sample_size, outfile);
			}
		}
	}

}

int avio_decode_audio(int argc, char** argv)
{


	const char* infilename = "..\\res\\believe.aac";
	FILE* in_file =   fopen(infilename,"rb");
	FILE* out_file = fopen("..\\res\\out.pcm", "wb");

	uint8_t* io_buffer = (uint8_t*)av_malloc(BUF_SIZE);//自定义缓存空间
	AVIOContext* avio_ctx = avio_alloc_context(io_buffer, BUF_SIZE,0,(void *)in_file,
		read_packet, NULL, NULL);
	AVFormatContext* fmt_ctx = avformat_alloc_context();
	fmt_ctx->pb = avio_ctx;//一定要挂接起来，

													//这里输入文件为NULL,
	int ret =avformat_open_input(&fmt_ctx,NULL,NULL,NULL);

	AVCodecID codec_id = strstr(infilename, "mp3") != NULL ? AV_CODEC_ID_MP3 : AV_CODEC_ID_AAC;

	const AVCodec* codec = avcodec_find_decoder(codec_id);
	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	avcodec_open2(codec_ctx,codec,NULL);

	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	while (1) {
		ret=av_read_frame(fmt_ctx,pkt);
		if (ret < 0) {
			logfferror("av_read_frame",ret);
			break;
		}
		decode_audio_to_file(codec_ctx,pkt,frame,out_file);
		av_packet_unref(pkt);
	}

	//flusing end
	decode_audio_to_file(codec_ctx, NULL, frame, out_file);

	fclose(in_file);
	fclose(out_file);

	av_free(io_buffer);
	av_frame_free(&frame);
	av_packet_free(&pkt);


	avcodec_free_context(&codec_ctx);//这里会崩溃，why？
	//avformat_free_context(fmt_ctx);
	avformat_close_input(&fmt_ctx);//内部会调用 avformat_free_context


	return 0;
}