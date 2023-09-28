/*
 * translations.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "translations.h"
#include "ui_translations.h"
#include "../../lib/Languages.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/VCMI_Lib.h"

Translations::Translations(CMapHeader & mh, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Translations),
	mapHeader(mh)
{
	ui->setupUi(this);
	
	//fill languages list
	for(auto & language : Languages::getLanguageList())
	{
		ui->languageSelect->addItem(QString("%1 (%2)").arg(QString::fromStdString(language.nameEnglish), QString::fromStdString(language.nameNative)));
		ui->languageSelect->setItemData(ui->languageSelect->count() - 1, QVariant(QString::fromStdString(language.identifier)));
		if(language.identifier == VLC->generaltexth->getPreferredLanguage())
			ui->languageSelect->setCurrentIndex(ui->languageSelect->count() - 1);
	}
}

Translations::~Translations()
{
	delete ui;
}

void Translations::on_languageSelect_currentIndexChanged(int index)
{
	auto language = ui->languageSelect->currentData().toString().toStdString();
	auto & translation = mapHeader.translations[language];
	bool hasLanguage = !translation.isNull();
	ui->supportedCheck->blockSignals(true);
	ui->supportedCheck->setChecked(hasLanguage);
	ui->supportedCheck->blockSignals(false);
	ui->translationsTable->setEnabled(hasLanguage);
	if(hasLanguage)
	{
		ui->translationsTable->blockSignals(true);
		ui->translationsTable->setRowCount(translation.Struct().size());
		int i = 0;
		for(auto & s : translation.Struct())
		{
			auto * wId = new QTableWidgetItem(QString::fromStdString(s.first));
			auto * wText = new QTableWidgetItem(QString::fromStdString(s.second.String()));
			wId->setFlags(wId->flags() & ~Qt::ItemIsEditable);
			wText->setFlags(wId->flags() | Qt::ItemIsEditable);
			ui->translationsTable->setItem(i, 0, wId);
			ui->translationsTable->setItem(i++, 0, wText);
		}
		ui->translationsTable->blockSignals(false);
	}
}


void Translations::on_supportedCheck_toggled(bool checked)
{
	auto language = ui->languageSelect->currentData().toString().toStdString();
	auto & translation = mapHeader.translations[language];
	bool hasLanguage = !translation.isNull();
	bool hasRecord = !translation.Struct().empty();
	
	if(checked)
	{
		if(!hasLanguage)
			translation = JsonNode(JsonNode::JsonType::DATA_STRUCT);
		
		//copy from default language
		translation = mapHeader.translations[VLC->generaltexth->getPreferredLanguage()];
		hasLanguage = !translation.isNull();
		
		ui->translationsTable->blockSignals(true);
		ui->translationsTable->setRowCount(translation.Struct().size());
		int i = 0;
		for(auto & s : translation.Struct())
		{
			auto * wId = new QTableWidgetItem(QString::fromStdString(s.first));
			auto * wText = new QTableWidgetItem(QString::fromStdString(s.second.String()));
			wId->setFlags(wId->flags() & ~Qt::ItemIsEditable);
			wText->setFlags(wId->flags() | Qt::ItemIsEditable);
			ui->translationsTable->setItem(i, 0, wId);
			ui->translationsTable->setItem(i++, 0, wText);
		}
		ui->translationsTable->blockSignals(false);
		ui->translationsTable->setEnabled(hasLanguage);
	}
	else
	{
		bool canRemove = language != VLC->generaltexth->getPreferredLanguage();
		if(!canRemove)
		{
			QMessageBox::information(this, tr("Remove translation"), tr("Default language cannot be removed"));
		}
		else if(hasRecord)
		{
			auto sure = QMessageBox::question(this, tr("Remove translation"), tr("This language has text records which will be removed. Continue?"));
			canRemove = sure != QMessageBox::No;
		}
		
		if(!canRemove)
		{
			ui->supportedCheck->blockSignals(true);
			ui->supportedCheck->setChecked(true);
			ui->supportedCheck->blockSignals(false);
			return;
		}
		ui->translationsTable->blockSignals(true);
		ui->translationsTable->clear();
		translation = JsonNode();
		ui->translationsTable->blockSignals(false);
		ui->translationsTable->setEnabled(false);
	}
}


void Translations::on_translationsTable_itemChanged(QTableWidgetItem *item)
{

}

