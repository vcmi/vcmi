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

class CModManager;
class CModList;
class CModListModel;
class CModFilterModel;
class CDownloadManager;
class QTableWidgetItem;

class CModEntry;

class CModListView : public QWidget
{
	Q_OBJECT

	std::unique_ptr<CModManager> manager;
	CModListModel * modModel;
	CModFilterModel * filterModel;
	CDownloadManager * dlManager;

	void setupModModel();
	void setupFilterModel();
	void setupModsView();

	void checkManagerErrors();

	/// replace mod ID's with proper human-readable mod names
	QStringList getModNames(QStringList input);

	// find mods unknown to mod list (not present in repo and not installed)
	QStringList findInvalidDependencies(QString mod);
	// find mods that block enabling of this mod: conflicting with this mod or one of required mods
	QStringList findBlockingMods(QString modUnderTest);
	// find mods that depend on this one
	QStringList findDependentMods(QString mod, bool excludeDisabled);

	void downloadFile(QString file, QString url, QString description);

	void installMods(QStringList archives);
	void installFiles(QStringList mods);

	QString genChangelogText(CModEntry & mod);
	QString genModInfoText(CModEntry & mod);

	void changeEvent(QEvent *event) override;
signals:
	void modsChanged();

public:
	explicit CModListView(QWidget * parent = 0);
	~CModListView();

	void loadScreenshots();
	void loadRepositories();

	void disableModInfo();

	void selectMod(const QModelIndex & index);

	const CModList & getModList() const;
	
	// First Launch View interface

	/// install mod by name
	void doInstallMod(const QString & modName);

	/// returns true if mod is available in repository and can be installed
	bool isModAvailable(const QString & modName);

	/// finds translation mod for specified languages. Returns empty string on error
	QString getTranslationModName(const QString & language);

	/// returns true if mod is currently enabled
	bool isModEnabled(const QString & modName);

public slots:
	void enableModByName(QString modName);
	void disableModByName(QString modName);

private slots:
	void dataChanged(const QModelIndex & topleft, const QModelIndex & bottomRight);
	void modSelected(const QModelIndex & current, const QModelIndex & previous);
	void downloadProgress(qint64 current, qint64 max);
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

	void on_pushButton_clicked();

	void on_refreshButton_clicked();

	void on_allModsView_activated(const QModelIndex & index);

	void on_tabWidget_currentChanged(int index);

	void on_screenshotsList_clicked(const QModelIndex & index);

private:
	Ui::CModListView * ui;
};
