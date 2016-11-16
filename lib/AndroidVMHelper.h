#pragma once

#include <jni.h>
#include <string>

/// helper class that allows access to java vm to communicate with java code from native
class AndroidVMHelper
{
    JNIEnv *envPtr;
    bool detachInDestructor;

public:
    AndroidVMHelper();

    ~AndroidVMHelper();

    JNIEnv *get();

    jclass findClassloadedClass(const std::string &name);

    static void cacheVM(JNIEnv *env);

    static void cacheVM(JavaVM *vm);
};
