#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <windows.h>
#include <iostream>

//#include "07-05-decode_audio.h"
#include "showInfoFromMediaFile.h"
#include "saveAACFromMediaFile.h"
//#include "07-6-decode_video.h"
#include "07-08-demuex-mp4.h"

int main(int argc, char** argv)
{
    //decode_audio(argc,argv);
    const char* default_filename = "..\\res\\believe.mp4";
    //saveAACFromMediaFile(default_filename);
    //showInfoFromMediaFile(argc,argv);
    //decode_video(argc,argv);
    demux_mp4(argc,argv);

    getchar();
    return 0;
}
