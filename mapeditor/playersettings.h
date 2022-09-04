#ifndef PLAYERSETTINGS_H
#define PLAYERSETTINGS_H

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
	explicit PlayerSettings(CMapHeader & mapHeader, QWidget *parent = nullptr);
	~PlayerSettings();

private slots:

	void on_playersCount_currentIndexChanged(int index);

private:
	Ui::PlayerSettings *ui;

	std::vector<PlayerParams*> paramWidgets;

	CMapHeader & header;
};

#endif // PLAYERSETTINGS_H
