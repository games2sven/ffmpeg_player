//
// Created by Administrator on 2018/10/4.
//

#include "ZLFFmpeg.h"
#include <cstring>
#include "macro.h"
#include <pthread.h>
extern "C" {
#include <libavutil/time.h>
}


void* task_prepare(void *args){
    ZLFFmpeg *zlfFmpeg = static_cast<ZLFFmpeg *>(args);
    zlfFmpeg->_prepare();
    return 0;
}

void* play(void *args){
    ZLFFmpeg *zlfFmpeg = static_cast<ZLFFmpeg *>(args);
    zlfFmpeg->_start();
    return 0;
}


ZLFFmpeg::ZLFFmpeg(JavaCallHelper *callHelper, const char *dataSource) {
    this->callHelper = callHelper;
    //防止 dataSource参数 指向的内存被释放
    this->datasource = new char[strlen(dataSource)];
    strcpy(this->datasource,dataSource);
}

ZLFFmpeg::~ZLFFmpeg() {
    DELETE(datasource);
}

void ZLFFmpeg::prepare() {
    //创建一个线程，在子线程里面进行prepare
    pthread_create(&pid,0,task_prepare,this);
}

void ZLFFmpeg::_prepare() {
    //初始化ffmpeg网络
    avformat_network_init();
    //1、打开媒体地址(文件地址、直播地址)
    // AVFormatContext  包含了 视频的 信息(宽、高等)
    //第一步：打开url地址
    // AVFormatContext  包含了 视频的 信息(宽、高等)
    //文件路径不对 手机没网
    // 第3个参数： 指示打开的媒体格式(传NULL，ffmpeg就会自动推到出是mp4还是flv)
    AVDictionary *options = 0;
    //设置超时时间 微妙 超时时间5秒
    av_dict_set(&options, "timeout", "5000000", 0);
    int result = avformat_open_input(&formatContext,datasource,0,&options);
    av_dict_free(&options);
    if(result != 0){//不为0表示打开失败
        LOGE("打开媒体失败:%s",av_err2str(result));
        if (isPlaying) {
            callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //第二步：查找媒体中的音视频流 （给formatContext里面的信息赋值）
    result = avformat_find_stream_info(formatContext,0);
    // 小于0 则失败
    if (result < 0){
        LOGE("查找流失败:%s",av_err2str(result));
        if (isPlaying) {
            callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    //nb_streams :几个流(几段视频/音频)
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //可能是一个视频，也可能是一个音频
        AVStream* stream = formatContext->streams[i];
        //包含了 解码 这段流 的各种参数信息(宽、高、码率、帧率)
        AVCodecParameters *codecParameters = stream->codecpar;
        //无论视频还是音频都需要干的一些事情（获得解码器）
        // 1、通过 当前流 使用的 编码方式，查找解码器()
        AVCodec *avCodec = avcodec_find_decoder(codecParameters->codec_id);
        if(avCodec == NULL){
            LOGE("查找解码器失败");
            if (isPlaying) {
                callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        //2,获取解码器上下文
        AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
        if(avCodecContext == NULL){
            LOGE("创建解码上下文失败");
            if (isPlaying) {
                callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        //3、设置上下文内的一些参数 (context->width)
//        context->width = codecpar->width;
//        context->height = codecpar->height;
        result = avcodec_parameters_to_context(avCodecContext,codecParameters);
        //失败
        if(result < 0){
            LOGE("设置解码上下文参数失败:%s",av_err2str(result));
            if (isPlaying) {
                callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        // 4、打开解码器
        result = avcodec_open2(avCodecContext,avCodec,0);
        if (result != 0){
            LOGE("打开解码器失败:%s",av_err2str(result));
            if (isPlaying) {
                callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        // 单位
        AVRational time_base = stream->time_base;

        //音频 //单独类去处理
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
            audioChannel = new AudioChannel(i,avCodecContext,time_base);
        } else if(codecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
            //1
            //帧率： 单位时间内 需要显示多少个图像
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = av_q2d(frame_rate);

            videoChannel = new VideoChannel(i,avCodecContext,time_base,fps);
            videoChannel->setRenderFrameCallback(callback);
        }
    };

    //没有音视频  (很少见)
    if(!audioChannel && !videoChannel){
        LOGE("没有音视频");
        if (isPlaying) {
            callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    // 准备完了 通知java 你随时可以开始播放
    if (isPlaying) {
        callHelper->onPrepare(THREAD_CHILD);
    }

}

void ZLFFmpeg::start() {
    //正在播放
    isPlaying = 1;

    //声音的解码与播放
    if(audioChannel){
        audioChannel->play();
    }
    //视频
    if(videoChannel){
        //设置为工作状态
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }
    pthread_create(&pid_play,0,play,this);
}

/**
 * 专门读取数据包
 */
void ZLFFmpeg::_start() {
    //1、读取媒体数据包(音视频数据包)
    int ret;
    while(isPlaying){
        //读取文件的时候没有网络请求，一下子读完了，可能导致oom
        //特别是读本地文件的时候 一下子就读完了
        if (audioChannel && audioChannel->packets.size() > 100) {
            //10ms
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }
        AVPacket* packet = av_packet_alloc();//
        ret = av_read_frame(formatContext,packet);
        //=0成功 其他:失败
        if(ret == 0){
            //stream_index 这一个流的一个序号
            if(audioChannel && packet->stream_index == audioChannel->id){
                audioChannel->packets.push(packet);
            }else if(videoChannel && packet->stream_index == videoChannel->id){
                videoChannel->packets.push(packet);
            }
        }else if(ret == AVERROR_EOF){
            //读取完成，但是可能还没播放完成
            if(audioChannel->packets.empty() && audioChannel->frames.empty() && videoChannel->packets.empty() && videoChannel->frames.empty()){
                break;//有点不懂？？
            }
        }else{
            break;
        }
    }
    isPlaying = 0;
    audioChannel->stop();
    videoChannel->stop();
}

void ZLFFmpeg::setRenderFrameCallback(RenderFrameCallback callback){
    this->callback = callback;
}

void *sync_stop(void *args){
    ZLFFmpeg *zlfFmpeg = static_cast<ZLFFmpeg *>(args);
    //卡在pid线程，，等到pid线程走完了之后才会走下面的方法
    pthread_join(zlfFmpeg->pid,0);
    //保证start线程走完
    pthread_join(zlfFmpeg->pid_play,0);
    DELETE(zlfFmpeg->videoChannel);
    DELETE(zlfFmpeg->audioChannel);
    //这个时候释放formatContext才没有风险
    if(zlfFmpeg->formatContext){
        //先关闭读取 (关闭fileintputstream)
        avformat_close_input(&zlfFmpeg->formatContext);
        avformat_free_context(zlfFmpeg->formatContext);
        zlfFmpeg->formatContext = 0;
    }
    DELETE(zlfFmpeg);
    return 0;
}

void ZLFFmpeg::stop() {
    isPlaying = 0;
    callHelper = 0;
    //stop是由java层调用，，可能是在主线程调用的，但是stop线程可能会卡住，所以必须要避免anr异常，需要开线程处理
    pthread_create(&pid_stop,0,sync_stop,this);
}
