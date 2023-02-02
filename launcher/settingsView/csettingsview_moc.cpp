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

#include "mainwindow_moc.h"

#include "../jsonutils.h"
#include "../launcherdirs.h"
#include "../updatedialog_moc.h"

#include <QFileInfo>
#include <QGuiApplication>

#include "../../lib/CConfigHandler.h"
#include "../../lib/VCMIDirs.h"

namespace
{
QString resolutionToString(const QSize & resolution)
{
	return QString{"%1x%2"}.arg(resolution.width()).arg(resolution.height());
}
}

/// List of encoding which can be selected from Launcher.
/// Note that it is possible to specify enconding manually in settings.json
static const std::string knownEncodingsList[] = //TODO: remove hardcode
{
	// Asks vcmi to automatically detect encoding
	"auto",
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

/// List of tags of languages that can be selected from Launcher (and have translation for Launcher)
static const std::string languageTagList[] =
{
	"english",
	"german",
	"polish",
	"russian",
	"ukrainian",
};

static const std::string cursorTypesList[] =
{
	"auto",
	"hardware",
	"software"
};

void CSettingsView::setDisplayList()
{
	QStringList list;

	for (const auto screen : QGuiApplication::screens())
		list << QString{"%1 - %2"}.arg(screen->name(), resolutionToString(screen->size()));

	if(list.count() < 2)
	{
		ui->comboBoxDisplayIndex->hide();
		ui->labelDisplayIndex->hide();
		fillValidResolutionsForScreen(0);
	}
	else
	{
		int displayIndex = settings["video"]["displayIndex"].Integer();
		ui->comboBoxDisplayIndex->addItems(list);
		// calls fillValidResolutions() in slot
		ui->comboBoxDisplayIndex->setCurrentIndex(displayIndex);
	}
}

void CSettingsView::loadSettings()
{
	ui->comboBoxShowIntro->setCurrentIndex(settings["video"]["showIntro"].Bool());

#ifdef Q_OS_IOS
	ui->comboBoxFullScreen->setCurrentIndex(true);
	ui->comboBoxFullScreen->setDisabled(true);
#else
	ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
	if (settings["video"]["realFullscreen"].Bool())
		ui->comboBoxFullScreen->setCurrentIndex(2);
#endif

	ui->comboBoxFriendlyAI->setCurrentText(QString::fromStdString(settings["server"]["friendlyAI"].String()));
	ui->comboBoxNeutralAI->setCurrentText(QString::fromStdString(settings["server"]["neutralAI"].String()));
	ui->comboBoxEnemyAI->setCurrentText(QString::fromStdString(settings["server"]["enemyAI"].String()));
	ui->comboBoxPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["playerAI"].String()));

	ui->spinBoxNetworkPort->setValue(settings["server"]["port"].Integer());

	ui->comboBoxAutoCheck->setCurrentIndex(settings["launcher"]["autoCheckRepositories"].Bool());
	// all calls to plainText will trigger textChanged() signal overwriting config. Create backup before editing widget
	JsonNode urls = settings["launcher"]["repositoryURL"];

	ui->plainTextEditRepos->clear();
	for(auto entry : urls.Vector())
		ui->plainTextEditRepos->appendPlainText(QString::fromUtf8(entry.String().c_str()));

	ui->lineEditUserDataDir->setText(pathToQString(VCMIDirs::get().userDataPath()));
	ui->lineEditGameDir->setText(pathToQString(VCMIDirs::get().binaryPath()));
	ui->lineEditTempDir->setText(pathToQString(VCMIDirs::get().userLogsPath()));

	std::string encoding = settings["general"]["encoding"].String();
	size_t encodingIndex = boost::range::find(knownEncodingsList, encoding) - knownEncodingsList;
	if(encodingIndex < ui->comboBoxEncoding->count())
		ui->comboBoxEncoding->setCurrentIndex((int)encodingIndex);
	ui->comboBoxAutoSave->setCurrentIndex(settings["general"]["saveFrequency"].Integer() > 0 ? 1 : 0);

	std::string language = settings["general"]["language"].String();
	size_t languageIndex = boost::range::find(languageTagList, language) - languageTagList;
	if(languageIndex < ui->comboBoxLanguage->count())
		ui->comboBoxLanguage->setCurrentIndex((int)languageIndex);

	std::string cursorType = settings["video"]["cursor"].String();
	size_t cursorTypeIndex = boost::range::find(cursorTypesList, cursorType) - cursorTypesList;
	ui->comboBoxCursorType->setCurrentIndex((int)cursorTypeIndex);
}

void CSettingsView::fillValidResolutions(bool isExtraResolutionsModEnabled)
{
	this->isExtraResolutionsModEnabled = isExtraResolutionsModEnabled;
	fillValidResolutionsForScreen(ui->comboBoxDisplayIndex->isVisible() ? ui->comboBoxDisplayIndex->currentIndex() : 0);
}

