#include "StdInc.h"
#include "generalsettings.h"
#include "ui_generalsettings.h"

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

void GeneralSettings::initialize(const CMap & map)
{
	ui->mapNameEdit->setText(tr(map.name.c_str()));
	ui->mapDescriptionEdit->setPlainText(tr(map.description.c_str()));
	ui->heroLevelLimit->setValue(map.levelLimit);
	ui->heroLevelLimitCheck->setChecked(map.levelLimit);

	//set difficulty
	switch(map.difficulty)
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

void GeneralSettings::update(CMap & map)
{
	map.name = ui->mapNameEdit->text().toStdString();
	map.description = ui->mapDescriptionEdit->toPlainText().toStdString();
	if(ui->heroLevelLimitCheck->isChecked())
		map.levelLimit = ui->heroLevelLimit->value();
	else
		map.levelLimit = 0;

	//set difficulty
	if(ui->diffRadio1->isChecked()) map.difficulty = 0;
	if(ui->diffRadio2->isChecked()) map.difficulty = 1;
	if(ui->diffRadio3->isChecked()) map.difficulty = 2;
	if(ui->diffRadio4->isChecked()) map.difficulty = 3;
	if(ui->diffRadio5->isChecked()) map.difficulty = 4;
}

void GeneralSettings::on_heroLevelLimitCheck_toggled(bool checked)
{
	ui->heroLevelLimit->setEnabled(checked);
}

