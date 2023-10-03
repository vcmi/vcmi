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
#include "ui_aboutproject_moc.h"

#include "../updatedialog_moc.h"

#include "../../lib/GameConstants.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/CArchiveLoader.h"

AboutProjectView::AboutProjectView(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::AboutProjectView)
{
	ui->setupUi(this);

	ui->lineEditUserDataDir->setText(pathToQString(VCMIDirs::get().userDataPath()));
	ui->lineEditGameDir->setText(pathToQString(VCMIDirs::get().binaryPath()));
	ui->lineEditTempDir->setText(pathToQString(VCMIDirs::get().userLogsPath()));
	ui->lineEditBuildVersion->setText(QString::fromStdString(GameConstants::VCMI_VERSION));
	ui->lineEditOperatingSystem->setText(QSysInfo::prettyProductName());
}

void AboutProjectView::changeEvent(QEvent *event)
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
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditGameDir->text()).absoluteFilePath()));
}

void AboutProjectView::on_openUserDataDir_clicked()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditUserDataDir->text()).absoluteFilePath()));
}

void AboutProjectView::on_openTempDir_clicked()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditTempDir->text()).absoluteFilePath()));
}

void AboutProjectView::on_pushButtonDiscord_clicked()
{
	QDesktopServices::openUrl(QUrl("https://discord.gg/chBT42V"));
}

void AboutProjectView::on_pushButtonSlack_clicked()
{
	QDesktopServices::openUrl(QUrl("https://slack.vcmi.eu/"));
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


void AboutProjectView::on_pushInstallAdditional_clicked()
{
	QFileInfo file = QFileInfo(QFileDialog::getOpenFileName(this, tr("Select exe file of chronicle episode"), tr("Application (*.exe)")));
	QDir sourceRoot = file.absoluteDir();

	if(!sourceRoot.exists())
		return;

	QStringList dirData = sourceRoot.entryList({"data"}, QDir::Filter::Dirs);
	QDir sourceRootParent = sourceRoot;
	sourceRootParent.cdUp();
	QStringList dirDataParent = sourceRootParent.entryList({"data"}, QDir::Filter::Dirs);

	if(dirData.empty())
	{
		QMessageBox::critical(this, "Chronicles data not found!", "Failed to detect valid Chronicles data in chosen directory.\nPlease select directory with installed Chronicles data.");
		return;
	}

	QDir sourceData = sourceRoot.filePath(dirData.front());
	QStringList files = sourceData.entryList({"xBitmap.lod"}, QDir::Filter::Files);

	if(files.empty())
	{
		QMessageBox::critical(this, "Chronicles data not found!", "Unknown or unsupported Chronicles found.\nPlease select directory with Heroes Chronicles.");
		return;
	}

	int chronicle = 1;
	std::string dest = VCMIDirs::get().userDataPath().append("Data").string();
	std::string src = QFileInfo(sourceData, files[0]).absoluteFilePath().toStdString();
	CArchiveLoader archive("/DATA", src, false);
	for (const auto& elem: archive.getEntries()) {
		if(!elem.second.name.find("HcSc"))
		{
			archive.extractToFolder(dest, "/DATA", elem.second, true);
			std::string tmp = dest + "/" + elem.second.name;
			QFile f(QString::fromStdString(tmp));
			tmp = elem.second.name;
			tmp.replace(tmp.find("HcSc"), std::string("HcSc").size(), "Hc" + std::to_string(chronicle));
			if (tmp.find("_1") != std::string::npos)
				tmp.replace(tmp.find("_1"), std::string("_1").size(), "_Se");
			if (tmp.find("_2") != std::string::npos)
				tmp.replace(tmp.find("_2"), std::string("_2").size(), "_En");
			if (tmp.find("_3") != std::string::npos)
				tmp.replace(tmp.find("_3"), std::string("_3").size(), "_Co");
			tmp = dest + "/" + tmp;
			f.rename(QString::fromStdString(tmp));
		}
		if(!elem.second.name.find("CamBkHc"))
		{
			archive.extractToFolder(dest, "/DATA", elem.second, true);
			std::string tmp = dest + "/" + elem.second.name;
			QFile f(QString::fromUtf8(tmp.data(), int(tmp.size())));
			tmp = dest + "/" + "Hc" + std::to_string(chronicle) + "_BG.pcx";
			f.rename(QString::fromUtf8(tmp.data(), int(tmp.size())));
		}
	}
}

