/*
 * CAndroidVMHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Global.h"

#ifdef VCMI_ANDROID

#include <jni.h>
#include <string>

/// helper class that allows access to java vm to communicate with java code from native
class CAndroidVMHelper
{
	JNIEnv * envPtr;
	bool detachInDestructor;

	jclass findClass(const std::string & name, bool classloaded);

public:
	CAndroidVMHelper();

	~CAndroidVMHelper();

	JNIEnv * get();

	jclass findClassloadedClass(const std::string & name);

	void callStaticVoidMethod(const std::string & cls, const std::string & method, bool classloaded = false);

	std::string callStaticStringMethod(const std::string & cls, const std::string & method, bool classloaded = false);

	void callCustomMethod(const std::string & cls, const std::string & method, const std::string & signature,
						  std::function<void(JNIEnv *, jclass, jmethodID)> fun, bool classloaded = false);

	static void cacheVM(JNIEnv * env);

	static constexpr const char * NATIVE_METHODS_DEFAULT_CLASS = "eu/vcmi/vcmi/NativeMethods";
};

#endif
