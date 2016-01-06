#pragma once

#include "StdInc.h"
#include "../../lib/CConfigHandler.h"

namespace Ui {
	class CModListView;
}

class CModManager;
class CModListModel;
class CModFilterModel;
class CDownloadManager;
class QTableWidgetItem;

class CModEntry;

class CModListView : public QWidget
{
	Q_OBJECT

	CModManager * manager;
	CModListModel * modModel;
	CModFilterModel * filterModel;
	CDownloadManager * dlManager;

	SettingsListener settingsListener;
	bool repositoriesChanged;

	void showEvent(QShowEvent * event);

	void keyPressEvent(QKeyEvent * event);

	void setupModModel();
	void setupFilterModel();
	void setupModsView();
	void loadRepositories();

	void checkManagerErrors();

	// find mods unknown to mod list (not present in repo and not installed)
	QStringList findInvalidDependencies(QString mod);
	// find mods that block enabling of this mod: conflicting with this mod or one of required mods
	QStringList findBlockingMods(QString mod);
	// find mods that depend on this one
	QStringList findDependentMods(QString mod, bool excludeDisabled);

	void downloadFile(QString file, QString url, QString description);

	void installMods(QStringList archives);
	void installFiles(QStringList mods);

	QString genChangelogText(CModEntry & mod);
	QString genModInfoText(CModEntry & mod);
public:
	explicit CModListView(QWidget *parent = 0);
	~CModListView();

	void showModInfo();
	void hideModInfo();
	void loadScreenshots();

	void enableModInfo();
	void disableModInfo();

	void selectMod(const QModelIndex & index);

private slots:
	void dataChanged(const QModelIndex & topleft, const QModelIndex & bottomRight);
	void modSelected(const QModelIndex & current, const QModelIndex & previous);
	void downloadProgress(qint64 current, qint64 max);
	void downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors);
	void modelReset ();
	void hideProgressBar();

	void on_hideModInfoButton_clicked();

	void on_lineEdit_textChanged(const QString &arg1);

	void on_comboBox_currentIndexChanged(int index);

	void on_enableButton_clicked();

	void on_disableButton_clicked();

	void on_updateButton_clicked();

	void on_uninstallButton_clicked();

	void on_installButton_clicked();

	void on_pushButton_clicked();

	void on_refreshButton_clicked();

	void on_allModsView_activated(const QModelIndex &index);

	void on_tabWidget_currentChanged(int index);

	void on_screenshotsList_clicked(const QModelIndex &index);

	void on_showInfoButton_clicked();

private:
	Ui::CModListView *ui;
};
