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

	QFileInfo file = QFileInfo(QFileDialog::getOpenFileName(this, tr("Select exe file of chronicle episode"), "", tr("Application (*.exe)")));
	QDir sourceRoot = file.absoluteDir();

	if(!sourceRoot.exists())
		return;

	int chronicleId = -1;
	std::vector<std::string> names = { "Warlords.exe", "Underworld.exe", "Elements.exe", "Dragons.exe", "WorldTree.exe", "FieryMoon.exe", "Beastmaster.exe", "Sword.exe" };
	for (int i = 0; i < names.size(); i++)
	{
		if(file.fileName().toStdString() == names[i])
			chronicleId = i + 1;
	}
	if(chronicleId == -1)
	{
		QMessageBox::critical(this, "Chronicles data not found!", "Unknown or unsupported Chronicles found.\nPlease select directory with Heroes Chronicles.");
		return;
	}

	extractChronicles(chronicleId, sourceRoot);
}

void AboutProjectView::extractChronicles(int chronicleId, QDir sourceRoot)
{
	QDir sourceRootParent = sourceRoot;
	sourceRootParent.cdUp();

	std::string sep = QString(QDir::separator()).toStdString();

	QStringList dirData = sourceRoot.entryList({"data"}, QDir::Filter::Dirs);
	if(dirData.empty())
	{
		QMessageBox::critical(this, "Chronicles data not found!", "Failed to detect valid Chronicles data in chosen directory.\nPlease select directory with installed Chronicles data.");
		return;
	}

	QDir sourceData = sourceRoot.filePath(dirData.front());
	QDir sourceDataParent = sourceRootParent.filePath(sourceRootParent.entryList({"data"}, QDir::Filter::Dirs).front());

	extractFile([chronicleId, sep](ArchiveEntry entry, CArchiveLoader archive, std::string dest){
		if(!entry.name.find("HcSc"))
		{
			archive.extractToFolder(dest, "", entry, true);

			std::string tmp = entry.name;
			tmp.replace(tmp.find("HcSc"), std::string("HcSc").size(), "Hc" + std::to_string(chronicleId));
			if (tmp.find("_1") != std::string::npos)
				tmp.replace(tmp.find("_1"), std::string("_1").size(), "_Se");
			if (tmp.find("_2") != std::string::npos)
				tmp.replace(tmp.find("_2"), std::string("_2").size(), "_En");
			if (tmp.find("_3") != std::string::npos)
				tmp.replace(tmp.find("_3"), std::string("_3").size(), "_Co");

			QFile::remove(QString::fromStdString(dest + sep + tmp));
			QFile(QString::fromStdString(dest + sep + entry.name)).rename(QString::fromStdString(dest + sep + tmp));
		}
		if(!boost::algorithm::to_lower_copy(entry.name).find("cambkhc"))
		{
			archive.extractToFolder(dest, "", entry, true);
			QFile::remove(QString::fromStdString(dest + sep + "Hc" + std::to_string(chronicleId) + "_BG.pcx"));
			QFile(QString::fromStdString(dest + sep + entry.name)).rename(QString::fromStdString(dest + sep + "Hc" + std::to_string(chronicleId) + "_BG.pcx"));
		}
	}, this, sourceData, "xBitmap.lod", "HcData");


	extractFile([chronicleId, sep](ArchiveEntry entry, CArchiveLoader archive, std::string dest){
		if(!boost::algorithm::to_lower_copy(entry.name).find("main"))
		{
			archive.extractToFolder(dest, "", entry, true);
			QFile::remove(QString::fromStdString(dest + sep + "Hc" + std::to_string(chronicleId) + ".h3c"));
			QFile(QString::fromStdString(dest + sep + entry.name)).rename(QString::fromStdString(dest + sep + "Hc" + std::to_string(chronicleId) + ".h3c"));
		}
	}, this, sourceData, "xlBitmap.lod", "HcMaps");
	if(chronicleId == 5) // special case "The World Tree"
	{
		QDir sourceMaps = sourceRoot.filePath(sourceRoot.entryList({"maps"}, QDir::Filter::Dirs).front());
		QFile(QDir::cleanPath(sourceMaps.absolutePath() + QDir::separator() + "Main.h3c")).copy(QString::fromStdString(VCMIDirs::get().userDataPath().append("HcMaps").string() + sep + "Hc" + std::to_string(chronicleId) + ".h3c"));
	}

	extractFile([sep](ArchiveEntry entry, CArchiveLoader archive, std::string dest){
		if(!boost::algorithm::to_lower_copy(entry.name).find("gamselbk") || !boost::algorithm::to_lower_copy(entry.name).find("loadbar"))
		{
			archive.extractToFolder(dest, "", entry, true);
		}
	}, this, sourceData, "xlBitmap.lod", "Hc" + std::to_string(chronicleId) + "Data");

	extractFile([sep](ArchiveEntry entry, CArchiveLoader archive, std::string dest){
		archive.extractToFolder(dest, "", entry, true);
	}, this, sourceData, "xSound.snd", "Hc" + std::to_string(chronicleId) + "Data");

	extractFile([sep](ArchiveEntry entry, CArchiveLoader archive, std::string dest){
		if(!boost::algorithm::to_lower_copy(entry.name).find("intro"))
		{
			archive.extractToFolder(dest, "", entry, true);
		}
	}, this, sourceData, "xVideo.vid", "Hc" + std::to_string(chronicleId) + "Data");

	extractFile([sep](ArchiveEntry entry, CArchiveLoader archive, std::string dest){
		for(auto & item : std::vector<std::string>{"HPL003sh", "HPL102br", "HPL139", "HPS006kn", "HPS137", "HPS141", "HPL004sh", "hpl112bs", "HPL140", "hps007sh", "HPS138", "HPS142", "HPL006kn", "HPL137", "HPS003sh", "HPS102br", "HPS139", "HPS143", "hpl007sh", "HPL138", "HPS004sh", "hps112bs", "HPS140"})
		{
			if(!boost::algorithm::to_lower_copy(entry.name).find(boost::algorithm::to_lower_copy(item)))
			{
				archive.extractToFolder(dest, "", entry, true);
			}
		}
	}, this, sourceDataParent, "bitmap.lod", "Hc" + std::to_string(chronicleId) + "Data");
}

void AboutProjectView::extractFile(std::function<void(ArchiveEntry, CArchiveLoader, std::string)> cb, QWidget * parent, QDir src, std::string srcFile, std::string dstFolder)
{
	QStringList files = src.entryList({QString::fromStdString(srcFile)}, QDir::Filter::Files);

	if(files.empty())
	{
		QMessageBox::critical(parent, "Chronicles data not found!", "Unknown or unsupported Chronicles found.\nPlease select directory with Heroes Chronicles.");
		return;
	}

	std::string dest = VCMIDirs::get().userDataPath().append(dstFolder).string();
	std::string srcDir = QFileInfo(src, files[0]).absoluteFilePath().toStdString();

	CArchiveLoader archive("", srcDir, false);
	for (const auto& elem: archive.getEntries()) {
		cb(elem.second, archive, dest);
	}
}