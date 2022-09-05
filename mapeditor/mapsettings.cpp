#include "mapsettings.h"
#include "ui_mapsettings.h"
#include "mainwindow.h"

MapSettings::MapSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings),
	controller(ctrl)
{
	ui->setupUi(this);

	assert(controller.map());

	ui->mapNameEdit->setText(QString::fromStdString(controller.map()->name));
	ui->mapDescriptionEdit->setPlainText(QString::fromStdString(controller.map()->description));

	show();

	//ui8 difficulty;
	//ui8 levelLimit;

	//std::string victoryMessage;
	//std::string defeatMessage;
	//ui16 victoryIconIndex;
	//ui16 defeatIconIndex;

	//std::vector<PlayerInfo> players; /// The default size of the vector is PlayerColor::PLAYER_LIMIT.
}

MapSettings::~MapSettings()
{
	delete ui;
}

void MapSettings::on_pushButton_clicked()
{
	controller.map()->name = ui->mapNameEdit->text().toStdString();
	controller.map()->description = ui->mapDescriptionEdit->toPlainText().toStdString();
	controller.commitChangeWithoutRedraw();
	close();
}
