//
// Created by Administrator on 2018/10/4.
//

#ifndef CUSTOMER_AUDIOCHANNEL_H
#define CUSTOMER_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
extern  "C"{
#include <libswresample/swresample.h>
};

class AudioChannel: public BaseChannel {
public:
    AudioChannel(int id, AVCodecContext *avCodecContext,AVRational time_base);
    ~AudioChannel();
    void play();
    void decode();
    void _play();
    void stop();
    int getPcm();

public:
    uint8_t *data = 0;
    int out_channels;//声道数量
    int out_sample_rate;//输出采样率
    int out_samplesize;//输出采样大小

private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    /**
   * OpenSL ES
   */
    // 引擎与引擎接口
    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject =0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;

    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface =0;

    //重采样结构体
    SwrContext *swrContext = 0;

};


#endif //CUSTOMER_AUDIOCHANNEL_H
