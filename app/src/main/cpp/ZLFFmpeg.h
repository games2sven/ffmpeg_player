//
// Created by Administrator on 2018/10/4.
//

#ifndef CUSTOMER_ZLFFMPEG_H
#define CUSTOMER_ZLFFMPEG_H


#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"


extern "C"{
#include <libavformat/avformat.h>
};

class ZLFFmpeg {
public:
    ZLFFmpeg(JavaCallHelper *callHelper, const char* dataSource);
    ~ZLFFmpeg();

    void prepare();
    void _prepare();
    void start();
    void _start();
    void stop();

    void setRenderFrameCallback(RenderFrameCallback callback);
public:
    char *datasource;
    pthread_t pid;//线程id
    AVFormatContext *formatContext = 0;
    JavaCallHelper *callHelper = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    pthread_t pid_play;
    pthread_t pid_stop;
    RenderFrameCallback callback;
    bool isPlaying;
};


#endif //CUSTOMER_ZLFFMPEG_H
