/*
 * CThreadHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Sets thread name that will be used for both logs and debugger (if supported)
/// WARNING: on Unix-like systems this method should not be used for main thread since it will also change name of the process
void DLL_LINKAGE setThreadName(const std::string &name);

/// Sets thread name for use in logging only
void DLL_LINKAGE setThreadNameLoggingOnly(const std::string &name);

/// Returns human-readable thread name that was set before, or string form of system-provided thread ID if no human-readable name was set
std::string DLL_LINKAGE getThreadName();

class DLL_LINKAGE ScopedThreadName : boost::noncopyable
{
public:
	ScopedThreadName(const std::string & name)
	{
		setThreadName(name);
	}
	~ScopedThreadName()
	{
		setThreadName({});
	}
};

VCMI_LIB_NAMESPACE_END
