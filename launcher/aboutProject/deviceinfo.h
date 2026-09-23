/*
 * deviceinfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"

class DeviceInfo final
{
private:
	static QString formatBytes(quint64 bytes);
	static QString readProcValue(const QString & fileName, const QStringList & keys);
	static QByteArray sysctlData(const char * name);
	static QString cpuName();
	static QString cpuFrequency();
	static std::optional<quint64> totalMemory();

public:
	static QString generateReport();
};
