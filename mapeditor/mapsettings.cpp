#include "mapsettings.h"
#include "ui_mapsettings.h"

MapSettings::MapSettings(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings)
{
	ui->setupUi(this);
}

MapSettings::~MapSettings()
{
	delete ui;
}