void CSettingsView::fillValidResolutionsForScreen(int screenIndex)
{
	ui->comboBoxResolution->blockSignals(true); // avoid saving wrong resolution after adding first item from the list
	ui->comboBoxResolution->clear();

	// TODO: read available resolutions from all mods
	QVariantList resolutions;
	if(isExtraResolutionsModEnabled)
	{
		const auto extrasResolutionsPath = settings["launcher"]["extraResolutionsModPath"].String().c_str();
		const auto extrasResolutionsJson = JsonUtils::JsonFromFile(CLauncherDirs::get().modsPath() + extrasResolutionsPath);
		resolutions = extrasResolutionsJson.toMap().value(QLatin1String{"GUISettings"}).toList();
	}
	if(resolutions.isEmpty())
	{
		ui->comboBoxResolution->blockSignals(false);
		ui->comboBoxResolution->addItem(resolutionToString({800, 600}));
		return;
	}

	const auto screens = qGuiApp->screens();
	const auto currentScreen = screenIndex < screens.size() ? screens[screenIndex] : qGuiApp->primaryScreen();
	const auto screenSize = currentScreen->size();
	MAYBE_UNUSED(screenSize);

	for(const auto & entry : resolutions)
	{
		const auto resolutionMap = entry.toMap().value(QLatin1String{"resolution"}).toMap();
		if(resolutionMap.isEmpty())
			continue;

		const auto widthValue = resolutionMap[QLatin1String{"x"}];
		const auto heightValue = resolutionMap[QLatin1String{"y"}];
		if(!widthValue.isValid() || !heightValue.isValid())
			continue;

		const QSize resolution{widthValue.toInt(), heightValue.toInt()};
#ifndef VCMI_IOS
		if(screenSize.width() < resolution.width() || screenSize.height() < resolution.height())
			continue;
#endif
		ui->comboBoxResolution->addItem(resolutionToString(resolution));
	}

	int resX = settings["video"]["screenRes"]["width"].Integer();
	int resY = settings["video"]["screenRes"]["height"].Integer();
	int resIndex = ui->comboBoxResolution->findText(resolutionToString({resX, resY}));
	ui->comboBoxResolution->setCurrentIndex(resIndex);

	ui->comboBoxResolution->blockSignals(false);

	// if selected resolution no longer exists, force update value to the first resolution
	if(resIndex == -1)
		ui->comboBoxResolution->setCurrentIndex(0);
}

CSettingsView::CSettingsView(QWidget * parent)
	: QWidget(parent), ui(new Ui::CSettingsView)
{
	ui->setupUi(this);

	ui->lineEditBuildVersion->setText(QString::fromStdString(GameConstants::VCMI_VERSION));
	loadSettings();
}

CSettingsView::~CSettingsView()
{
	delete ui;
}


void CSettingsView::on_comboBoxResolution_currentTextChanged(const QString & arg1)
{
	QStringList list = arg1.split("x");

	Settings node = settings.write["video"]["screenRes"];
	node["width"].Float() = list[0].toInt();
	node["height"].Float() = list[1].toInt();
}

void CSettingsView::on_comboBoxFullScreen_currentIndexChanged(int index)
{
	Settings nodeFullscreen     = settings.write["video"]["fullscreen"];
	Settings nodeRealFullscreen = settings.write["video"]["realFullscreen"];
	nodeFullscreen->Bool() = (index != 0);
	nodeRealFullscreen->Bool() = (index == 2);
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

	fillValidResolutionsForScreen(index);
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


void CSettingsView::on_comboBoxLanguage_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["language"];
	node->String() = languageTagList[index];

	if ( auto mainWindow = dynamic_cast<MainWindow*>(qApp->activeWindow()) )
		mainWindow->updateTranslation();
}

void CSettingsView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QWidget::changeEvent(event);
}

void CSettingsView::on_comboBoxCursorType_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["cursor"];
	node->String() = cursorTypesList[index];
}


void CSettingsView::on_listWidgetSettings_currentRowChanged(int currentRow)
{
	QVector<QWidget*> targetWidgets = {
		ui->labelGeneral,
		ui->labelVideo,
		ui->labelArtificialIntelligence,
		ui->labelDataDirs,
		ui->labelRepositories
	};

	QWidget * currentTarget = targetWidgets[currentRow];

	// We want to scroll in a way that will put target widget in topmost visible position
	// To show not just header, but all settings in this group as well
	// In order to do that, let's scroll to the very bottom and the scroll back up until target widget is visible
	int maxPosition = ui->settingsScrollArea->verticalScrollBar()->maximum();
	ui->settingsScrollArea->verticalScrollBar()->setValue(maxPosition);
	ui->settingsScrollArea->ensureWidgetVisible(currentTarget, 5, 5);

}
