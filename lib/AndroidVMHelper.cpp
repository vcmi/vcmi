#include "AndroidVMHelper.h"

JavaVM *vmCache = nullptr;

JNIEnv* AndroidVMHelper::attach()
{
    JNIEnv *ret;
    vmCache->AttachCurrentThread(&ret, nullptr);
    return ret;
}

void AndroidVMHelper::detach()
{
    vmCache->DetachCurrentThread();
}

void AndroidVMHelper::cacheVM(JNIEnv *env)
{
    env->GetJavaVM(&vmCache);
}

void AndroidVMHelper::cacheVM(JavaVM *vm)
{
    vmCache = vm;
}





