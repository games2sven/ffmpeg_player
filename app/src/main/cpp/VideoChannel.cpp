//
// Created by Administrator on 2018/10/4.
//
extern "C"{
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include "VideoChannel.h"

void *decode_task(void *args){
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decode();
    return 0;
}

void *render_task(void *args){
    VideoChannel* channel = static_cast<VideoChannel *>(args);
    channel->render();
    return 0;
}

/**
 * 丢包 直到下一个关键帧(I,B,P三种帧只有I能正常播放，，B和P需要参考I帧播放)
 * @param q
 */

void dropAvPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        //如果不属于 I 帧
        if (packet->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAvPacket(&packet);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 丢已经解码的图片（丢包）
 */
void dropAvFrame(queue<AVFrame *> &q){
    if(!q.empty()){
        AVFrame *frame = q.front();
        BaseChannel::releaseAvFrame(&frame);
        q.pop();
    }
}


VideoChannel::VideoChannel(int id,
                           AVCodecContext *avCodecContext,AVRational time_base,int fps) : BaseChannel(id, avCodecContext,time_base) {
    this->fps = fps;
//    frames.setReleaseCallback(releaseAvFrame);
    //用于设置一个同步操作队列的一个函数指针
    frames.setSyncHandle(dropAvFrame);
}

VideoChannel::~VideoChannel() {
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}

void VideoChannel::play() {
    isPlaying = 1;
    frames.setWork(1);
    packets.setWork(1);
    //1、解码
    pthread_create(&pid_decode, 0, decode_task, this);
    //2、播放
    pthread_create(&pid_render, 0, render_task, this);
}

//解码
void VideoChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying){
        //取出一个数据包
        int ret = packets.pop(packet);
        if(!isPlaying){
            break;
        }

        //取出失败
        if(!ret){
            continue;
        }
        //把包丢给解码器
        ret = avcodec_send_packet(avCodecContext,packet);
        releaseAvPacket(&packet);
        //重试
        if(ret != 0){
            break;
        }
        //代表了一帧数据
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(avCodecContext,frame);
        //需要更多的数据才能够解码（根据返回值进行判断）
        if(ret == AVERROR(EAGAIN)){
            continue;
        }else if(ret != 0){
            break;
        }
        //再开一个线程来进行播放，以免堵塞上面的解码
        frames.push(frame);
    }
    releaseAvPacket(&packet);
}

//渲染
void VideoChannel::render() {
    //通过算法SWS_BILINEAR转换得出RGBA数据
    swsContext = sws_getContext(avCodecContext->width,avCodecContext->height,avCodecContext->pix_fmt,
                    avCodecContext->width,avCodecContext->height,AV_PIX_FMT_RGBA,
                    SWS_BILINEAR,0,0,0);
    AVFrame* frame = 0;
    //每个画面 刷新的间隔 单位：秒
    double frame_delays = 1.0/fps;
    //指针数组
    uint8_t *dst_data[4];
    int dst_linesize[4];
    av_image_alloc(dst_data,dst_linesize,
                   avCodecContext->width,avCodecContext->height,AV_PIX_FMT_RGBA,1);
    while (isPlaying){
        int ret = frames.pop(frame);
        if(!isPlaying){
            break;
        }
        //src_linesize: 表示每一行存放的 字节长度
        sws_scale(swsContext,reinterpret_cast<const uint8_t *const *>(frame->data),frame->linesize,0,avCodecContext->height,dst_data,dst_linesize);
        //不能直接进行回调播放，需要计算延时时间
      //获得当前这一个画面 播放的相对时间
        double clock = frame->best_effort_timestamp * av_q2d(time_base);
        //额外的间隔时间
        double extra_delay = frame->repeat_pict/(2*fps);
        //真实需要的间隔时间
        double delays = extra_delay + frame_delays;
        if(!audioChannel){//没有音频
            //休眠
            //视频快了
            //av_usleep(frame_delays*1000000+x);
            //视频慢了
            //av_usleep(frame_delays*1000000-x);
            av_usleep(delays*1000000);
        }else{
            if(clock == 0){
                av_usleep(delays*1000000);
            } else{
                //比较音频与视频
                double audioClock = audioChannel->clock;
                //间隔 音视频相差的间隔
                double diff = clock -audioClock;
                if(diff >0){//视频快了
                    av_usleep((delays+diff) * 1000000);
                }else if(diff < 0){//音频快了
                    //当间隔时间大于一定的值后就已经追不上来了，需要丢包
                    if(fabs(diff) >= 0.05){
                        releaseAvFrame(&frame);
                        //丢包
                        frames.sync();
                        continue;
                    }else{
                        //不睡了，快点赶上 音频
                    }
                }
            }
        }

        //回调出去进行播放
        callback(dst_data[0],dst_linesize[0],avCodecContext->width,avCodecContext->height);
        releaseAvFrame(&frame);
    }
    av_freep(&dst_data[0]);
    releaseAvFrame(&frame);
    isPlaying = 0;
    sws_freeContext(swsContext);
    swsContext = 0;
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback callback) {
    this->callback = callback;
}

void VideoChannel::stop() {
    isPlaying = 0;
    frames.setWork(0);
    packets.setWork(0);
    pthread_join(pid_decode,0);
    pthread_join(pid_render,0);
}