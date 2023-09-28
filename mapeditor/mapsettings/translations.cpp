/*
 * translations.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "translations.h"
#include "ui_translations.h"

Translations::Translations(CMapHeader & mh, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Translations),
	mapHeader(mh)
{
	ui->setupUi(this);
}

Translations::~Translations()
{
	delete ui;
}

void Translations::on_languageSelect_currentIndexChanged(int index)
{

}


void Translations::on_supportedCheck_toggled(bool checked)
{

}


void Translations::on_translationsTable_itemChanged(QTableWidgetItem *item)
{

}

