#pragma once
#include <stdio.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/bsf.h"
}

void logfferror(int ret)
{
	char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
	av_strerror(ret, buf, AV_ERROR_MAX_STRING_SIZE);
	printf("fferror=%s", buf);
}

int make_adts_header(char* const p_adts_header, const int data_length,
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


void video_pkt_saveto_h264_file(AVPacket* pkt, AVFormatContext* fmt_ctx, FILE* file)
{
	static const AVBitStreamFilter* bsf = NULL;
	static AVBSFContext* bsf_ctx = NULL;
	if (bsf == NULL) {
		bsf = av_bsf_get_by_name("h264_mp4toannexb");
		if (bsf == NULL) {
			printf("av_bsf_get_by_name failed");
			return;
		}
		int ret = -1;

		ret = av_bsf_alloc(bsf, &bsf_ctx);
		if (ret < 0) {
			logfferror(ret);
			return;
		}
		//初始化bsf_ctx->par_in ,编码参数
		ret = avcodec_parameters_copy(bsf_ctx->par_in,
			fmt_ctx->streams[pkt->stream_index]->codecpar);
		if (ret < 0)
		{
			logfferror(ret);
			av_bsf_free(&bsf_ctx);
			return;
		}

		ret = av_bsf_init(bsf_ctx);
		if (ret < 0)
		{
			logfferror(ret);
			av_bsf_free(&bsf_ctx);
			return;
		}
	}
	else {
		int ret = -1;//经过h264_mp4toannexb过滤器 将会增加SPS,PPS,和startcode
		ret=av_bsf_send_packet(bsf_ctx, pkt);
		if (ret < 0) {
			logfferror(ret);
			av_packet_free(&pkt);
			return;
		}

		while(1)
		{
			ret = av_bsf_receive_packet(bsf_ctx, pkt);
			//发送一个packet可能会返回多个packet，需要一直读到返回EAGAIN
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			if (ret < 0) {
				logfferror(ret);
				return;
			}
			fwrite(pkt->data, 1, pkt->size, file);
			av_packet_unref(pkt);//释放pkt的buf，否则内存泄露，注意不是释放释放结构体
		}  
	}
}

//输入mp4文件，输出h264文件和aac文件
int demux_mp4(int argc, char* argv[])
{
	char input_filename[MAX_PATH];
	const char* h264_filename = NULL;
	const char* aac_filename = NULL;
	if (argc != 4) {
		printf("usage: input.mp4 out.h264 out.aac");
		strcpy(input_filename, "..\\res\\believe.mp4");
		h264_filename = "..\\res\\out.h264";
		aac_filename = "..\\res\\out.aac";
	}
	else {
		strcpy(input_filename, argv[1]);
		h264_filename = argv[2];
		aac_filename = argv[3];
	}

	FILE* h264_file = fopen(h264_filename, "wb");
	FILE* aac_file = fopen(aac_filename, "wb");
	if (h264_file == NULL || aac_file == NULL) {
		printf("open file failed");
		return -1;
	}
	int ret = -1;
	int video_index = -1;
	int audio_index = -1;
	AVPacket* pkt = NULL;
	AVFormatContext* fmt_ctx = avformat_alloc_context();
	if (!fmt_ctx) {
		printf("avformat_alloc_context failed");
		goto return_error;
	}
	//不是网络流不需要设置超时
	ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
	if (ret != 0) {
		printf("avformat_open_input failed");
		avformat_close_input(&fmt_ctx);
		logfferror(ret);
		goto return_error;
	}
	getchar();
	AVStream * stream=fmt_ctx->streams[0];
	stream->nb_frames;
	stream->codec_info_nb_frames;
	video_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (video_index == -1)
	{
		printf("av_find_best_stream video_index failed");
		goto return_error;
	}
	audio_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (audio_index == -1)
	{
		printf("av_find_best_stream audio_index failed");
		goto return_error;
	}
	pkt = av_packet_alloc();
	av_init_packet(pkt);//初始化packet的数据buf

	while (1) {
		ret = av_read_frame(fmt_ctx, pkt);
		//内部不会释放pkt.buf, 用完pkt需要手动释放buf, 否则内存泄露
		if (ret < 0) {
			logfferror(ret);
			break;
		}
		if (pkt->stream_index == audio_index) {
			char adts_header_buf[7] = { 0 };
			make_adts_header(adts_header_buf, pkt->size,
				fmt_ctx->streams[audio_index]->codecpar->profile,
				fmt_ctx->streams[audio_index]->codecpar->sample_rate,
				fmt_ctx->streams[audio_index]->codecpar->channels);
			fwrite(adts_header_buf, 1, 7, aac_file);//写adts header , ts流不适用，ts流分离出来的packet带了adts header
			fwrite(pkt->data, 1, pkt->size, aac_file);//写adts data

		}
		else if (pkt->stream_index == video_index) {
			video_pkt_saveto_h264_file(pkt, fmt_ctx, h264_file);
		}
		av_packet_unref(pkt);

	}


	goto return_success;

return_error:
	if (h264_file)
		fclose(h264_file);
	if (aac_file)
		fclose(aac_file);
	return -1;
return_success:
	printf("\ndone\n");
	av_packet_free(&pkt);
	avformat_close_input(&fmt_ctx);
	return 0;
}