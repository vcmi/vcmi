#ifndef STARTGAMETAB_H
#define STARTGAMETAB_H

#include <QWidget>

namespace Ui
{
class StartGameTab;
}

class MainWindow;

class StartGameTab : public QWidget
{
	Q_OBJECT

	MainWindow * getMainWindow();
public:
	explicit StartGameTab(QWidget * parent = nullptr);
	~StartGameTab();

private slots:
	void on_buttonPlay_clicked();

private:
	Ui::StartGameTab * ui;
};

#endif // STARTGAMETAB_H
