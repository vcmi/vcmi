#ifndef PLAYERSETTINGS_H
#define PLAYERSETTINGS_H

#include <QDialog>

namespace Ui {
class PlayerSettings;
}

class PlayerSettings : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSettings(QWidget *parent = nullptr);
	~PlayerSettings();

private:
	Ui::PlayerSettings *ui;
};

#endif // PLAYERSETTINGS_H
