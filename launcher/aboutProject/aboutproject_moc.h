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

enum class EUserDirectory;

class AboutProjectView : public QWidget
{
	Q_OBJECT

	void changeEvent(QEvent * event) override;

	/// Hides a widget and expands second widgets to take place of first widget in layout
	void hideAndStretchWidget(QGridLayout * layout, QWidget * toHide, QWidget * toStretch);

	void refreshDirectoryPaths();
	void changeDirectory(EUserDirectory directory, const QString & title);

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
