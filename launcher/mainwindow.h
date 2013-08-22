#pragma once
#include <QMainWindow>

namespace Ui {
	class MainWindow;
}

class QTableWidgetItem;

class MainWindow : public QMainWindow
{
	Q_OBJECT

	void load();
	void startExecutable(QString name);
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_startGameButon_clicked();

private:
	Ui::MainWindow *ui;
};
