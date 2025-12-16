/*
 * hdextractor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "hdextractor.h"

#include "../../lib/VCMIDirs.h"

HdExtractor::HdExtractor(QWidget *p) :
	parent(p)
{
}

HdExtractor::SubModType HdExtractor::archiveTypeToSubModType(ArchiveType v)
{
	SubModType subModType = SubModType::X2;
	if(vstd::contains({ArchiveType::BITMAP_X2, ArchiveType::SPRITE_X2}, v))
		subModType = SubModType::X2;
	else if(vstd::contains({ArchiveType::BITMAP_X3, ArchiveType::SPRITE_X3}, v))
		subModType = SubModType::X3;
	else if(vstd::contains({ArchiveType::BITMAP_LOC_X2, ArchiveType::SPRITE_LOC_X2}, v))
		subModType = SubModType::LOC_X2;
	else if(vstd::contains({ArchiveType::BITMAP_LOC_X3, ArchiveType::SPRITE_LOC_X3}, v))
		subModType = SubModType::LOC_X3;

	return subModType;
};

void HdExtractor::installHd()
{
	QString tmpDir = QFileDialog::getExistingDirectory(parent, tr("Select Directory with HD Edition (Steam folder)"), QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(tmpDir.isEmpty())
		return;

	QDir dir(tmpDir);

	if(!dir.exists("HOMM3 2.0.exe"))
	{
		QMessageBox::critical(parent, tr("Invalid folder"), tr("The selected folder does not contain HOMM3 2.0.exe! Please select the HD Edition installation folder."));
		return;
	}

	QString language = "";
	auto folderList = QDir(QDir::cleanPath(dir.absolutePath() + QDir::separator() + "data/LOC")).entryList(QDir::Filter::Dirs);
	for(auto lng : languages.keys())
		for(auto folder : folderList)
			if(lng == folder)
				language = lng;
	
	QDir dst(pathToQString(VCMIDirs::get().userDataPath() / "Mods" / "hd-edition"));
	dst.mkpath(dst.path());
	createModJson(std::nullopt, dst, language);

	QDir dstData(dst.filePath("content/data"));
	dstData.mkpath(".");
	QFile::copy(dir.filePath("data/spriteFlagsInfo.txt"), dstData.filePath("spriteFlagsInfo.txt"));

	QDir dstDataFlags(dst.filePath("content/data/flags"));
	dstDataFlags.mkpath(".");
	for (const QFileInfo &fileInfo : QDir(dir.filePath("data/flags")).entryInfoList(QDir::Files))
	{
		QString srcFile = fileInfo.absoluteFilePath();
		QString destFile = dstDataFlags.filePath(fileInfo.fileName());
		QFile::copy(srcFile, destFile);
	}

	for(auto modType : {X2, X3, LOC_X2, LOC_X3})
	{
		QString suffix = (language.isEmpty() || modType == SubModType::X2 || modType == SubModType::X3) ? "" : "_" + language;
		QDir modPath = QDir(dst.filePath(QString("mods/") + submodnames.at(modType) + suffix));
		modPath.mkpath(modPath.path());
		createModJson(modType, modPath, languages[language]);

		QDir contentDataDir(modPath.filePath("content/data"));
    	contentDataDir.mkpath(".");

		for(auto & type : {ArchiveType::BITMAP_X2, ArchiveType::BITMAP_X3, ArchiveType::SPRITE_X2, ArchiveType::SPRITE_X3, ArchiveType::BITMAP_LOC_X2, ArchiveType::BITMAP_LOC_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3})
		{
			if(archiveTypeToSubModType(type) != modType)
				continue;

			QFile fileName;
			if(vstd::contains({ArchiveType::BITMAP_LOC_X2, ArchiveType::BITMAP_LOC_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3}, type))
				fileName.setFileName(dir.filePath(QString("data/LOC/") + language + "/" + pakfiles.at(type)));
			else
				fileName.setFileName(dir.filePath(QString("data/") + pakfiles.at(type)));

			QString destPath = contentDataDir.filePath(QFileInfo(fileName).fileName());
			fileName.copy(contentDataDir.filePath(fileName.fileName()));
			if(!fileName.copy(destPath))
				QMessageBox::critical(parent, tr("Extraction error"), tr("Please delete mod and try again! Failed to copy file %1 to %2").arg(fileName.fileName(), destPath), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}

void HdExtractor::createModJson(std::optional<SubModType> submodType, QDir path, QString language)
{
	if (auto result = submodType)
	{
		QString scale = (*result == SubModType::X2 || *result == SubModType::LOC_X2) ? "2" : "3";
		bool isTranslation = (*result == SubModType::LOC_X2 || *result == SubModType::LOC_X3);
		QJsonObject mod;
		if(isTranslation)
		{
			mod = QJsonObject({
				{ "modType", "Translation" },
				{ "name", "HD Localisation (" + language + ") (x" + scale + ")" },
				{ "description", "Translated Resources (x" + scale + ")" },
				{ "author", "Ubisoft" },
				{ "version", "1.0" },
				{ "contact", "vcmi.eu" },
				{ "language", language },
			});
		}
		else
		{
			mod = QJsonObject({
				{ "modType", "Graphical" },
				{ "name", "HD (x" + scale + ")" },
				{ "description", "Resources (x" + scale + ")" },
				{ "author", "Ubisoft" },
				{ "version", "1.0" },
				{ "contact", "vcmi.eu" },
			});
		}
		
		QFile jsonFile(path.filePath("mod.json"));
		jsonFile.open(QFile::WriteOnly);
		jsonFile.write(QJsonDocument(mod).toJson());
	}
	else
	{
		QJsonObject mod
		{
			{ "modType", "Graphical" },
			{ "name", "Heroes III HD Edition" },
			{ "description", "Extracted resources from official Heroes HD to make it usable on VCMI" },
			{ "author", "Ubisoft" },
			{ "version", "1.0" },
			{ "contact", "vcmi.eu" },
		};
		
		QFile jsonFile(path.filePath("mod.json"));
		jsonFile.open(QFile::WriteOnly);
		jsonFile.write(QJsonDocument(mod).toJson());
	}
}
