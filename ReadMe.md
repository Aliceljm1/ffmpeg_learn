## ffmpeg-demuxer 详细解析媒体文件
###  saveAACFromMediaFile
### showInfoFromMediaFile

###  saveKeyFrameImg 保存关键帧图片
### demux_mp4 输入mp4文件，输出h264文件和aac文件
### avio_decode_audio 通过AVIO模式读取原始数据，解码为pcm

## 调试ffmpeg库：
进入地址：https://github.com/ShiftMediaProject/FFmpeg/releases?page=2
下载编译好的lib,pdb,和对应的源代码
直接引用就能编译调试，注意需引用产物中的include
当前调试版本：FFmpeg-5.1.r106626

## 下载静态编译好的版本地址  shared版本 包含拆分的dll 
https://github.com/advancedfx/ffmpeg.zeranoe.com-builds-mirror
