#ifndef MAPSETTINGS_H
#define MAPSETTINGS_H

#include <QDialog>

namespace Ui {
class MapSettings;
}

class MapSettings : public QDialog
{
	Q_OBJECT

public:
	explicit MapSettings(QWidget *parent = nullptr);
	~MapSettings();

private:
	Ui::MapSettings *ui;
};

#endif // MAPSETTINGS_H
