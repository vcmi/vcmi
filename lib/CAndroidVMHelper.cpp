/*
 * CAndroidVMHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "CAndroidVMHelper.h"

#ifdef VCMI_ANDROID
static JavaVM * vmCache = nullptr;

/// cached java classloader so that we can find our classes from other threads
static jobject vcmiClassLoader;
static jmethodID vcmiFindClassMethod;

void CAndroidVMHelper::cacheVM(JNIEnv * env)
{
	env->GetJavaVM(&vmCache);
}

CAndroidVMHelper::CAndroidVMHelper()
{
	auto res = vmCache->GetEnv((void **) &envPtr, JNI_VERSION_1_1);
	if(res == JNI_EDETACHED)
	{
		auto attachRes = vmCache->AttachCurrentThread(&envPtr, nullptr);
		if(attachRes == JNI_OK)
		{
			detachInDestructor = true; // only detach if we actually attached env
		}
	}
	else
	{
		detachInDestructor = false;
	}
}
CAndroidVMHelper::~CAndroidVMHelper()
{
	if(envPtr && detachInDestructor)
	{
		vmCache->DetachCurrentThread();
		envPtr = nullptr;
	}
}

JNIEnv * CAndroidVMHelper::get()
{
	return envPtr;
}

jclass CAndroidVMHelper::findClassloadedClass(const std::string & name)
{
	auto env = get();
	return static_cast<jclass>(env->CallObjectMethod(vcmiClassLoader, vcmiFindClassMethod,
		env->NewStringUTF(name.c_str())));
}

void CAndroidVMHelper::callStaticVoidMethod(const std::string & cls, const std::string & method,
											bool classloaded)
{
	auto env = get();
	auto javaHelper = findClass(cls, classloaded);
	auto methodId = env->GetStaticMethodID(javaHelper, method.c_str(), "()V");
	env->CallStaticVoidMethod(javaHelper, methodId);
}

std::string CAndroidVMHelper::callStaticStringMethod(const std::string & cls, const std::string & method,
													 bool classloaded)
{
	auto env = get();
	auto javaHelper = findClass(cls, classloaded);
	auto methodId = env->GetStaticMethodID(javaHelper, method.c_str(), "()Ljava/lang/String;");
	jstring jres = static_cast<jstring>(env->CallStaticObjectMethod(javaHelper, methodId));
	return std::string(env->GetStringUTFChars(jres, nullptr));
}

void CAndroidVMHelper::callCustomMethod(const std::string & cls, const std::string & method,
										const std::string & signature,
										std::function<void(JNIEnv *, jclass, jmethodID)> fun, bool classloaded)
{
	auto env = get();
	auto javaHelper = findClass(cls, classloaded);
	auto methodId = env->GetStaticMethodID(javaHelper, method.c_str(), signature.c_str());
	fun(env, javaHelper, methodId);
}

jclass CAndroidVMHelper::findClass(const std::string & name, bool classloaded)
{
	if(classloaded)
	{
		return findClassloadedClass(name);
	}
	return get()->FindClass(name.c_str());
}

extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_initClassloader(JNIEnv * baseEnv, jobject * cls)
{
	CAndroidVMHelper::cacheVM(baseEnv);
	CAndroidVMHelper envHelper;
	auto env = envHelper.get();
	auto anyVCMIClass = env->FindClass(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS);
	jclass classClass = env->GetObjectClass(anyVCMIClass);
	auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
	auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
	vcmiClassLoader = (jclass) env->NewGlobalRef(env->CallObjectMethod(anyVCMIClass, getClassLoaderMethod));
	vcmiFindClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
}
#endif
