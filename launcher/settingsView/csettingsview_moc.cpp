#include "StdInc.h"
#include "csettingsview_moc.h"
#include "ui_csettingsview_moc.h"

#include <QFileInfo>

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
    "GBK",    // extension of GB2312, also known as CP936
    "GB2312"  // basic set for Simplified Chinese. Separate from GBK to allow proper detection of H3 fonts
};

void CSettingsView::loadSettings()
{
	int resX = settings["video"]["screenRes"]["width"].Float();
	int resY = settings["video"]["screenRes"]["height"].Float();

	int resIndex = ui->comboBoxResolution->findText(QString("%1x%2").arg(resX).arg(resY));

	ui->comboBoxResolution->setCurrentIndex(resIndex);
	ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
	ui->comboBoxShowIntro->setCurrentIndex(settings["video"]["showIntro"].Bool());

	int neutralAIIndex = ui->comboBoxNeutralAI->findText(QString::fromUtf8(settings["server"]["neutralAI"].String().c_str()));
	int playerAIIndex = ui->comboBoxPlayerAI->findText(QString::fromUtf8(settings["server"]["playerAI"].String().c_str()));

	ui->comboBoxNeutralAI->setCurrentIndex(neutralAIIndex);
	ui->comboBoxPlayerAI->setCurrentIndex(playerAIIndex);

	ui->spinBoxNetworkPort->setValue(settings["server"]["port"].Float());

	ui->comboBoxAutoCheck->setCurrentIndex(settings["launcher"]["autoCheckRepositories"].Bool());
	// all calls to plainText will trigger textChanged() signal overwriting config. Create backup before editing widget
	JsonNode urls = settings["launcher"]["repositoryURL"];

	ui->plainTextEditRepos->clear();
	for (auto entry : urls.Vector())
		ui->plainTextEditRepos->appendPlainText(QString::fromUtf8(entry.String().c_str()));

	ui->lineEditUserDataDir->setText(pathToQString(VCMIDirs::get().userDataPath()));
	ui->lineEditGameDir->setText(pathToQString(VCMIDirs::get().binaryPath()));
	ui->lineEditTempDir->setText(pathToQString(VCMIDirs::get().userCachePath()));

	std::string encoding = settings["general"]["encoding"].String();
	size_t encodingIndex = boost::range::find(knownEncodingsList, encoding) - knownEncodingsList;
	if (encodingIndex < ui->comboBoxEncoding->count())
		ui->comboBoxEncoding->setCurrentIndex(encodingIndex);
}

CSettingsView::CSettingsView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CSettingsView)
{
	ui->setupUi(this);

	loadSettings();
}

CSettingsView::~CSettingsView()
{
	delete ui;
}

void CSettingsView::on_comboBoxResolution_currentIndexChanged(const QString &arg1)
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

void CSettingsView::on_comboBoxAutoCheck_currentIndexChanged(int index)
{
	Settings node = settings.write["launcher"]["autoCheckRepositories"];
	node->Bool() = index;
}

void CSettingsView::on_comboBoxPlayerAI_currentIndexChanged(const QString &arg1)
{
	Settings node = settings.write["server"]["playerAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxNeutralAI_currentIndexChanged(const QString &arg1)
{
	Settings node = settings.write["server"]["neutralAI"];
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
	for (QString line : list)
	{
		if (line.trimmed().size() > 0)
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
