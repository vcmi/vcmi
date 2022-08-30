#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>

#include "maphandler.h"

class CMap;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

	void setMap(std::unique_ptr<CMap> cmap);
	void reloadMap();

private slots:
    void on_actionOpen_triggered();

	void on_actionSave_as_triggered();

	void on_actionNew_triggered();

private:
    Ui::MainWindow *ui;
	
	QGraphicsScene * scene;
	QGraphicsScene * sceneMini;
	QPixmap minimap;
	
	std::unique_ptr<CMap> map;
	QString filename;
	bool unsaved = false;
};

#endif // MAINWINDOW_H
