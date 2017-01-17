#pragma once
#include <QMainWindow>
#include <QStringList>

namespace Ui {
	class MainWindow;
}

class QTableWidgetItem;

class MainWindow : public QMainWindow
{
	Q_OBJECT
private:
	Ui::MainWindow *ui;
	void load();
	void startExecutable(QString name);
public:
	explicit MainWindow(const QStringList& displayList, QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_startGameButton_clicked();
};
