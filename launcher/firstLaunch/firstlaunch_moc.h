/*
 * firstlaunch_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../StdInc.h"

#include "../demo.h"

namespace Ui
{
class FirstLaunchView;
}

class CModListView;
class ProgressOverlay;

class FirstLaunchView : public QWidget, public IDemoInstallerCallback
{
	Q_OBJECT

	std::unique_ptr<DemoInstaller> demo;
	ProgressOverlay * demoOverlay = nullptr;
	bool demoDataActive = false;

	// IDemoInstallerCallback
	void onInstallFinished() override;
	void onInstallError() override;
	void onInstallProgress(float percent) override;

	void changeEvent(QEvent *event) override;
	CModListView * getModView();

	void setSetupProgress(int progress);
	void activateTabLanguage();
	void activateTabHeroesData();
	void activateTabModPreset();
	void exitSetup(bool goToMods);
	
	// Tab Language
	void languageSelected(const QString & languageCode);

	// Tab Heroes III Data
	bool heroesDataDetect(bool checkDemo);

	void heroesDataMissing();
	void heroesDataDetected();

	QString getHeroesInstallDir();
	void extractGogData();
	void extractGogDataAsync(QString filePathBin, QString filePathExe);
	ProgressOverlay * createOverlay(const QString & title, bool indeterminate = true);
	bool performCopyFlow(const QString& path, ProgressOverlay* overlay, bool removeSource);
	void copyHeroesData(const QString & path = {}, bool removeSource = false);

	// Tab Mod Preset
	void modPresetUpdate();

	QString findTranslationModName();

	bool checkCanInstallTranslation();
	bool checkCanInstallExtras();
	bool checkCanInstallHota();
	bool checkCanInstallWog();
	bool checkCanInstallTow();
	bool checkCanInstallFod();
	bool checkCanInstallMod(const QString & modID);

public:
	explicit FirstLaunchView(QWidget * parent = nullptr);
	~FirstLaunchView() override;

	void enterSetup();

	// Tab Heroes III Data
	bool heroesDataUpdate(bool checkDemo);

    bool needPostCopyCheckExe;
    bool needPostCopyCheckBin;

    QString checkFileMagic(const QString &filename, const QString &filter, const QByteArray &magic, const QString &ext, bool &openFailed) const;

public slots:

private slots:

	void on_buttonTabLanguage_clicked();
	void on_buttonTabHeroesData_clicked();
	void on_buttonTabModPreset_clicked();
	void on_listWidgetLanguage_currentRowChanged(int currentRow);
	void on_pushButtonLanguageNext_clicked();
	void on_pushButtonDataNext_clicked();
	void on_pushButtonDataBack_clicked();

	void on_pushButtonDataSearch_clicked();

	void on_pushButtonDemo_clicked();

	void on_pushButtonDataCopy_clicked();

	void on_pushButtonGogInstall_clicked();

	void on_pushButtonPresetBack_clicked();

	void on_pushButtonPresetNext_clicked();

	void on_pushButtonDiscord_clicked();

	void on_pushButtonGithub_clicked();

private:
	std::unique_ptr<Ui::FirstLaunchView> ui;
};
