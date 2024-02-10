/*
 * EntryPoint.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../server/CVCMIServer.h"

#include "../lib/CConsoleHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/VCMIDirs.h"
#include "../lib/VCMI_Lib.h"

#ifdef VCMI_ANDROID
#include <jni.h>
#include <android/log.h>
#include "lib/CAndroidVMHelper.h"
#endif

#include <boost/program_options.hpp>

const std::string SERVER_NAME_AFFIX = "server";
const std::string SERVER_NAME = GameConstants::VCMI_VERSION + std::string(" (") + SERVER_NAME_AFFIX + ')';

static void handleCommandOptions(int argc, const char * argv[], boost::program_options::variables_map & options)
{
	namespace po = boost::program_options;
	po::options_description opts("Allowed options");
	opts.add_options()
	("help,h", "display help and exit")
	("version,v", "display version information and exit")
	("run-by-client", "indicate that server launched by client on same machine")
	("port", po::value<ui16>(), "port at which server will listen to connections from client")
	("lobby", "start server in lobby mode in which server connects to a global lobby");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), options);
		}
		catch(boost::program_options::error & e)
		{
			std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		}
	}

#ifdef SINGLE_PROCESS_APP
	options.emplace("run-by-client", po::variable_value{true, true});
#endif

	po::notify(options);

#ifndef SINGLE_PROCESS_APP
	if(options.count("help"))
	{
		auto time = std::time(nullptr);
		printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
		printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", std::localtime(&time)->tm_year + 1900);
		printf("This is free software; see the source for copying conditions. There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		printf("\n");
		std::cout << opts;
		exit(0);
	}

	if(options.count("version"))
	{
		printf("%s\n", GameConstants::VCMI_VERSION.c_str());
		std::cout << VCMIDirs::get().genHelpString();
		exit(0);
	}
#endif
}

#ifdef SINGLE_PROCESS_APP
#define main server_main
#endif

#if VCMI_ANDROID_DUAL_PROCESS
void CVCMIServer::create()
{
	const int argc = 1;
	const char * argv[argc] = { "android-server" };
#else
int main(int argc, const char * argv[])
{
#endif

#if !defined(VCMI_ANDROID) && !defined(SINGLE_PROCESS_APP)
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
	boost::filesystem::current_path(boost::filesystem::system_complete(argv[0]).parent_path());
#endif

#ifndef VCMI_IOS
	console = new CConsoleHandler();
#endif
	CBasicLogConfigurator logConfig(VCMIDirs::get().userLogsPath() / "VCMI_Server_log.txt", console);
	logConfig.configureDefault();
	logGlobal->info(SERVER_NAME);

	boost::program_options::variables_map opts;
	handleCommandOptions(argc, argv, opts);
	preinitDLL(console, false);
	logConfig.configure();

	loadDLLClasses();
	std::srand(static_cast<uint32_t>(time(nullptr)));

	{
		CVCMIServer server(opts);

#ifdef SINGLE_PROCESS_APP
		boost::condition_variable * cond = reinterpret_cast<boost::condition_variable *>(const_cast<char *>(argv[0]));
		cond->notify_one();
#endif

		server.run();

		// CVCMIServer destructor must be called here - before VLC cleanup
	}


#if VCMI_ANDROID_DUAL_PROCESS
	CAndroidVMHelper envHelper;
	envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "killServer");
#endif
	logConfig.deconfigure();
	vstd::clear_pointer(VLC);

#if !VCMI_ANDROID_DUAL_PROCESS
	return 0;
#endif
}

#if VCMI_ANDROID_DUAL_PROCESS
extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_createServer(JNIEnv * env, jclass cls)
{
	__android_log_write(ANDROID_LOG_INFO, "VCMI", "Got jni call to init server");
	CAndroidVMHelper::cacheVM(env);

	CVCMIServer::create();
}

extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_initClassloader(JNIEnv * baseEnv, jclass cls)
{
	CAndroidVMHelper::initClassloader(baseEnv);
}
#elif defined(SINGLE_PROCESS_APP)
void CVCMIServer::create(boost::condition_variable * cond, const std::vector<std::string> & args)
{
	std::vector<const void *> argv = {cond};
	for(auto & a : args)
		argv.push_back(a.c_str());
	main(argv.size(), reinterpret_cast<const char **>(&*argv.begin()));
}

#ifdef VCMI_ANDROID
void CVCMIServer::reuseClientJNIEnv(void * jniEnv)
{
	CAndroidVMHelper::initClassloader(jniEnv);
	CAndroidVMHelper::alwaysUseLoadedClass = true;
}
#endif // VCMI_ANDROID
#endif // VCMI_ANDROID_DUAL_PROCESS
