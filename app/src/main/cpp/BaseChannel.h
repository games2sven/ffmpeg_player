//
// Created by Administrator on 2018/10/7.
//

#ifndef CUSTOMER_BASECHANNEL_H
#define CUSTOMER_BASECHANNEL_H

extern "C" {
#include <libavcodec/avcodec.h>
};
#include "safe_queue.h"

class BaseChannel{
public:
    BaseChannel(int id,AVCodecContext *avCodecContext,AVRational time_base) : id(id),avCodecContext(avCodecContext),time_base(time_base){
        frames.setReleaseCallback(releaseAvFrame);
        packets.setReleaseCallback(releaseAvPacket);
    };

    virtual ~BaseChannel(){
        frames.clear();
        packets.clear();
    }

    static void releaseAvPacket(AVPacket** packet){
        if(packet){
            av_packet_free(packet);
            //为什么用指针的指针？
            // 指针的指针能够修改传递进来的指针的指向
            *packet = 0;
        }
    }

    static void releaseAvFrame(AVFrame **frame){
        if(frame){
            av_frame_free(frame);
            //为什么用指针的指针？
            // 指针的指针能够修改传递进来的指针的指向
            *frame = 0;
        }
    }


    //纯虚方法 相当于 抽象方法
    virtual void play() = 0;
    virtual void stop() = 0;

    int id;
    bool isPlaying;
    AVCodecContext *avCodecContext;
    //未解码数据包队列
    SafeQueue<AVPacket *> packets;
    //解码数据包队列
    SafeQueue<AVFrame *> frames;
    //时间基（用来做音视频同步）
    AVRational time_base;
public:
    double clock;
};



#endif //CUSTOMER_BASECHANNEL_H
