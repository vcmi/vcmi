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

	void manualInstallFile(QString filePath);
	void downloadFile(QString file, QString url, QString description, qint64 sizeBytes = 0);
	void downloadFile(QString file, QUrl url, QString description, qint64 sizeBytes = 0);

	void installMods(QStringList archives);
	void installMaps(QStringList maps);
	void installFiles(QStringList mods);

	QString genChangelogText(const ModState & mod);
	QString genModInfoText(const ModState & mod);

	void changeEvent(QEvent *event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent *event) override;

public:
	explicit CModListView(QWidget * parent = nullptr);
	~CModListView();

	void loadScreenshots();
	void loadRepositories();

	void disableModInfo();

	void selectMod(const QModelIndex & index);

	// First Launch View interface

	/// install mod by name
	void doInstallMod(const QString & modName);

	/// returns true if mod is available in repository and can be installed
	bool isModAvailable(const QString & modName);

	/// finds translation mod for specified languages. Returns empty string on error
	QString getTranslationModName(const QString & language);

	/// returns true if mod is currently enabled
	bool isModEnabled(const QString & modName);

	/// returns true if mod is currently installed
	bool isModInstalled(const QString & modName);

public slots:
	void enableModByName(QString modName);
	void disableModByName(QString modName);

private slots:
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

	void on_installFromFileButton_clicked();

	void on_pushButton_clicked();

	void on_refreshButton_clicked();

	void on_allModsView_activated(const QModelIndex & index);

	void on_tabWidget_currentChanged(int index);

	void on_screenshotsList_clicked(const QModelIndex & index);

	void on_allModsView_doubleClicked(const QModelIndex &index);

private:
	Ui::CModListView * ui;
};
