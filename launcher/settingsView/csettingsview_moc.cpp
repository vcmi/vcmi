/*
 * csettingsview_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "csettingsview_moc.h"
#include "ui_csettingsview_moc.h"

#include "../updatedialog_moc.h"

#include <QFileInfo>
#include <QGuiApplication>

#include "../../lib/CConfigHandler.h"
#include "../../lib/VCMIDirs.h"

/// List of encoding which can be selected from Launcher.
/// Note that it is possible to specify enconding manually in settings.json
static const std::string knownEncodingsList[] = //TODO: remove hardcode
{
	// European Windows-125X encodings
	"CP1250", // West European, covers mostly Slavic languages that use latin script
	"CP1251", // Covers languages that use cyrillic scrypt
	"CP1252", // Latin/East European, covers most of latin languages
	// Chinese encodings
	"GBK", // extension of GB2312, also known as CP936
	"GB2312", // basic set for Simplified Chinese. Separate from GBK to allow proper detection of H3 fonts
	// Korean encodings
	"CP949" // extension of EUC-KR.
};

void CSettingsView::setDisplayList()
{
	QStringList list;

	for (const auto screen : QGuiApplication::screens())
	{
		QString string;
		const auto & rect = screen->geometry();
		QTextStream(&string) << screen->name() << " - " << rect.width() << "x" << rect.height();
		list << string;
	}

	if(list.count() < 2)
	{
		ui->comboBoxDisplayIndex->hide();
		ui->labelDisplayIndex->hide();
	}
	else
	{
		int displayIndex = settings["video"]["displayIndex"].Integer();
		ui->comboBoxDisplayIndex->clear();
		ui->comboBoxDisplayIndex->addItems(list);
		ui->comboBoxDisplayIndex->setCurrentIndex(displayIndex);
	}
}

void CSettingsView::loadSettings()
{
	int resX = settings["video"]["screenRes"]["width"].Float();
	int resY = settings["video"]["screenRes"]["height"].Float();
	int resIndex = ui->comboBoxResolution->findText(QString("%1x%2").arg(resX).arg(resY));

	ui->comboBoxResolution->setCurrentIndex(resIndex);
	ui->comboBoxShowIntro->setCurrentIndex(settings["video"]["showIntro"].Bool());

#ifdef Q_OS_IOS
	ui->comboBoxFullScreen->setCurrentIndex(true);
	ui->checkBoxFullScreen->setChecked(false);
	for (auto widget : std::initializer_list<QWidget *>{ui->comboBoxFullScreen, ui->checkBoxFullScreen})
		widget->setDisabled(true);
#else
	ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
	ui->checkBoxFullScreen->setChecked(settings["video"]["realFullscreen"].Bool());
#endif

	int friendlyAIIndex = ui->comboBoxFriendlyAI->findText(QString::fromUtf8(settings["server"]["friendlyAI"].String().c_str()));
	int neutralAIIndex = ui->comboBoxNeutralAI->findText(QString::fromUtf8(settings["server"]["neutralAI"].String().c_str()));
	int enemyAIIndex = ui->comboBoxEnemyAI->findText(QString::fromUtf8(settings["server"]["enemyAI"].String().c_str()));
	int playerAIIndex = ui->comboBoxPlayerAI->findText(QString::fromUtf8(settings["server"]["playerAI"].String().c_str()));

	ui->comboBoxFriendlyAI->setCurrentIndex(friendlyAIIndex);
	ui->comboBoxNeutralAI->setCurrentIndex(neutralAIIndex);
	ui->comboBoxEnemyAI->setCurrentIndex(enemyAIIndex);
	ui->comboBoxPlayerAI->setCurrentIndex(playerAIIndex);

	ui->spinBoxNetworkPort->setValue(settings["server"]["port"].Integer());

	ui->comboBoxAutoCheck->setCurrentIndex(settings["launcher"]["autoCheckRepositories"].Bool());
	// all calls to plainText will trigger textChanged() signal overwriting config. Create backup before editing widget
	JsonNode urls = settings["launcher"]["repositoryURL"];

	ui->plainTextEditRepos->clear();
	for(auto entry : urls.Vector())
		ui->plainTextEditRepos->appendPlainText(QString::fromUtf8(entry.String().c_str()));

	ui->lineEditUserDataDir->setText(pathToQString(VCMIDirs::get().userDataPath()));
	ui->lineEditGameDir->setText(pathToQString(VCMIDirs::get().binaryPath()));
	ui->lineEditTempDir->setText(pathToQString(VCMIDirs::get().userCachePath()));

	std::string encoding = settings["general"]["encoding"].String();
	size_t encodingIndex = boost::range::find(knownEncodingsList, encoding) - knownEncodingsList;
	if(encodingIndex < ui->comboBoxEncoding->count())
		ui->comboBoxEncoding->setCurrentIndex((int)encodingIndex);
	ui->comboBoxAutoSave->setCurrentIndex(settings["general"]["saveFrequency"].Integer() > 0 ? 1 : 0);
}

CSettingsView::CSettingsView(QWidget * parent)
	: QWidget(parent), ui(new Ui::CSettingsView)
{
	ui->setupUi(this);

	ui->labelBuildVersion->setText(QString::fromStdString(GameConstants::VCMI_VERSION));
	loadSettings();
}

CSettingsView::~CSettingsView()
{
	delete ui;
}


void CSettingsView::on_comboBoxResolution_currentIndexChanged(const QString & arg1)
{
	QStringList list = arg1.split("x");

	Settings node = settings.write["video"]["screenRes"];
	node["width"].Float() = list[0].toInt();
	node["height"].Float() = list[1].toInt();
}

void CSettingsView::on_comboBoxFullScreen_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["fullscreen"];
	node->Bool() = index;
}

void CSettingsView::on_checkBoxFullScreen_stateChanged(int state)
{
	Settings node = settings.write["video"]["realFullscreen"];
	node->Bool() = state;
}

void CSettingsView::on_comboBoxAutoCheck_currentIndexChanged(int index)
{
	Settings node = settings.write["launcher"]["autoCheckRepositories"];
	node->Bool() = index;
}

void CSettingsView::on_comboBoxDisplayIndex_currentIndexChanged(int index)
{
	Settings node = settings.write["video"];
	node["displayIndex"].Float() = index;
}

void CSettingsView::on_comboBoxPlayerAI_currentIndexChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["playerAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxFriendlyAI_currentIndexChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["friendlyAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxNeutralAI_currentIndexChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["neutralAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxEnemyAI_currentIndexChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["enemyAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_spinBoxNetworkPort_valueChanged(int arg1)
{
	Settings node = settings.write["server"]["port"];
	node->Float() = arg1;
}

void CSettingsView::on_plainTextEditRepos_textChanged()
{
	Settings node = settings.write["launcher"]["repositoryURL"];

	QStringList list = ui->plainTextEditRepos->toPlainText().split('\n');

	node->Vector().clear();
	for(QString line : list)
	{
		if(line.trimmed().size() > 0)
		{
			JsonNode entry;
			entry.String() = line.trimmed().toUtf8().data();
			node->Vector().push_back(entry);
		}
	}
}

void CSettingsView::on_comboBoxEncoding_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["encoding"];
	node->String() = knownEncodingsList[index];
}

void CSettingsView::on_openTempDir_clicked()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditTempDir->text()).absoluteFilePath()));
}

void CSettingsView::on_openUserDataDir_clicked()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditUserDataDir->text()).absoluteFilePath()));
}

void CSettingsView::on_openGameDataDir_clicked()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditGameDir->text()).absoluteFilePath()));
}

void CSettingsView::on_comboBoxShowIntro_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["showIntro"];
	node->Bool() = index;
}

void CSettingsView::on_changeGameDataDir_clicked()
{

}

void CSettingsView::on_comboBoxAutoSave_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["saveFrequency"];
	node->Integer() = index;
}

void CSettingsView::on_updatesButton_clicked()
{
	UpdateDialog::showUpdateDialog(true);
}

