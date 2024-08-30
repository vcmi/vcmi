/*
 * chroniclesextractor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"

class ChroniclesExtractor : public QObject
{
	Q_OBJECT

	QWidget *parent;
	std::function<void(float percent)> cb;

	QDir tempDir;
	int extractionFile;
	int fileCount;

	bool handleTempDir(bool create);
	int getChronicleNo(QFile & file);
	bool extractGogInstaller(QString filePath);
public:
	void installChronicles(QStringList exe);

	ChroniclesExtractor(QWidget *p, std::function<void(float percent)> cb = nullptr);
};