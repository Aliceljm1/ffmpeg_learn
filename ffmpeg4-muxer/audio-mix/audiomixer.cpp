#include "audiomixer.h"

AudioMixer::AudioMixer()
    : initialized_(false)
    , filter_graph_(nullptr)
    , audio_output_info_(nullptr)
{

    audio_mix_info_.reset(new AudioInfo);
    audio_mix_info_->filter_name = "amix";     // 混音用的

    audio_sink_info_.reset(new AudioInfo);
    audio_sink_info_->filter_name = "sink";    // 输出
}

AudioMixer::~AudioMixer()
{
    if(initialized_) {
        exit();
    }
}

int AudioMixer::addAudioInput(uint32_t index, uint32_t samplerate, uint32_t channels,
                              uint32_t bitsPerSample, AVSampleFormat format)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (initialized_)
    {
        return -1;
    }

    if (audio_input_info_map.find(index) != audio_input_info_map.end())
    {
        return -1;      // 已经存在则返回-1
    }

    // 初始化一个input 可以有多个输入
    auto& filterInfo = audio_input_info_map[index];
    // 初始化音频相关的参数
    filterInfo.samplerate = samplerate;
    filterInfo.channels = channels;
    filterInfo.bitsPerSample = bitsPerSample;
    filterInfo.format = format;
    filterInfo.filter_name = std::string("input") + std::to_string(index);

    return 0;
}

int AudioMixer::addAudioOutput(const uint32_t samplerate, const uint32_t channels,
                               const uint32_t bitsPerSample, const AVSampleFormat format)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (initialized_)
    {
        return -1;
    }
    // 初始化输出相关的参数       只有一个输出
    audio_output_info_.reset(new AudioInfo);
    audio_output_info_->samplerate = samplerate;
    audio_output_info_->channels = channels;
    audio_output_info_->bitsPerSample = bitsPerSample;
    audio_output_info_->format = format;
    audio_output_info_->filter_name = "output";
    return 0;
}
/*
inputs
    The number of inputs. If unspecified, it defaults to 2.//输入的数量，如果没有指明，默认为2.
duration
    How to determine the end-of-stream.//决定了流的结束
    longest
    The duration of the longest input. (default)//最长输入的持续时间
    shortest
    The duration of the shortest input.//最短输入的持续时间
    first
    The duration of the first input.//第一个输入的持续时间 比如直播，mic为主的时候 first，
dropout_transition
    The transition time, in seconds, for volume renormalization when an input stream ends.
    The default value is 2 seconds.
*/
/**
 * @brief 初始化
 * @param duration longest最长输入时间,shortest最短,first第一个输入持续的时间
 * @return
 */
int AudioMixer::init(const  char *duration)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (initialized_)
    {
        return -1;
    }

    if (audio_input_info_map.size() == 0)
    {
        return -1;
    }

    filter_graph_ = avfilter_graph_alloc(); // 创建avfilter_graph
    if (filter_graph_ == nullptr)
    {
        return -1;
    }

    char args[512] = {0};

    const AVFilter *amix = avfilter_get_by_name("amix");    // 混音
    audio_mix_info_->filterCtx = avfilter_graph_alloc_filter(filter_graph_, amix, 
        audio_mix_info_->filter_name.c_str());
    /*inputs=输入流数量, duration=决定流的结束,
     * dropout_transition= 输入流结束时,容量重整时间,
     * (longest最长输入时间,shortest最短,first第一个输入持续的时间))*/
    snprintf(args, sizeof(args), "inputs=%d:duration=%s:dropout_transition=0",
             audio_input_info_map.size(), duration);
    if (avfilter_init_str(audio_mix_info_->filterCtx, args) != 0)
    {
        printf("[AudioMixer] avfilter_init_str(amix) failed.\n");
        return -1;
    }

    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    audio_sink_info_->filterCtx = avfilter_graph_alloc_filter(filter_graph_, abuffersink, 
        audio_sink_info_->filter_name.c_str());
    if (avfilter_init_str(audio_sink_info_->filterCtx, nullptr) != 0)
    {
        printf("[AudioMixer] avfilter_init_str(abuffersink) failed.\n");
        return -1;
    }

    for (auto& iter : audio_input_info_map)
    {
        const AVFilter *abuffer = avfilter_get_by_name("abuffer");
        snprintf(args, sizeof(args),
                 "sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
                 iter.second.samplerate,
                 av_get_sample_fmt_name(iter.second.format),
                 av_get_default_channel_layout(iter.second.channels));
        printf("[AudioMixer] input(%d) args: %s\n", iter.first, args);

        iter.second.filterCtx = avfilter_graph_alloc_filter(filter_graph_, abuffer,
                                                            audio_output_info_->filter_name.c_str());

        if (avfilter_init_str(iter.second.filterCtx, args) != 0)
        {
            printf("[AudioMixer] avfilter_init_str(abuffer) failed.\n");
            return -1;
        }
        //所有的输入音频link到mixFilter中，  iter.first 是input index 
        if (avfilter_link(iter.second.filterCtx, 0, audio_mix_info_->filterCtx, iter.first) != 0)
        {
            printf("[AudioMixer] avfilter_link(abuffer(%d), amix) failed.", iter.first);
            return -1;
        }
    }

    if (audio_output_info_ != nullptr)
    {//格式过滤器，连接到最后的输出过滤器abuffersink
        const AVFilter *aformat = avfilter_get_by_name("aformat");
        snprintf(args, sizeof(args),
                 "sample_rates=%d:sample_fmts=%s:channel_layouts=0x%I64x",
                 audio_output_info_->samplerate,
                 av_get_sample_fmt_name(audio_output_info_->format),
                 av_get_default_channel_layout(audio_output_info_->channels));
        printf("[AudioMixer] output args: %s\n", args);
        audio_output_info_->filterCtx = avfilter_graph_alloc_filter(filter_graph_, aformat,
                                                                    "aformat");

        if (avfilter_init_str(audio_output_info_->filterCtx, args) != 0)
        {
            printf("[AudioMixer] avfilter_init_str(aformat) failed. %s\n", args);
            return -1;
        }

        if (avfilter_link(audio_mix_info_->filterCtx, 0, audio_output_info_->filterCtx, 0) != 0)
        {
            printf("[AudioMixer] avfilter_link(amix, aformat) failed.\n");
            return -1;
        }

        if (avfilter_link(audio_output_info_->filterCtx, 0, audio_sink_info_->filterCtx, 0) != 0)
        {
            printf("[AudioMixer] avfilter_link(aformat, abuffersink) failed.\n");
            return -1;
        }
    }

    if (avfilter_graph_config(filter_graph_, NULL) < 0)
    {
        printf("[AudioMixer] avfilter_graph_config() failed.\n");
        return -1;
    }

    initialized_ = true;
    return 0;
}

