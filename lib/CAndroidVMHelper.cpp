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
#include "ScopeGuard.h"

#include <algorithm>
#include <stdexcept>

static JavaVM * vmCache = nullptr;

/// cached java classloader so that we can find our classes from other threads
static jobject vcmiClassLoader;
static jmethodID vcmiLoadClassMethod;

static void checkJavaException(JNIEnv * env, const std::string & context)
{
	if(env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		throw std::runtime_error("JNI call failed: " + context);
	}
}

bool CAndroidVMHelper::alwaysUseLoadedClass = false;

void CAndroidVMHelper::cacheVM(JNIEnv * env)
{
	if(!env || env->GetJavaVM(&vmCache) != JNI_OK || !vmCache)
		throw std::runtime_error("Failed to obtain Android Java VM");
}

CAndroidVMHelper::CAndroidVMHelper()
	: envPtr(nullptr)
	, detachInDestructor(false)
{
	if(!vmCache)
		throw std::runtime_error("Android Java VM is not initialized");
	auto res = vmCache->GetEnv((void **) &envPtr, JNI_VERSION_1_1);
	if(res == JNI_EDETACHED)
	{
		auto attachRes = vmCache->AttachCurrentThread(&envPtr, nullptr);
		if(attachRes != JNI_OK || !envPtr)
			throw std::runtime_error("Failed to attach thread to Android Java VM");
		detachInDestructor = true;
	}
	else if(res != JNI_OK || !envPtr)
		throw std::runtime_error("Failed to obtain Android JNI environment");
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
	if(!vcmiClassLoader || !vcmiLoadClassMethod)
		throw std::runtime_error("Android application class loader is not initialized");
	std::string binaryName = name;
	std::replace(binaryName.begin(), binaryName.end(), '/', '.');
	auto javaName = env->NewStringUTF(binaryName.c_str());
	checkJavaException(env, "creating class name " + binaryName);
	if(!javaName)
		throw std::runtime_error("Failed to create Java class name " + binaryName);
	auto releaseName = vstd::makeScopeGuard([&] { env->DeleteLocalRef(javaName); });
	auto result = static_cast<jclass>(env->CallObjectMethod(vcmiClassLoader, vcmiLoadClassMethod, javaName));
	checkJavaException(env, "loading Java class " + binaryName);
	if(!result)
		throw std::runtime_error("Failed to load Java class " + binaryName);
	return result;
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
	auto releaseClass = vstd::makeScopeGuard([&] { env->DeleteLocalRef(javaHelper); });
	const auto context = cls + '.' + method;
	auto methodId = env->GetStaticMethodID(javaHelper, method.c_str(), "()Ljava/lang/String;");
	checkJavaException(env, "finding Java method " + context);
	if(!methodId)
		throw std::runtime_error("Failed to find Java method " + context);
	jstring jres = static_cast<jstring>(env->CallStaticObjectMethod(javaHelper, methodId));
	checkJavaException(env, "calling Java method " + context);
	if(!jres)
		throw std::runtime_error("Java method returned null: " + context);
	auto releaseResult = vstd::makeScopeGuard([&] { env->DeleteLocalRef(jres); });
	const char * value = env->GetStringUTFChars(jres, nullptr);
	checkJavaException(env, "reading Java result " + context);
	if(!value)
		throw std::runtime_error("Failed to read Java result " + context);
	auto releaseValue = vstd::makeScopeGuard([&] { env->ReleaseStringUTFChars(jres, value); });
	return std::string(value);
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
	if(alwaysUseLoadedClass || classloaded)
	{
		return findClassloadedClass(name);
	}
	auto env = get();
	auto result = env->FindClass(name.c_str());
	checkJavaException(env, "finding Java class " + name);
	if(!result)
		throw std::runtime_error("Failed to find Java class " + name);
	return result;
}

void CAndroidVMHelper::initClassloader(void * baseEnv)
{
	if(vcmiClassLoader && vcmiLoadClassMethod)
		return;
	CAndroidVMHelper::cacheVM(static_cast<JNIEnv *>(baseEnv));
	CAndroidVMHelper envHelper;
	auto env = envHelper.get();
	const auto frameResult = env->PushLocalFrame(8);
	checkJavaException(env, "creating application class loader reference frame");
	if(frameResult != JNI_OK)
		throw std::runtime_error("Failed to create application class loader reference frame");
	auto releaseFrame = vstd::makeScopeGuard([&] { env->PopLocalFrame(nullptr); });
	auto anyVCMIClass = envHelper.findClass(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, false);
	jclass classClass = env->GetObjectClass(anyVCMIClass);
	checkJavaException(env, "finding application Class type");
	if(!classClass)
		throw std::runtime_error("Failed to find application Class type");
	auto classLoaderClass = envHelper.findClass("java/lang/ClassLoader", false);
	auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
	checkJavaException(env, "finding Class.getClassLoader");
	if(!getClassLoaderMethod)
		throw std::runtime_error("Failed to find Class.getClassLoader");
	auto loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	checkJavaException(env, "finding ClassLoader.loadClass");
	if(!loadClassMethod)
		throw std::runtime_error("Failed to find ClassLoader.loadClass");
	auto classLoader = env->CallObjectMethod(anyVCMIClass, getClassLoaderMethod);
	checkJavaException(env, "reading application class loader");
	if(!classLoader)
		throw std::runtime_error("Application class loader is null");
	auto globalLoader = env->NewGlobalRef(classLoader);
	checkJavaException(env, "retaining application class loader");
	if(!globalLoader)
		throw std::runtime_error("Failed to retain application class loader");
	vcmiLoadClassMethod = loadClassMethod;
	vcmiClassLoader = globalLoader;
}
#endif
