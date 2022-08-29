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

private slots:
    void on_actionOpen_triggered();

private:
    Ui::MainWindow *ui;
	
	QGraphicsScene * scene;
	
	std::unique_ptr<CMap> map;

	MapHandler mapHandler;
};

#endif // MAINWINDOW_H
