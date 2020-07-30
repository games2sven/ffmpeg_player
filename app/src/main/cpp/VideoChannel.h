//
// Created by Administrator on 2018/10/4.
//

#ifndef CUSTOMER_VIDEOCHANNEL_H
#define CUSTOMER_VIDEOCHANNEL_H


#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libswscale/swscale.h>
};

typedef void (*RenderFrameCallback)(uint8_t *,int,int,int);
class VideoChannel : public BaseChannel{
public:
    VideoChannel(int id, AVCodecContext *avCodecContext,AVRational time_base,int fps);
    ~VideoChannel();

    void setAudioChannel(AudioChannel *audioChannel);

    //解码+播放
    void play();
    void decode();
    void render();
    void stop();
    void setRenderFrameCallback(RenderFrameCallback callback);

private:
    pthread_t pid_decode;
    pthread_t pid_render;
    SwsContext *swsContext = 0;
    RenderFrameCallback callback;
    AudioChannel *audioChannel = 0;
    int fps;
};


#endif //CUSTOMER_VIDEOCHANNEL_H