int AudioMixer::exit()
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (initialized_)
    {
        for (auto iter : audio_input_info_map)
        {//ffmpeg对象要单独释放
            if (iter.second.filterCtx != nullptr)
            {
                avfilter_free(iter.second.filterCtx);
            }
        }

        audio_input_info_map.clear();

        if (audio_output_info_ && audio_output_info_->filterCtx)
        {
            avfilter_free(audio_output_info_->filterCtx);
            audio_output_info_->filterCtx = nullptr;
        }

        if (audio_mix_info_->filterCtx)
        {
            avfilter_free(audio_mix_info_->filterCtx);
            audio_mix_info_->filterCtx = nullptr;
        }

        if (audio_sink_info_->filterCtx)
        {
            avfilter_free(audio_sink_info_->filterCtx);
            audio_sink_info_->filterCtx = nullptr;
        }

        avfilter_graph_free(&filter_graph_);
        filter_graph_ = nullptr;
        initialized_ = false;
    }

    return 0;
}

/**
* 将输入音频数据，构造为AVFrame，然后调用av_buffersrc_add_frame()添加到filter graph中
* 如果输入数据为null,则调用av_buffersrc_add_frame(XX,NULL)冲刷缓冲区
*/
int AudioMixer::addFrame(uint32_t index, uint8_t *inBuf, uint32_t size)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (!initialized_)
    {
        return -1;
    }

    auto iter = audio_input_info_map.find(index);
    if (iter == audio_input_info_map.end())
    {
        return -1;
    }

    if(inBuf && size > 0) {
        std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame *ptr) { av_frame_free(&ptr); });

        avFrame->sample_rate = iter->second.samplerate;
        avFrame->format = iter->second.format;
        avFrame->channel_layout = av_get_default_channel_layout(iter->second.channels);
        avFrame->nb_samples = size * 8 / iter->second.bitsPerSample / iter->second.channels;
        
        av_frame_get_buffer(avFrame.get(), 1);//依据媒体格式给AVFrame分配内存
         memcpy(avFrame->extended_data[0], inBuf, size);

        if (av_buffersrc_add_frame(iter->second.filterCtx, avFrame.get()) != 0)
        {
            return -1;
        }
    } else {//冲刷缓冲区
        if (av_buffersrc_add_frame(iter->second.filterCtx, NULL) != 0)
        {
            return -1;
        }
    }


    return 0;
}

int AudioMixer::getFrame(uint8_t *outBuf, uint32_t maxOutBufSize)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (!initialized_)
    {
        return -1;
    }
    //智能指针
    std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame *ptr) { av_frame_free(&ptr); });

    int ret = av_buffersink_get_frame(audio_sink_info_->filterCtx, avFrame.get());

    if (ret < 0)
    {
        //        printf("ret = %d, %d\n", ret, AVERROR(EAGAIN));
        return -1;
    }

    //获取所有通道数据
    int size = av_samples_get_buffer_size(NULL, avFrame->channels, avFrame->nb_samples, (AVSampleFormat)avFrame->format, 1);
    //why? 为什么这里用这个函数获取数据大小，而不是直接用avFrame->linesize[0] * avFrame->channels 呢？
    //因为音频可能是planar格式，也可能是packed格式，planar格式的数据大小不一定等于linesize[0] * channels（这只是单通道数据） ,
    if (size > (int)maxOutBufSize)
    {
        return 0;
    }

    memcpy(outBuf, avFrame->extended_data[0], size);
    return size;
}
