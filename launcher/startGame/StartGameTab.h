#ifndef STARTGAMETAB_H
#define STARTGAMETAB_H

#include <QWidget>

namespace Ui
{
class StartGameTab;
}

enum class EGameUpdateStatus : int8_t
{
	NOT_CHECKED,
	NO_UPDATE,
	UPDATE_AVAILABLE
};

enum class ETranslationStatus : int8_t;

class MainWindow;

class StartGameTab : public QWidget
{
	Q_OBJECT

	MainWindow * getMainWindow();

	void refreshState();

	void refreshUpdateStatus(EGameUpdateStatus status);
	void refreshTranslation(ETranslationStatus status);
	void refreshMods();
	void refreshGameData();

public:
	explicit StartGameTab(QWidget * parent = nullptr);
	~StartGameTab();

private slots:
	void on_buttonGameStart_clicked();
	void on_buttonOpenChangelog_clicked();
	void on_buttonOpenDownloads_clicked();
	void on_buttonUpdateCheck_clicked();
	void on_buttonGameEditor_clicked();
	void on_buttonImportFiles_clicked();
	void on_buttonInstallTranslation_clicked();
	void on_buttonActivateTranslation_clicked();
	void on_buttonUpdateMods_clicked();
	void on_buttonHelpImportFiles_clicked();
	void on_buttonInstallTranslationHelp_clicked();
	void on_buttonActivateTranslationHelp_clicked();
	void on_buttonUpdateModsHelp_clicked();
	void on_buttonChroniclesHelp_clicked();
	void on_buttonMissingSoundtrackHelp_clicked();
	void on_buttonMissingVideoHelp_clicked();
	void on_buttonMissingFilesHelp_clicked();
	void on_buttonMissingCampaignsHelp_clicked();

private:
	Ui::StartGameTab * ui;
};

#endif // STARTGAMETAB_H
