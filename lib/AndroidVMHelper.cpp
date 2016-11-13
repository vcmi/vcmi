#include "AndroidVMHelper.h"

static JavaVM *vmCache = nullptr;

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
    auto res = vmCache->GetEnv((void**)&envPtr, JNI_VERSION_1_1);
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











