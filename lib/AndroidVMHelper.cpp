#include "AndroidVMHelper.h"

static JavaVM *vmCache = nullptr;

/// cached java classloader so that we can find our classes from other threads
static jobject vcmiClassLoader;
static jmethodID vcmiFindClassMethod;

void AndroidVMHelper::cacheVM(JNIEnv *env)
{
    env->GetJavaVM(&vmCache);
}

void AndroidVMHelper::cacheVM(JavaVM *vm)
{
    vmCache = vm;
}

AndroidVMHelper::AndroidVMHelper()
{
    auto res = vmCache->GetEnv((void **) &envPtr, JNI_VERSION_1_1);
    if (res == JNI_EDETACHED)
    {
        auto attachRes = vmCache->AttachCurrentThread(&envPtr, nullptr);
        if (attachRes == JNI_OK)
        {
            detachInDestructor = true; // only detach if we actually attached env
        }
    }
    else
    {
        detachInDestructor = false;
    }
}

AndroidVMHelper::~AndroidVMHelper()
{
    if (envPtr && detachInDestructor)
    {
        vmCache->DetachCurrentThread();
        envPtr = nullptr;
    }
}

JNIEnv *AndroidVMHelper::get()
{
    return envPtr;
}

jclass AndroidVMHelper::findClassloadedClass(const std::string &name)
{
    auto env = get();
    return static_cast<jclass>(env->CallObjectMethod(vcmiClassLoader, vcmiFindClassMethod, env->NewStringUTF(name.c_str())));;
}

#include <android/log.h>

extern "C" JNIEXPORT void JNICALL Java_org_libsdl_app_NativeMethods_initClassloader(JNIEnv *baseEnv, jobject *cls)
{
    AndroidVMHelper::cacheVM(baseEnv);
    AndroidVMHelper envHelper;
    auto env = envHelper.get();
    auto anyVCMIClass = env->FindClass("org/libsdl/app/VCMIJavaHelpers");
    jclass classClass = env->GetObjectClass(anyVCMIClass);
    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    vcmiClassLoader = (jclass)env->NewGlobalRef(env->CallObjectMethod(anyVCMIClass, getClassLoaderMethod));
    vcmiFindClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
}











