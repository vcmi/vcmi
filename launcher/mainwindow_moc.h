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
#include <QTranslator>

namespace Ui
{
class MainWindow;
const QString teamName = "vcmi";
const QString appName = "launcher";
}

class QTableWidgetItem;
class CModList;
class CModListView;

class MainWindow : public QMainWindow
{
	Q_OBJECT

#ifdef ENABLE_QT_TRANSLATIONS
	QTranslator translator;
#endif
	Ui::MainWindow * ui;

	void load();

	enum TabRows
	{
		MODS = 0,
		SETTINGS = 1,
		SETUP = 2,
		ABOUT = 3,
	};

public:
	explicit MainWindow(QWidget * parent = nullptr);
	~MainWindow() override;

	const CModList & getModList() const;
	CModListView * getModView();

	void updateTranslation();
	void computeSidePanelSizes();
	
	void detectPreferredLanguage();
	void enterSetup();
	void exitSetup();
	void switchToModsTab();

protected:
	void changeEvent(QEvent * event) override;

public slots:
	void on_startGameButton_clicked();
	
private slots:
	void on_modslistButton_clicked();
	void on_settingsButton_clicked();
	void on_startEditorButton_clicked();
	void on_aboutButton_clicked();
};
