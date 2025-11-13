/*
 * prepare.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "prepare.h"
#include "prepare_p.h"
#include "../vcmiqt/launcherdirs.h"

namespace launcher
{
void prepare()
{
#ifdef VCMI_ANDROID
	prepareAndroid();
#elif defined(VCMI_IOS)
	prepareIos();
#endif

	CLauncherDirs::prepare();
}
}
