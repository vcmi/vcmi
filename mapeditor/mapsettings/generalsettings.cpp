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
	ui->heroLevelLimit->setValue(controller->map()->levelLimit);
	ui->heroLevelLimitCheck->setChecked(controller->map()->levelLimit);

	//set difficulty
	switch(controller->map()->difficulty)
	{
		case 0:
			ui->diffRadio1->setChecked(true);
			break;

		case 1:
			ui->diffRadio2->setChecked(true);
			break;

		case 2:
			ui->diffRadio3->setChecked(true);
			break;

		case 3:
			ui->diffRadio4->setChecked(true);
			break;

		case 4:
			ui->diffRadio5->setChecked(true);
			break;
	};
}

void GeneralSettings::update()
{
	controller->map()->name = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "name"),  ui->mapNameEdit->text().toStdString()));
	controller->map()->description = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "description"), ui->mapDescriptionEdit->toPlainText().toStdString()));
	if(ui->heroLevelLimitCheck->isChecked())
		controller->map()->levelLimit = ui->heroLevelLimit->value();
	else
		controller->map()->levelLimit = 0;

	//set difficulty
	if(ui->diffRadio1->isChecked()) controller->map()->difficulty = 0;
	if(ui->diffRadio2->isChecked()) controller->map()->difficulty = 1;
	if(ui->diffRadio3->isChecked()) controller->map()->difficulty = 2;
	if(ui->diffRadio4->isChecked()) controller->map()->difficulty = 3;
	if(ui->diffRadio5->isChecked()) controller->map()->difficulty = 4;
}

void GeneralSettings::on_heroLevelLimitCheck_toggled(bool checked)
{
	ui->heroLevelLimit->setEnabled(checked);
}

