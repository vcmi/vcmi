/*
 * jsonutils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "helper.h"

#include "../lib/CConfigHandler.h"

#include <QObject>
#include <QScroller>

namespace Helper
{
void loadSettings()
{
	settings.init("config/settings.json", "vcmi:settings");
}

void enableScrollBySwiping(QObject * scrollTarget)
{
#ifdef VCMI_MOBILE
	QScroller::grabGesture(scrollTarget, QScroller::LeftMouseButtonGesture);
#endif
}
}
