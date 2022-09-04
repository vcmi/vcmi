#include "playersettings.h"
#include "ui_playersettings.h"

PlayerSettings::PlayerSettings(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PlayerSettings)
{
	ui->setupUi(this);
}

PlayerSettings::~PlayerSettings()
{
	delete ui;
}
