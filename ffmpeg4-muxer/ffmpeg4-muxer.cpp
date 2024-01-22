// ffmpeg4-muxer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "muxing_mp4.h"

#pragma comment(lib, "../ffmpeg-4.2.1/lib/avformat.lib")
#pragma comment(lib, "../ffmpeg-4.2.1/lib/avcodec.lib")
#pragma comment(lib, "../ffmpeg-4.2.1/lib/avutil.lib")
#pragma comment(lib, "../ffmpeg-4.2.1/lib/swresample.lib")
#pragma comment(lib, "../ffmpeg-4.2.1/lib/swscale.lib")

#include "09-02-video-watermark.h"

int main(int argc,char** argv)
{
    //muxing_flv_ffmpeg4(argc, argv);
    //muxing_mp4(argc,argv);
    filterYuv(argc,argv);
    std::cout << "Hello World!\n";
}
