/*
 * cmodlistview_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"
#include "../../lib/CConfigHandler.h"

namespace Ui
{
class CModListView;
}

class ModStateController;
class CModList;
class ModStateItemModel;
class ModStateModel;
class CModFilterModel;
class CDownloadManager;
class QTableWidgetItem;

class ModState;

class CModListView : public QWidget
{
	Q_OBJECT

	std::shared_ptr<ModStateModel> modStateModel;
	std::unique_ptr<ModStateController> manager;
	ModStateItemModel * modModel;
	CModFilterModel * filterModel;
	CDownloadManager * dlManager;
	JsonNode accumulatedRepositoryData;
	QString activatingPreset;

	QStringList enqueuedModDownloads;

	void setupModModel();
	void setupFilterModel();
	void setupModsView();

	void checkManagerErrors();

	/// replace mod ID's with proper human-readable mod names
	QStringList getModNames(QString queryingMod, QStringList input);

	/// returns list of mods that are needed for install of this mod (potentially including this mod itself)
	QStringList getModsToInstall(QString mod);

	// find mods unknown to mod list (not present in repo and not installed)
	QStringList findUnavailableMods(QStringList candidates);

	void installMods(QStringList archives);
	void installMaps(QStringList maps);

	QString genChangelogText(const ModState & mod);
	QString genModInfoText(const ModState & mod);

	void changeEvent(QEvent *event) override;

	auto buttonEnabledState(QString modName, ModState & mod);

public:
	explicit CModListView(QWidget * parent = nullptr);
	~CModListView();

	void loadScreenshots();
	void loadRepositories();

	void reload(const QString& modToSelect = QString());

	void disableModInfo();

	void selectMod(const QModelIndex & index);

	// First Launch View interface

	/// install mod by name
	void doInstallMod(const QString & modName);

	/// uninstall mod by name
	void doUninstallMod(const QString & modName);

	/// update mod by name
	void doUpdateMod(const QString & modName);

	/// open mod dictionary by name
	void openModDictionary(const QString & modName);

	/// returns true if mod is available in repository and can be installed
	bool isModAvailable(const QString & modName);

	/// finds translation mod for specified languages. Returns empty string on error
	QString getTranslationModName(const QString & language);

	/// finds all already imported Heroes Chronicles mods (if any)
	QStringList getInstalledChronicles();

	/// finds all mods that can be updated
	QStringList getUpdateableMods();

	void createNewPreset(const QString & presetName);
	void deletePreset(const QString & presetName);
	void activatePreset(const QString & presetName);
	void renamePreset(const QString & oldPresetName, const QString & newPresetName);

	QStringList getAllPresets() const;
	QString getActivePreset() const;

	JsonNode exportCurrentPreset() const;
	void importPreset(const JsonNode & data);

	/// returns true if mod is currently enabled
	bool isModEnabled(const QString & modName);

	/// returns true if mod is currently installed
	bool isModInstalled(const QString & modName);

	void downloadMod(const ModState & mod);
	void downloadFile(QString file, QUrl url, QString description, qint64 sizeBytes = 0);
	void installFiles(QStringList mods);

public slots:
	void enableModByName(QString modName);
	void disableModByName(QString modName);

private slots:
	void onCustomContextMenu(const QPoint &point);
	void dataChanged(const QModelIndex & topleft, const QModelIndex & bottomRight);
	void modSelected(const QModelIndex & current, const QModelIndex & previous);
	void downloadProgress(qint64 current, qint64 max);
	void extractionProgress(qint64 current, qint64 max);
	void downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors);
	void modelReset();
	void hideProgressBar();

	void on_lineEdit_textChanged(const QString & arg1);
	void on_comboBox_currentIndexChanged(int index);
	void on_enableButton_clicked();
	void on_disableButton_clicked();
	void on_updateButton_clicked();
	void on_uninstallButton_clicked();
	void on_installButton_clicked();
	void on_abortButton_clicked();
	void on_refreshButton_clicked();
	void on_allModsView_activated(const QModelIndex & index);
	void on_tabWidget_currentChanged(int index);
	void on_screenshotsList_clicked(const QModelIndex & index);
	void on_allModsView_doubleClicked(const QModelIndex &index);

private:
	Ui::CModListView * ui;
};
