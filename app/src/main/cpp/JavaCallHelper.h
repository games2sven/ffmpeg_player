//
// Created by Administrator on 2018/10/4.
//

#ifndef CUSTOMER_JAVACALLHELPER_H
#define CUSTOMER_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *vm,JNIEnv* env,jobject instace);
    ~JavaCallHelper();
    //回调java的方法
    void onError(int thread,int errorCode);
    void onPrepare(int thread);

private:
    JavaVM *vm;
    JNIEnv *env;
    jobject  instance;
    jmethodID onErrorId;
    jmethodID onPrepareId;

};


#endif //CUSTOMER_JAVACALLHELPER_H