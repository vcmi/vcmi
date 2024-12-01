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

	void refreshState();
public:
	explicit StartGameTab(QWidget * parent = nullptr);
	~StartGameTab();

private slots:
	void on_buttonGameStart_clicked();

	void on_buttonOpenChangelog_clicked();

	void on_buttonOpenDownloads_clicked();

	void on_buttonUpdateCheck_clicked();

private:
	Ui::StartGameTab * ui;
};

#endif // STARTGAMETAB_H
