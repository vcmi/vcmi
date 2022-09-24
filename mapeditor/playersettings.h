#pragma once

#include "StdInc.h"
#include <QDialog>
#include "playerparams.h"

namespace Ui {
class PlayerSettings;
}

class PlayerSettings : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSettings(MapController & controller, QWidget *parent = nullptr);
	~PlayerSettings();

private slots:

	void on_playersCount_currentIndexChanged(int index);

	void on_pushButton_clicked();

private:
	Ui::PlayerSettings *ui;

	std::vector<PlayerParams*> paramWidgets;
	
	MapController & controller;
};
