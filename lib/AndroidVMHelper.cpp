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

void AndroidVMHelper::callStaticVoidMethod(const std::string &cls, const std::string &method, bool classloaded /*=false*/)
{
    auto env = get();
    auto javaHelper = findClass(cls, classloaded);
    auto methodId = env->GetStaticMethodID(javaHelper, method.c_str(), "()V");
    env->CallStaticVoidMethod(javaHelper, methodId);
}

std::string AndroidVMHelper::callStaticStringMethod(const std::string &cls, const std::string &method, bool classloaded /*=false*/)
{
    auto env = get();
    auto javaHelper = findClass(cls, classloaded);
    auto methodId = env->GetStaticMethodID(javaHelper, method.c_str(), "()Ljava/lang/String;");
    jstring jres = static_cast<jstring>(env->CallStaticObjectMethod(javaHelper, methodId));
    return std::string(env->GetStringUTFChars(jres, nullptr));
}

jclass AndroidVMHelper::findClass(const std::string &name, bool classloaded)
{
    if (classloaded)
    {
        return findClassloadedClass(name);
    }
    return get()->FindClass(name.c_str());
}

extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_initClassloader(JNIEnv *baseEnv, jobject *cls)
{
    AndroidVMHelper::cacheVM(baseEnv);
    AndroidVMHelper envHelper;
    auto env = envHelper.get();
    auto anyVCMIClass = env->FindClass(AndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS);
    jclass classClass = env->GetObjectClass(anyVCMIClass);
    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    vcmiClassLoader = (jclass) env->NewGlobalRef(env->CallObjectMethod(anyVCMIClass, getClassLoaderMethod));
    vcmiFindClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
}
