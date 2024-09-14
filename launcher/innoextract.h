/*
 * innoextract.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class Innoextract : public QObject
{
public:
	static QString extract(QString installer, QString outDir, std::function<void (float percent)> cb = nullptr);
};
