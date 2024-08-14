/*
 * CMT.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMT.h"

#include "CGameInfo.h"
#include "mainmenu/CMainMenu.h"
#include "media/CMusicHandler.h"
#include "media/CSoundHandler.h"
#include "media/CVideoHandler.h"
#include "gui/CursorHandler.h"
#include "CPlayerInterface.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "CServerHandler.h"
#include "windows/CMessage.h"
#include "windows/InfoWindows.h"
#include "render/IScreenHandler.h"
#include "render/Graphics.h"

#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/VCMI_Lib.h"

#include "../lib/logging/CBasicLogConfigurator.h"

#include <SDL_main.h>
#include <SDL.h>

#ifdef VCMI_ANDROID
#include "../lib/CAndroidVMHelper.h"
#include <SDL_system.h>
#endif

#if __MINGW32__
#undef main
#endif
extern std::atomic<bool> headlessQuit;
extern std::optional<std::string> criticalInitializationError;
extern CBasicLogConfigurator *logConfig;

[[noreturn]] static void quitApplicationImmediately(int error_code)
{
	// Perform quick exit without executing static destructors and let OS cleanup anything that we did not
	// We generally don't care about them and this leads to numerous issues, e.g.
	// destruction of locked mutexes (fails an assertion), even in third-party libraries (as well as native libs on Android)
	// Android - std::quick_exit is available only starting from API level 21
	// Mingw, macOS and iOS - std::quick_exit is unavailable (at least in current version of CI)
#if (defined(__ANDROID_API__) && __ANDROID_API__ < 21) || (defined(__MINGW32__)) || defined(VCMI_APPLE)
	::exit(error_code);
#else
	std::quick_exit(error_code);
#endif
}

[[noreturn]] void quitApplication()
{
	CSH->endNetwork();

	if(!settings["session"]["headless"].Bool())
	{
		if(CSH->client)
			CSH->endGameplay();

		GH.windows().clear();
	}

	vstd::clear_pointer(CSH);

	CMM.reset();

	if(!settings["session"]["headless"].Bool())
	{
		// cleanup, mostly to remove false leaks from analyzer
		if(CCS)
		{
			delete CCS->consoleh;
			delete CCS->curh;
			delete CCS->videoh;
			delete CCS->musich;
			delete CCS->soundh;

			vstd::clear_pointer(CCS);
		}
		CMessage::dispose();

		vstd::clear_pointer(graphics);
	}

	vstd::clear_pointer(VLC);

	// sometimes leads to a hang. TODO: investigate
	//vstd::clear_pointer(console);// should be removed after everything else since used by logging

	if(!settings["session"]["headless"].Bool())
		GH.screenHandler().close();

	if(logConfig != nullptr)
	{
		logConfig->deconfigure();
		delete logConfig;
		logConfig = nullptr;
	}

	std::cout << "Ending...\n";
	quitApplicationImmediately(0);
}

void handleQuit(bool ask)
{
	if(!ask)
	{
		if(settings["session"]["headless"].Bool())
		{
			headlessQuit = true;
		}
		else
		{
			quitApplication();
		}

		return;
	}

	// FIXME: avoids crash if player attempts to close game while opening is still playing
	// use cursor handler as indicator that loading is not done yet
	// proper solution would be to abort init thread (or wait for it to finish)
	if (!CCS->curh)
	{
		quitApplicationImmediately(0);
	}

	if (LOCPLINT)
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[69], quitApplication, nullptr);
	else
		CInfoWindow::showYesNoDialog(CGI->generaltexth->allTexts[69], {}, quitApplication, {}, PlayerColor(1));
}

void handleFatalError(const std::string & message, bool terminate)
{
	logGlobal->error("FATAL ERROR ENCOUNTERED, VCMI WILL NOW TERMINATE");
	logGlobal->error("Reason: %s", message);

	std::string messageToShow = "Fatal error! " + message;

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error!", messageToShow.c_str(), nullptr);

	if (terminate)
		throw std::runtime_error(message);
	else
		quitApplicationImmediately(1);
}
