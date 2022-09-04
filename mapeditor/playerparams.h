#ifndef PLAYERPARAMS_H
#define PLAYERPARAMS_H

#include <QWidget>
#include "../lib/mapping/CMap.h"

namespace Ui {
class PlayerParams;
}

class PlayerParams : public QWidget
{
	Q_OBJECT

public:
	explicit PlayerParams(const CMapHeader & mapHeader, int playerId, QWidget *parent = nullptr);
	~PlayerParams();

	PlayerInfo playerInfo;
	int playerColor;

private slots:
	void on_radioHuman_toggled(bool checked);

	void on_radioCpu_toggled(bool checked);

private:
	Ui::PlayerParams *ui;
};

#endif // PLAYERPARAMS_H
