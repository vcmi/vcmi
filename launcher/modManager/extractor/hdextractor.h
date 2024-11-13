/*
 * hdsextractor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../StdInc.h"

class CZipSaver;

class HdExtractor : public QObject
{
	enum ArchiveType
	{
		BITMAP_X2,
		BITMAP_X3,
		SPRITE_X2,
		SPRITE_X3,
		BITMAP_LOC_X2,
		BITMAP_LOC_X3,
		SPRITE_LOC_X2,
		SPRITE_LOC_X3
	};

	enum SubModType
	{
		X2,
		X3,
		LOC_X2,
		LOC_X3
	};

	std::map<ArchiveType, std::string> PAKPATH = {
		{BITMAP_X2, "bitmap_DXT_com_x2.pak"},
		{BITMAP_X3, "bitmap_DXT_com_x3.pak"},
		{SPRITE_X2, "sprite_DXT_com_x2.pak"},
		{SPRITE_X3, "sprite_DXT_com_x3.pak"},
		{BITMAP_LOC_X2, "bitmap_DXT_loc_x2.pak"},
		{BITMAP_LOC_X3, "bitmap_DXT_loc_x3.pak"},
		{SPRITE_LOC_X2, "sprite_DXT_loc_x2.pak"},
		{SPRITE_LOC_X3, "sprite_DXT_loc_x3.pak"}
	};

	std::map<std::string, std::string> LANGUAGES = {
		{"CH", "chinese"},
		{"CZ", "czech"},
		{"DE", "german"},
		{"EN", "english"},
		{"ES", "spanish"},
		{"FR", "french"},
		{"IT", "italian"},
		{"PL", "polish"},
		{"RU", "russian"}
	};

	const int FILES = 71110;

	Q_OBJECT

	QWidget *parent;
	std::function<void(float percent)> cb;
	QDir tempDir;
	QDir baseDir;
	class ModGenerator
	{
		std::map<SubModType, std::string> SUBMODNAMES = {
			{X2, "x2"},
			{X3, "x3"},
			{LOC_X2, "x2_loc"},
			{LOC_X3, "x3_loc"}
		};

		std::string language;
		std::map<std::string, std::vector<std::vector<std::string>>> sdData;

		std::map<std::string, std::vector<std::string>> flagData;
		QImage flag2x;
		QImage flag3x;

		std::map<SubModType, std::shared_ptr<CZipSaver>> savers;
	public:
		ModGenerator(QDir outputDir, std::string language);
		void addFile(QString groupName, QString imageName, QImage & img, ArchiveType type);
		void createModJson(std::optional<SubModType> submodType, QDir path, std::string language);
		void createAnimationJson(QString name, std::vector<QString> files, ArchiveType type);
		QImage resizeSprite(QImage & img, QString groupName, QString imageName, ArchiveType type, int * sdOffsets);
		QImage addFlags(QImage & img, QString groupName, QString imageName, int factor);
		bool filter(QString groupName, ArchiveType type);
		QImage getBorder(QImage & img, QRgb c, int window);
		void loadFlagData(QDir path);
	};
	
	void extract();
	void decodePac(QFile & file, std::function<void(QString, QString, std::vector<QByteArray> &)> cb);
	void cropImages(std::vector<QImage> & images, QString imageParam, std::function<void(QString, QImage &, int[2], bool)> cb);
	QImage loadDds(QByteArray & data);
	bool createTempDir();
	void removeTempDir();
public:
	void installHd(QDir path);
	HdExtractor(QWidget *p, std::function<void(float percent)> cb = nullptr);
	static SubModType archiveTypeToSubModType(ArchiveType v);
};
