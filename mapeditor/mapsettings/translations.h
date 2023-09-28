/*
 * translations.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include "../lib/mapping/CMap.h"

namespace Ui {
class Translations;
}

class Translations : public QDialog
{
	Q_OBJECT
	
	void fillTranslationsTable(const std::string & language);

public:
	explicit Translations(CMapHeader & mapHeader, QWidget *parent = nullptr);
	~Translations();
	
	//removes unused string IDs from map translations
	static void cleanupRemovedItems(CMap & map);
	static void cleanupRemovedItems(CMap & map, const std::string & pattern);

private slots:
	void on_languageSelect_currentIndexChanged(int index);

	void on_supportedCheck_toggled(bool checked);

	void on_translationsTable_itemChanged(QTableWidgetItem *item);

private:
	Ui::Translations *ui;
	CMapHeader & mapHeader;
	std::string mapPreferredLanguage;
};
