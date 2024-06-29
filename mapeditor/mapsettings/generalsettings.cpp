/*
 * generalsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "generalsettings.h"
#include "ui_generalsettings.h"
#include "../mapcontroller.h"

GeneralSettings::GeneralSettings(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::GeneralSettings)
{
	ui->setupUi(this);
}

GeneralSettings::~GeneralSettings()
{
	delete ui;
}

void GeneralSettings::initialize(MapController & c)
{
	AbstractSettings::initialize(c);
	ui->mapNameEdit->setText(QString::fromStdString(controller->map()->name.toString()));
	ui->mapDescriptionEdit->setPlainText(QString::fromStdString(controller->map()->description.toString()));
	ui->authorEdit->setText(QString::fromStdString(controller->map()->author.toString()));
	ui->authorContactEdit->setText(QString::fromStdString(controller->map()->authorContact.toString()));
	ui->mapCreationDateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(controller->map()->creationDateTime));
	ui->mapVersionEdit->setText(QString::fromStdString(controller->map()->mapVersion.toString()));
	ui->heroLevelLimit->setValue(controller->map()->levelLimit);
	ui->heroLevelLimitCheck->setChecked(controller->map()->levelLimit);

	//set difficulty
	switch(controller->map()->difficulty)
	{
		case EMapDifficulty::EASY:
			ui->diffRadio1->setChecked(true);
			break;

		case EMapDifficulty::NORMAL:
			ui->diffRadio2->setChecked(true);
			break;

		case EMapDifficulty::HARD:
			ui->diffRadio3->setChecked(true);
			break;

		case EMapDifficulty::EXPERT:
			ui->diffRadio4->setChecked(true);
			break;

		case EMapDifficulty::IMPOSSIBLE:
			ui->diffRadio5->setChecked(true);
			break;
	};
}

void GeneralSettings::update()
{
	controller->map()->name = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "name"),  ui->mapNameEdit->text().toStdString()));
	controller->map()->description = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "description"), ui->mapDescriptionEdit->toPlainText().toStdString()));
	controller->map()->author = MetaString::createFromRawString(ui->authorEdit->text().toStdString());
	controller->map()->authorContact = MetaString::createFromRawString(ui->authorContactEdit->text().toStdString());
	controller->map()->creationDateTime = ui->mapCreationDateTimeEdit->dateTime().toSecsSinceEpoch();
	controller->map()->mapVersion = MetaString::createFromRawString(ui->mapVersionEdit->text().toStdString());
	if(ui->heroLevelLimitCheck->isChecked())
		controller->map()->levelLimit = ui->heroLevelLimit->value();
	else
		controller->map()->levelLimit = 0;

	//set difficulty
	if(ui->diffRadio1->isChecked()) controller->map()->difficulty = EMapDifficulty::EASY;
	if(ui->diffRadio2->isChecked()) controller->map()->difficulty = EMapDifficulty::NORMAL;
	if(ui->diffRadio3->isChecked()) controller->map()->difficulty = EMapDifficulty::HARD;
	if(ui->diffRadio4->isChecked()) controller->map()->difficulty = EMapDifficulty::EXPERT;
	if(ui->diffRadio5->isChecked()) controller->map()->difficulty = EMapDifficulty::IMPOSSIBLE;
}

void GeneralSettings::on_heroLevelLimitCheck_toggled(bool checked)
{
	ui->heroLevelLimit->setEnabled(checked);
}

