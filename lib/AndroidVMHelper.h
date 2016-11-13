#pragma once

#include <jni.h>

/// helper class that allows access to java vm to communicate with java code from native
class AndroidVMHelper
{
public:
    static JNIEnv *attach();
    static void detach();
    static void cacheVM(JNIEnv *env);
    static void cacheVM(JavaVM *vm);
};
