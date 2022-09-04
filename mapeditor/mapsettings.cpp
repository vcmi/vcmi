#include "mapsettings.h"
#include "ui_mapsettings.h"
#include "mainwindow.h"

MapSettings::MapSettings(MainWindow *parent) :
	QDialog(static_cast<QWidget*>(parent)),
	ui(new Ui::MapSettings),
	main(parent)
{
	ui->setupUi(this);

	assert(main);
	assert(main->getMap());

	ui->mapNameEdit->setText(QString::fromStdString(main->getMap()->name));
	ui->mapDescriptionEdit->setPlainText(QString::fromStdString(main->getMap()->description));

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
	main->getMap()->name = ui->mapNameEdit->text().toStdString();
	main->getMap()->description = ui->mapDescriptionEdit->toPlainText().toStdString();
	close();
}
