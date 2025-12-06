/*
 * hdextractor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"

class HdExtractor : public QObject
{
	Q_OBJECT

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

	const std::map<ArchiveType, QString> pakfiles = {
		{BITMAP_X2, "bitmap_DXT_com_x2.pak"},
		{BITMAP_X3, "bitmap_DXT_com_x3.pak"},
		{SPRITE_X2, "sprite_DXT_com_x2.pak"},
		{SPRITE_X3, "sprite_DXT_com_x3.pak"},
		{BITMAP_LOC_X2, "bitmap_DXT_loc_x2.pak"},
		{BITMAP_LOC_X3, "bitmap_DXT_loc_x3.pak"},
		{SPRITE_LOC_X2, "sprite_DXT_loc_x2.pak"},
		{SPRITE_LOC_X3, "sprite_DXT_loc_x3.pak"}
	};

	const QMap<QString, QString> languages = {
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

	const std::map<SubModType, QString> submodnames = {
		{X2, "x2"},
		{X3, "x3"},
		{LOC_X2, "x2_loc"},
		{LOC_X3, "x3_loc"}
	};

	QWidget *parent;

	SubModType archiveTypeToSubModType(ArchiveType v);
	void createModJson(std::optional<SubModType> submodType, QDir path, QString language);
public:
	void installHd();

	HdExtractor(QWidget *p);
};
