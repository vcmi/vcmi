/*
 * mainwindow_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QMainWindow>
#include <QStringList>

namespace Ui
{
class MainWindow;
const QString teamName = "VCMI Team";
const QString appName = "VCMI Launcher";
}

class QTableWidgetItem;

class MainWindow : public QMainWindow
{
	Q_OBJECT

private:
	Ui::MainWindow * ui;
	void load();
#ifndef Q_OS_IOS
	void startExecutable(QString name);
#endif

public:
	explicit MainWindow(QWidget * parent = 0);
	~MainWindow();

public slots:
	void on_startGameButton_clicked();
};
