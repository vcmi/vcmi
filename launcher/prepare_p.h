/*
 * prepare_p.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

namespace launcher
{
#ifdef VCMI_ANDROID
void prepareAndroid();
#elif defined(VCMI_IOS)
void prepareIos();
#endif
}
