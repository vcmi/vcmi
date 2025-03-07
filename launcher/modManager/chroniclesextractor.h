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

	bool createTempDir();
	void removeTempDir();
	std::vector<int> getChronicleNo();
	bool extractGogInstaller(QString filePath);
	void createBaseMod() const;
	void createChronicleMod(int no);
	void extractFiles(int no) const;

	const QStringList chronicles = {
		{}, // fake	0th "chronicle", to create 1-based list
		"Warlords of the Wasteland",
		"Conquest of the Underworld",
		"Masters of the Elements",
		"Clash of the Dragons",
		"The World Tree",
		"The Fiery Moon",
		"Revolt of the Beastmasters",
		"The Sword of Frost",
	};
public:
	void installChronicles(QStringList exe);

	ChroniclesExtractor(QWidget *p, std::function<void(float percent)> cb = nullptr);
};
