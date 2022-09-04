#ifndef MAPSETTINGS_H
#define MAPSETTINGS_H

#include <QDialog>

namespace Ui {
class MapSettings;
}

class MainWindow;
class MapSettings : public QDialog
{
	Q_OBJECT

public:
	explicit MapSettings(MainWindow *parent = nullptr);
	~MapSettings();

private slots:
	void on_pushButton_clicked();

private:
	Ui::MapSettings *ui;
	MainWindow * main;
};

#endif // MAPSETTINGS_H
