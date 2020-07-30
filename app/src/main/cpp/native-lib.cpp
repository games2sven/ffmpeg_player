#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "ZLFFmpeg.h"
#include "macro.h"

extern "C"{
#include <libavcodec/avcodec.h>
}

ZLFFmpeg *zlfFmpeg = 0;
ANativeWindow* window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;
JavaCallHelper *helper = 0;

JavaVM *javaVm = 0;
int JNI_OnLoad(JavaVM* vm,void*r){
    javaVm = vm;
    return JNI_VERSION_1_6;
}

//渲染
void render(uint8_t *data,int linesize,int w,int h){
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w,
                                     h,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride：一行多少个数据（RGBA） *4
    int dst_linesize = window_buffer.stride * 4;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst_data + i * dst_linesize, data + i * linesize, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_sven_player_ZLPlayer_native_1prepare(JNIEnv *env, jobject instance,
                                                      jstring dataSource_) {
    const char *dataSource = env->GetStringUTFChars(dataSource_, 0);
    //创建播放器
     helper = new JavaCallHelper(javaVm,env,instance);
    zlfFmpeg = new ZLFFmpeg(helper,dataSource);
    zlfFmpeg->setRenderFrameCallback(render);
    zlfFmpeg->prepare();
    env->ReleaseStringUTFChars(dataSource_, dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_sven_player_ZLPlayer_native_1start(JNIEnv *env, jobject instance) {

    // TODO
    if(zlfFmpeg){
        zlfFmpeg->start();
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_sven_player_ZLPlayer_native_1setSurface(JNIEnv *env, jobject instance,
                                                         jobject surface) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_sven_player_ZLPlayer_native_1stop(JNIEnv *env, jobject instance) {

    // TODO
    if(zlfFmpeg){
        zlfFmpeg->stop();
    }
    DELETE(helper);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_sven_player_ZLPlayer_native_1release(JNIEnv *env, jobject instance) {

    pthread_mutex_lock(&mutex);
    if (window) {
        //把播放器window释放
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);

}