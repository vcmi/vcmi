/*
 * aboutproject_moc.h, part of VCMI engine
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
class AboutProjectView;
}

class ProgressOverlay;
class IVCMIDirs;
enum class EUserDirectory;

class AboutProjectView : public QWidget
{
	Q_OBJECT

	enum class EExistingTargetAction
	{
		MERGE,
		BACK_UP,
		REPLACE
	};

	void changeEvent(QEvent * event) override;

	/// Hides a widget and expands second widgets to take place of first widget in layout
	void hideAndStretchWidget(QGridLayout * layout, QWidget * toHide, QWidget * toStretch);

	void refreshDirectoryPaths();
	void changeDirectory(EUserDirectory directory, const QString & title);
	bool isSameOrChildPath(const QString & path, const QString & parent) const;
	bool containsActiveUserDirectory(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & path) const;
	bool copyDirectoryContents(const QString & source,	const QString & destination, ProgressOverlay & progress, QString & error, bool overwrite = false);
	std::optional<EExistingTargetAction> askExistingTargetAction(const QString & target);
	QString availableBackupPath(const QString & target) const;
	bool installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, QString & backupPath, QString & error);

public:
	explicit AboutProjectView(QWidget * parent = nullptr);
	~AboutProjectView() override;

signals:
	void logDirectoryChanged(const QString & path);

private slots:
	void on_updatesButton_clicked();

	void on_openGameDataDir_clicked();

	void on_openUserDataDir_clicked();

	void on_openTempDir_clicked();

	void on_pushButtonDiscord_clicked();

	void on_pushButtonGithub_clicked();

	void on_pushButtonHomepage_clicked();

	void on_pushButtonBugreport_clicked();

	void on_pushButtonExportLogs_clicked();

	void on_pushButtonExportSaves_clicked();

	void on_openConfigDir_clicked();

	void on_changeUserDataDir_clicked();

	void on_changeTempDir_clicked();

	void on_openCacheDir_clicked();

	void on_changeCacheDir_clicked();

	void on_changeConfigDir_clicked();

	void on_openSaveDir_clicked();

	void on_changeSaveDir_clicked();

private:
	std::unique_ptr<Ui::AboutProjectView> ui;
};
