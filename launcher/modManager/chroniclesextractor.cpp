/*
 * chroniclesextractor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "chroniclesextractor.h"

void ChroniclesExtractor::installExe(QWidget *parent, QStringList exe)
{
	for(QString f : exe)
	{
		QFile file(f);
		if(!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(parent, tr("File cannot opened"), file.errorString());
			continue;
		}

		QByteArray magic{"MZ"};
		QByteArray magicFile = file.read(magic.length());
		if(!magicFile.startsWith(magic))
		{
			QMessageBox::critical(parent, tr("Invalid file selected"), tr("You have to select an gog installer file!"));
			continue;
		}

		QByteArray dataBegin = file.read(10'000'000);
		const std::map<int, QByteArray> chronicles = {
			{1, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Warlords of the Wasteland"), 90}},
			{2, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Conquest of the Underworld"), 92}},
			{3, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Masters of the Elements"), 86}},
			{4, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Clash of the Dragons"), 80}},
			{5, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - The World Tree"), 68}},
			{6, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - The Fiery Moon"), 68}},
			{7, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Revolt of the Beastmasters"), 92}},
			{8, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - The Sword of Frost"), 76}}
		};
		int chronicle = 0;
		for (const auto& kv : chronicles) {
			if(dataBegin.contains(kv.second))
			{
				chronicle = kv.first;
				break;
			}
		}
		if(!chronicle)
		{
			QMessageBox::critical(parent, tr("Invalid file selected"), tr("You have to select an chronicle installer file!"));
			continue;
		}
	}
}