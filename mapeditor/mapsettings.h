#pragma once

#include <QDialog>
#include "mapcontroller.h"

namespace Ui {
class MapSettings;
}

class MapSettings : public QDialog
{
	Q_OBJECT

public:
	explicit MapSettings(MapController & controller, QWidget *parent = nullptr);
	~MapSettings();

private slots:
	void on_pushButton_clicked();

private:
	Ui::MapSettings *ui;
	MapController & controller;
};
