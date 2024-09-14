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
	int getChronicleNo(QFile & file);
	bool extractGogInstaller(QString filePath);
	void createBaseMod() const;
	void createChronicleMod(int no);
	void extractFiles(int no) const;

	const std::map<int, QByteArray> chronicles = {
		{1, QByteArray{reinterpret_cast<const char*>(u"Warlords of the Wasteland"), 50}},
		{2, QByteArray{reinterpret_cast<const char*>(u"Conquest of the Underworld"), 52}},
		{3, QByteArray{reinterpret_cast<const char*>(u"Masters of the Elements"), 46}},
		{4, QByteArray{reinterpret_cast<const char*>(u"Clash of the Dragons"), 40}},
		{5, QByteArray{reinterpret_cast<const char*>(u"The World Tree"), 28}},
		{6, QByteArray{reinterpret_cast<const char*>(u"The Fiery Moon"), 28}},
		{7, QByteArray{reinterpret_cast<const char*>(u"Revolt of the Beastmasters"), 52}},
		{8, QByteArray{reinterpret_cast<const char*>(u"The Sword of Frost"), 36}}
	};
public:
	void installChronicles(QStringList exe);

	ChroniclesExtractor(QWidget *p, std::function<void(float percent)> cb = nullptr);
};
