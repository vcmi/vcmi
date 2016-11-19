#pragma once

#include <jni.h>
#include <string>

/// helper class that allows access to java vm to communicate with java code from native
class AndroidVMHelper
{
    JNIEnv *envPtr;
    bool detachInDestructor;

    jclass findClass(const std::string &name, bool classloaded);
public:
    AndroidVMHelper();

    ~AndroidVMHelper();

    JNIEnv *get();

    jclass findClassloadedClass(const std::string &name);

    void callStaticVoidMethod(const std::string &cls, const std::string &method, bool classloaded = false);

    std::string callStaticStringMethod(const std::string &cls, const std::string &method, bool classloaded = false);

    static void cacheVM(JNIEnv *env);

    static void cacheVM(JavaVM *vm);

    static constexpr const char *NATIVE_METHODS_DEFAULT_CLASS = "eu/vcmi/vcmi/NativeMethods";
};
