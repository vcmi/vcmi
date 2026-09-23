/*
 * aboutproject_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "aboutproject_moc.h"
#include "userdirectorymanager.h"
#include "ui_aboutproject_moc.h"

#include "../updatedialog_moc.h"
#include "../helper.h"

#include "../../lib/GameConstants.h"
#include "../../lib/VCMIDirs.h"

void AboutProjectView::hideAndStretchWidget(QGridLayout * layout, QWidget * toHide, QWidget * toStretch)
{
	toHide->hide();

	int index = layout->indexOf(toStretch);
	int row;
	int col;
	int unused;
	layout->getItemPosition(index, &row, &col, &unused, &unused);
	layout->removeWidget(toHide);
	layout->removeWidget(toStretch);
	layout->addWidget(toStretch, row, col, 1, -1);
}

AboutProjectView::AboutProjectView(QWidget * parent)
	: QWidget(parent)
	, ui(std::make_unique<Ui::AboutProjectView>())
{
	ui->setupUi(this);

	refreshDirectoryPaths();
	ui->lineEditGameDir->setText(pathToQString(boost::filesystem::absolute(VCMIDirs::get().binaryPath())));
	ui->lineEditBuildVersion->setText(QString(GameConstants::VCMI_VERSION));
	ui->lineEditOperatingSystem->setText(QSysInfo::prettyProductName());

#ifndef VCMI_WINDOWS
	ui->changeUserDataDir->hide();
	ui->changeTempDir->hide();
	ui->changeCacheDir->hide();
	ui->changeConfigDir->hide();
	ui->changeSaveDir->hide();
#endif

#ifdef VCMI_MOBILE
	// On mobile platforms these directories are generally not accessible from phone itself, only via USB connection from PC
	// Remove "Open" buttons and stretch line with text into now-empty space
	hideAndStretchWidget(ui->gridLayout, ui->openGameDataDir, ui->lineEditGameDir);
#ifdef VCMI_ANDROID
	hideAndStretchWidget(ui->gridLayout, ui->openUserDataDir, ui->lineEditUserDataDir);
	hideAndStretchWidget(ui->gridLayout, ui->openCacheDir, ui->lineEditCacheDir);
	hideAndStretchWidget(ui->gridLayout, ui->openTempDir, ui->lineEditTempDir);
	hideAndStretchWidget(ui->gridLayout, ui->openConfigDir, ui->lineEditConfigDir);
	hideAndStretchWidget(ui->gridLayout, ui->openSaveDir, ui->lineEditSaveDir);
#endif
#endif
}

AboutProjectView::~AboutProjectView() = default;

void AboutProjectView::refreshDirectoryPaths()
{
	const auto & dirs = VCMIDirs::get();
	ui->lineEditUserDataDir->setText(pathToQString(dirs.userDataPath()));
	ui->lineEditCacheDir->setText(pathToQString(dirs.userCachePath()));
	ui->lineEditConfigDir->setText(pathToQString(dirs.userConfigPath()));
	ui->lineEditTempDir->setText(pathToQString(dirs.userLogsPath()));
	ui->lineEditSaveDir->setText(pathToQString(dirs.userSavePath()));
}

void AboutProjectView::changeEvent(QEvent * event)
{
	if(event->type() == QEvent::LanguageChange)
		ui->retranslateUi(this);

	QWidget::changeEvent(event);
}

void AboutProjectView::on_updatesButton_clicked()
{
	UpdateDialog::showUpdateDialog(true);
}

void AboutProjectView::on_openGameDataDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditGameDir->text());
}

void AboutProjectView::on_openUserDataDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditUserDataDir->text());
}

void AboutProjectView::on_openTempDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditTempDir->text());
}

void AboutProjectView::on_openConfigDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditConfigDir->text());
}

void AboutProjectView::changeDirectory(EUserDirectory directory, const QString & title)
{
#if defined(VCMI_WINDOWS)
	WindowsUserDirectoryManager manager(this);
	manager.changeDirectory(directory, title, [this](const QString & changedLogPath)
	{
		refreshDirectoryPaths();
		if(!changedLogPath.isEmpty())
			emit logDirectoryChanged(changedLogPath);
	});
#else
	// TODO: Every Non-Windows OS is unsupported right now
	Q_UNUSED(directory);
	Q_UNUSED(title);
#endif
}

void AboutProjectView::on_changeUserDataDir_clicked()
{
	changeDirectory(EUserDirectory::DATA, tr("Select user data directory"));
}

void AboutProjectView::on_changeTempDir_clicked()
{
	changeDirectory(EUserDirectory::LOGS, tr("Select log files directory"));
}

void AboutProjectView::on_openCacheDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditCacheDir->text());
}

void AboutProjectView::on_changeCacheDir_clicked()
{
	changeDirectory(EUserDirectory::CACHE, tr("Select cache directory"));
}

void AboutProjectView::on_changeConfigDir_clicked()
{
	changeDirectory(EUserDirectory::CONFIG, tr("Select configuration files directory"));
}

void AboutProjectView::on_openSaveDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditSaveDir->text());
}

void AboutProjectView::on_changeSaveDir_clicked()
{
	changeDirectory(EUserDirectory::SAVES, tr("Select saved games directory"));
}

void AboutProjectView::on_pushButtonDiscord_clicked()
{
	QDesktopServices::openUrl(QUrl("https://discord.gg/chBT42V"));
}

void AboutProjectView::on_pushButtonGithub_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi"));
}

void AboutProjectView::on_pushButtonHomepage_clicked()
{
	QDesktopServices::openUrl(QUrl("https://vcmi.eu/"));
}

void AboutProjectView::on_pushButtonBugreport_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi/issues"));
}
