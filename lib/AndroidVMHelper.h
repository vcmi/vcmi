#pragma once

#include <jni.h>

/// helper class that allows access to java vm to communicate with java code from native
class AndroidVMHelper
{
    JNIEnv *envPtr;
    bool detachInDestructor;
public:
    AndroidVMHelper();
    ~AndroidVMHelper();
    JNIEnv* get();
    static void cacheVM(JNIEnv *env);
    static void cacheVM(JavaVM *vm);
};
