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

namespace Ui
{
class FirstLaunchView;
}

class CModListView;

class FirstLaunchView : public QWidget
{
	Q_OBJECT

	// vcmibuilder script is not available on these platforms
#if defined(VCMI_WINDOWS) || defined(VCMI_MOBILE) || defined(VCMI_APPLE)
	static constexpr bool hasVCMIBuilderScript = false;
#else
	static constexpr bool hasVCMIBuilderScript = true;
#endif

	void changeEvent(QEvent *event);
	CModListView * getModView();

	void setSetupProgress(int progress);
	void enterSetup();
	void activateTabLanguage();
	void activateTabHeroesData();
	void activateTabModPreset();
	void exitSetup();

	// Tab Language
	void languageSelected(const QString & languageCode);

	// Tab Heroes III Data
	void heroesDataUpdate();
	bool heroesDataDetect();

	void heroesDataMissing();
	void heroesDataDetected();

	QString heroesLanguageDetect();
	void heroesLanguageUpdate();
	void forceHeroesLanguage(const QString & language);

	void copyHeroesData();

	// Tab Mod Preset
	void modPresetUpdate();

	QString findTranslationModName();

	bool checkCanInstallTranslation();
	bool checkCanInstallWog();
	bool checkCanInstallHota();
	bool checkCanInstallExtras();
	bool checkCanInstallMod(const QString & modID);

	void installMod(const QString & modID);

public:
	explicit FirstLaunchView(QWidget * parent = 0);

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

	void on_pushButtonDataCopy_clicked();

	void on_pushButtonDataHelp_clicked();

	void on_comboBoxLanguage_currentIndexChanged(int index);

	void on_pushButtonPresetBack_clicked();

	void on_pushButtonPresetNext_clicked();

private:
	Ui::FirstLaunchView * ui;

};
