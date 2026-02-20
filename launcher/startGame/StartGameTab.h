/*
 * StartGameTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

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

	void refreshUpdateStatus(EGameUpdateStatus status);
	void refreshTranslation(ETranslationStatus status);
	void refreshMods();
	void refreshPresets();
	void refreshGameData();

	void changeEvent(QEvent *event) override;
public:
	explicit StartGameTab(QWidget * parent = nullptr);
	~StartGameTab();

	void refreshState();

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
	void on_buttonInstallHdEditionHelp_clicked();
	void on_buttonInstallHdEdition_clicked();
	void on_buttonPresetExport_clicked();
	void on_buttonPresetImport_clicked();
	void on_buttonPresetNew_clicked();
	void on_buttonPresetDelete_clicked();
	void on_comboBoxModPresets_currentTextChanged(const QString &arg1);
	void on_buttonPresetRename_clicked();

	void clipboardDataChanged();
private:
	Ui::StartGameTab * ui;
};
