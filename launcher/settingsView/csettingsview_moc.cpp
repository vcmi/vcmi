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

#include "../modManager/cmodlistview_moc.h"
#include "../helper.h"
#include "../jsonutils.h"
#include "../languages.h"

#include <QFileInfo>
#include <QGuiApplication>

#include "../../lib/CConfigHandler.h"

#ifndef VCMI_MOBILE
#include <SDL2/SDL.h>
#endif

namespace
{
QString resolutionToString(const QSize & resolution)
{
	return QString{"%1x%2"}.arg(resolution.width()).arg(resolution.height());
}

static constexpr std::array cursorTypesList =
{
	"hardware",
	"software"
};

static constexpr std::array upscalingFilterTypes =
{
	"nearest",
	"linear",
	"best"
};

}

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

#ifdef VCMI_MOBILE
	ui->comboBoxFullScreen->hide();
	ui->labelFullScreen->hide();
#else
	ui->labelReservedArea->hide();
	ui->sliderReservedArea->hide();
	ui->comboBoxRelativeCursorMode->hide();
	ui->sliderRelativeCursorSpeed->hide();
	if (settings["video"]["realFullscreen"].Bool())
		ui->comboBoxFullScreen->setCurrentIndex(2);
	else
		ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
#endif
	fillValidScalingRange();

	ui->spinBoxInterfaceScaling->setValue(settings["video"]["resolution"]["scaling"].Float());
	ui->spinBoxFramerateLimit->setValue(settings["video"]["targetfps"].Float());
	ui->spinBoxFramerateLimit->setDisabled(settings["video"]["vsync"].Bool());
	ui->comboBoxVSync->setCurrentIndex(settings["video"]["vsync"].Bool());
	ui->sliderReservedArea->setValue(std::round(settings["video"]["reservedWidth"].Float() * 100));

	ui->comboBoxFriendlyAI->setCurrentText(QString::fromStdString(settings["server"]["friendlyAI"].String()));
	ui->comboBoxNeutralAI->setCurrentText(QString::fromStdString(settings["server"]["neutralAI"].String()));
	ui->comboBoxEnemyAI->setCurrentText(QString::fromStdString(settings["server"]["enemyAI"].String()));

	ui->comboBoxEnemyPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["playerAI"].String()));
	ui->comboBoxAlliedPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["alliedAI"].String()));

	ui->spinBoxNetworkPort->setValue(settings["server"]["localPort"].Integer());

	ui->comboBoxAutoCheck->setCurrentIndex(settings["launcher"]["autoCheckRepositories"].Bool());

	ui->lineEditRepositoryDefault->setText(QString::fromStdString(settings["launcher"]["defaultRepositoryURL"].String()));
	ui->lineEditRepositoryExtra->setText(QString::fromStdString(settings["launcher"]["extraRepositoryURL"].String()));

	ui->lineEditRepositoryDefault->setEnabled(settings["launcher"]["defaultRepositoryEnabled"].Bool());
	ui->lineEditRepositoryExtra->setEnabled(settings["launcher"]["extraRepositoryEnabled"].Bool());

	ui->checkBoxRepositoryDefault->setChecked(settings["launcher"]["defaultRepositoryEnabled"].Bool());
	ui->checkBoxRepositoryExtra->setChecked(settings["launcher"]["extraRepositoryEnabled"].Bool());

	ui->checkBoxIgnoreSslErrors->setChecked(settings["launcher"]["ignoreSslErrors"].Bool());

	ui->comboBoxAutoSave->setCurrentIndex(settings["general"]["saveFrequency"].Integer() > 0 ? 1 : 0);

	ui->spinBoxAutoSaveLimit->setValue(settings["general"]["autosaveCountLimit"].Integer());

	ui->checkBoxAutoSavePrefix->setChecked(settings["general"]["useSavePrefix"].Bool());

	ui->lineEditAutoSavePrefix->setText(QString::fromStdString(settings["general"]["savePrefix"].String()));
	ui->lineEditAutoSavePrefix->setEnabled(settings["general"]["useSavePrefix"].Bool());

	Languages::fillLanguages(ui->comboBoxLanguage, false);
	fillValidRenderers();

	std::string cursorType = settings["video"]["cursor"].String();
	int cursorTypeIndex = vstd::find_pos(cursorTypesList, cursorType);
	ui->comboBoxCursorType->setCurrentIndex(cursorTypeIndex);

	std::string upscalingFilter = settings["video"]["scalingMode"].String();
	int upscalingFilterIndex = vstd::find_pos(upscalingFilterTypes, upscalingFilter);
	ui->comboBoxUpscalingFilter->setCurrentIndex(upscalingFilterIndex);

	ui->sliderMusicVolume->setValue(settings["general"]["music"].Integer());
	ui->sliderSoundVolume->setValue(settings["general"]["sound"].Integer());
	ui->comboBoxRelativeCursorMode->setCurrentIndex(settings["general"]["userRelativePointer"].Bool());
	ui->sliderRelativeCursorSpeed->setValue(settings["general"]["relativePointerSpeedMultiplier"].Integer());
	ui->comboBoxHapticFeedback->setCurrentIndex(settings["launcher"]["hapticFeedback"].Bool());
	ui->sliderLongTouchDuration->setValue(settings["general"]["longTouchTimeMilliseconds"].Integer());
	ui->slideToleranceDistanceMouse->setValue(settings["input"]["mouseToleranceDistance"].Integer());
	ui->sliderToleranceDistanceTouch->setValue(settings["input"]["touchToleranceDistance"].Integer());
	ui->sliderToleranceDistanceController->setValue(settings["input"]["shortcutToleranceDistance"].Integer());
	ui->sliderControllerSticksSensitivity->setValue(settings["input"]["controllerAxisSpeed"].Integer());
	ui->sliderControllerSticksAcceleration->setValue(settings["input"]["controllerAxisScale"].Float() * 100);
	ui->lineEditGameLobbyHost->setText(QString::fromStdString(settings["lobby"]["hostname"].String()));
	ui->spinBoxNetworkPortLobby->setValue(settings["lobby"]["port"].Integer());
}

void CSettingsView::fillValidResolutions()
{
	fillValidResolutionsForScreen(ui->comboBoxDisplayIndex->isVisible() ? ui->comboBoxDisplayIndex->currentIndex() : 0);
}

QSize CSettingsView::getPreferredRenderingResolution()
{
#ifndef VCMI_MOBILE
	bool fullscreen = settings["video"]["fullscreen"].Bool();
	bool realFullscreen = settings["video"]["realFullscreen"].Bool();

	if (!fullscreen || realFullscreen)
	{
		int resX = settings["video"]["resolution"]["width"].Integer();
		int resY = settings["video"]["resolution"]["height"].Integer();
		return QSize(resX, resY);
	}
#endif
	return QApplication::primaryScreen()->geometry().size() * QApplication::primaryScreen()->devicePixelRatio();
}

void CSettingsView::fillValidScalingRange()
{
	//FIXME: this code is copy of ScreenHandler::getSupportedScalingRange

	// H3 resolution, any resolution smaller than that is not correctly supported
	static const QSize minResolution = {800, 600};
	// arbitrary limit on *downscaling*. Allow some downscaling, if requested by user. Should be generally limited to 100+ for all but few devices
	static const double minimalScaling = 50;

	QSize renderResolution = getPreferredRenderingResolution();
	double maximalScalingWidth = 100.0 * renderResolution.width() / minResolution.width();
	double maximalScalingHeight = 100.0 * renderResolution.height() / minResolution.height();
	double maximalScaling = std::min(maximalScalingWidth, maximalScalingHeight);

	ui->spinBoxInterfaceScaling->setRange(minimalScaling, maximalScaling);
}

#ifndef VCMI_MOBILE

static QStringList getAvailableRenderingDrivers()
{
	SDL_Init(SDL_INIT_VIDEO);
	QStringList result;

	result += QString(); // empty value for autoselection

	int driversCount = SDL_GetNumRenderDrivers();

	for(int it = 0; it < driversCount; it++)
	{
		SDL_RendererInfo info;
		if (SDL_GetRenderDriverInfo(it, &info) == 0)
			result += QString::fromLatin1(info.name);
	}

	SDL_Quit();
	return result;
}

static QVector<QSize> findAvailableResolutions(int displayIndex)
{
	// Ugly workaround since we don't actually need SDL in Launcher
	// However Qt at the moment provides no way to query list of available resolutions
	QVector<QSize> result;
	SDL_Init(SDL_INIT_VIDEO);

	int modesCount = SDL_GetNumDisplayModes(displayIndex);

	for (int i =0; i < modesCount; ++i)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(displayIndex, i, &mode) != 0)
			continue;

		QSize resolution(mode.w, mode.h);

		result.push_back(resolution);
	}

	boost::range::sort(result, [](const auto & left, const auto & right)
	{
		return left.height() * left.width() < right.height() * right.width();
	});

	result.erase(boost::unique(result).end(), result.end());

	SDL_Quit();

	return result;
}

void CSettingsView::fillValidResolutionsForScreen(int screenIndex)
{
	QSignalBlocker guard(ui->comboBoxResolution); // avoid saving wrong resolution after adding first item from the list

	ui->comboBoxResolution->clear();

	bool fullscreen = settings["video"]["fullscreen"].Bool();
	bool realFullscreen = settings["video"]["realFullscreen"].Bool();

	if (!fullscreen || realFullscreen)
	{
		QVector<QSize> resolutions = findAvailableResolutions(screenIndex);

		for(const auto & entry : resolutions)
			ui->comboBoxResolution->addItem(resolutionToString(entry));
	}
	else
	{
		ui->comboBoxResolution->addItem(resolutionToString(getPreferredRenderingResolution()));
	}
	ui->comboBoxResolution->setEnabled(ui->comboBoxResolution->count() > 1);

	int resX = settings["video"]["resolution"]["width"].Integer();
	int resY = settings["video"]["resolution"]["height"].Integer();
	int resIndex = ui->comboBoxResolution->findText(resolutionToString({resX, resY}));
	ui->comboBoxResolution->setCurrentIndex(resIndex);

	// if selected resolution no longer exists, force update value to the largest (last) resolution
	if(resIndex == -1)
		ui->comboBoxResolution->setCurrentIndex(ui->comboBoxResolution->count() - 1);
}

void CSettingsView::fillValidRenderers()
{
	QSignalBlocker guard(ui->comboBoxRendererType); // avoid saving wrong renderer after adding first item from the list

	ui->comboBoxRendererType->clear();

	auto driversList = getAvailableRenderingDrivers();
	ui->comboBoxRendererType->addItems(driversList);

	std::string rendererName = settings["video"]["driver"].String();

	int index = ui->comboBoxRendererType->findText(QString::fromStdString(rendererName));
	ui->comboBoxRendererType->setCurrentIndex(index);
}
#else
void CSettingsView::fillValidResolutionsForScreen(int screenIndex)
{
	// resolutions are not selectable on mobile platforms
	ui->comboBoxResolution->hide();
	ui->labelResolution->hide();
}

void CSettingsView::fillValidRenderers()
{
	// untested on mobile platforms
	ui->comboBoxRendererType->hide();
	ui->labelRendererType->hide();
}
#endif

CSettingsView::CSettingsView(QWidget * parent)
	: QWidget(parent), ui(new Ui::CSettingsView)
{
	ui->setupUi(this);
	Helper::enableScrollBySwiping(ui->settingsScrollArea);

	loadSettings();
}

CSettingsView::~CSettingsView()
{
	delete ui;
}

void CSettingsView::on_comboBoxResolution_currentTextChanged(const QString & arg1)
{
	QStringList list = arg1.split("x");

	Settings node = settings.write["video"]["resolution"];
	node["width"].Float() = list[0].toInt();
	node["height"].Float() = list[1].toInt();

	fillValidResolutions();
	fillValidScalingRange();
}

void CSettingsView::on_comboBoxFullScreen_currentIndexChanged(int index)
{
	Settings nodeFullscreen     = settings.write["video"]["fullscreen"];
	Settings nodeRealFullscreen = settings.write["video"]["realFullscreen"];
	nodeFullscreen->Bool() = (index != 0);
	nodeRealFullscreen->Bool() = (index == 2);

	fillValidResolutions();
	fillValidScalingRange();
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

void CSettingsView::on_comboBoxFriendlyAI_currentTextChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["friendlyAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxNeutralAI_currentTextChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["neutralAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxEnemyAI_currentTextChanged(const QString & arg1)
{
	Settings node = settings.write["server"]["enemyAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_spinBoxNetworkPort_valueChanged(int arg1)
{
	Settings node = settings.write["server"]["port"];
	node->Float() = arg1;
}

void CSettingsView::on_comboBoxShowIntro_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["showIntro"];
	node->Bool() = index;
}

void CSettingsView::on_comboBoxAutoSave_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["saveFrequency"];
	node->Integer() = index;
}

void CSettingsView::on_comboBoxLanguage_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["language"];
	QString selectedLanguage = ui->comboBoxLanguage->itemData(index).toString();
	node->String() = selectedLanguage.toStdString();

	if(auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow()))
		mainWindow->updateTranslation();
}

void CSettingsView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		Languages::fillLanguages(ui->comboBoxLanguage, false);
		loadTranslation();
	}
	QWidget::changeEvent(event);
}

void CSettingsView::showEvent(QShowEvent * event)
{
	loadTranslation();
	QWidget::showEvent(event);
}

void CSettingsView::on_comboBoxCursorType_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["cursor"];
	node->String() = cursorTypesList[index];
}

void CSettingsView::loadTranslation()
{
	QString baseLanguage = Languages::getHeroesDataLanguage();

	auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow());

	if (!mainWindow)
		return;

	QString languageName = QString::fromStdString(settings["general"]["language"].String());
	QString modName = mainWindow->getModView()->getTranslationModName(languageName);
	bool translationExists = !modName.isEmpty();
	bool translationNeeded = languageName != baseLanguage;
	bool showTranslation = translationNeeded && translationExists;

	ui->labelTranslation->setVisible(showTranslation);
	ui->labelTranslationStatus->setVisible(showTranslation);
	ui->pushButtonTranslation->setVisible(showTranslation);

	if (!translationExists || !translationNeeded)
		return;

	bool translationAvailable = mainWindow->getModView()->isModAvailable(modName);
	bool translationEnabled = mainWindow->getModView()->isModEnabled(modName);

	ui->pushButtonTranslation->setVisible(!translationEnabled);

	if (translationEnabled)
	{
		ui->labelTranslationStatus->setText(tr("Active"));
	}

	if (!translationEnabled && !translationAvailable)
	{
		ui->labelTranslationStatus->setText(tr("Disabled"));
		ui->pushButtonTranslation->setText(tr("Enable"));
	}

	if (translationAvailable)
	{
		ui->labelTranslationStatus->setText(tr("Not Installed"));
		ui->pushButtonTranslation->setText(tr("Install"));
	}
}

void CSettingsView::on_pushButtonTranslation_clicked()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow());

	assert(mainWindow);
	if (!mainWindow)
		return;

	QString languageName = QString::fromStdString(settings["general"]["language"].String());
	QString modName = mainWindow->getModView()->getTranslationModName(languageName);

	assert(!modName.isEmpty());
	if (modName.isEmpty())
		return;

	if (mainWindow->getModView()->isModAvailable(modName))
	{
		mainWindow->switchToModsTab();
		mainWindow->getModView()->doInstallMod(modName);
	}
	else
	{
		mainWindow->getModView()->enableModByName(modName);
	}
}

void CSettingsView::on_checkBoxRepositoryDefault_stateChanged(int arg1)
{
	Settings node = settings.write["launcher"]["defaultRepositoryEnabled"];
	node->Bool() = arg1;
	ui->lineEditRepositoryDefault->setEnabled(arg1);
}

void CSettingsView::on_checkBoxRepositoryExtra_stateChanged(int arg1)
{
	Settings node = settings.write["launcher"]["extraRepositoryEnabled"];
	node->Bool() = arg1;
	ui->lineEditRepositoryExtra->setEnabled(arg1);
}

void CSettingsView::on_lineEditRepositoryExtra_textEdited(const QString &arg1)
{
	Settings node = settings.write["launcher"]["extraRepositoryURL"];
	node->String() = arg1.toStdString();
}

void CSettingsView::on_spinBoxInterfaceScaling_valueChanged(int arg1)
{
	Settings node = settings.write["video"]["resolution"]["scaling"];
	node->Float() = arg1;
}

void CSettingsView::on_refreshRepositoriesButton_clicked()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow());

	assert(mainWindow);
	if (!mainWindow)
		return;

	mainWindow->getModView()->loadRepositories();
}

void CSettingsView::on_spinBoxFramerateLimit_valueChanged(int arg1)
{
	Settings node = settings.write["video"]["targetfps"];
	node->Float() = arg1;
}

void CSettingsView::on_checkBoxVSync_stateChanged(int arg1)
{
	Settings node = settings.write["video"]["vsync"];
	node->Bool() = arg1;
	ui->spinBoxFramerateLimit->setDisabled(settings["video"]["vsync"].Bool());
}

void CSettingsView::on_comboBoxEnemyPlayerAI_currentTextChanged(const QString &arg1)
{
	Settings node = settings.write["server"]["playerAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_comboBoxAlliedPlayerAI_currentTextChanged(const QString &arg1)
{
	Settings node = settings.write["server"]["alliedAI"];
	node->String() = arg1.toUtf8().data();
}

void CSettingsView::on_checkBoxAutoSavePrefix_stateChanged(int arg1)
{
	Settings node = settings.write["general"]["useSavePrefix"];
	node->Bool() = arg1;
	ui->lineEditAutoSavePrefix->setEnabled(arg1);
}

void CSettingsView::on_spinBoxAutoSaveLimit_valueChanged(int arg1)
{
	Settings node = settings.write["general"]["autosaveCountLimit"];
	node->Float() = arg1;
}

void CSettingsView::on_lineEditAutoSavePrefix_textEdited(const QString & arg1)
{
	Settings node = settings.write["general"]["savePrefix"];
	node->String() = arg1.toStdString();
}

void CSettingsView::on_spinBoxReservedArea_valueChanged(int arg1)
{
	Settings node = settings.write["video"]["reservedWidth"];
	node->Float() = float(arg1) / 100; // percentage -> ratio
}

void CSettingsView::on_comboBoxRendererType_currentTextChanged(const QString &arg1)
{
	Settings node = settings.write["video"]["driver"];
	node->String() = arg1.toStdString();
}

void CSettingsView::on_checkBoxIgnoreSslErrors_clicked(bool checked)
{
	Settings node = settings.write["launcher"]["ignoreSslErrors"];
	node->Bool() = checked;
}

void CSettingsView::on_comboBoxUpscalingFilter_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["scalingMode"];
	node->String() = upscalingFilterTypes[index];
}

void CSettingsView::on_sliderMusicVolume_valueChanged(int value)
{
	Settings node = settings.write["general"]["music"];
	node->Integer() = value;
}

void CSettingsView::on_sliderSoundVolume_valueChanged(int value)
{
	Settings node = settings.write["general"]["sound"];
	node->Integer() = value;
}

void CSettingsView::on_comboBoxRelativeCursorMode_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["userRelativePointer"];
	node->Bool() = index;
}

void CSettingsView::on_sliderRelativeCursorSpeed_valueChanged(int value)
{
	Settings node = settings.write["general"]["relativePointerSpeedMultiplier"];
	node->Float() = value / 100.0;
}

void CSettingsView::on_comboBoxHapticFeedback_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["hapticFeedback"];
	node->Bool() = index;
}

void CSettingsView::on_sliderLongTouchDuration_valueChanged(int value)
{
	Settings node = settings.write["general"]["longTouchTimeMilliseconds"];
	node->Integer() = value;
}

void CSettingsView::on_slideToleranceDistanceMouse_valueChanged(int value)
{
	Settings node = settings.write["input"]["mouseToleranceDistance"];
	node->Integer() = value;
}

void CSettingsView::on_sliderToleranceDistanceTouch_valueChanged(int value)
{
	Settings node = settings.write["input"]["touchToleranceDistance"];
	node->Integer() = value;
}

void CSettingsView::on_sliderToleranceDistanceController_valueChanged(int value)
{
	Settings node = settings.write["input"]["shortcutToleranceDistance"];
	node->Integer() = value;
}

void CSettingsView::on_lineEditGameLobbyHost_textChanged(const QString & arg1)
{
	Settings node = settings.write["lobby"]["hostname"];
	node->String() = arg1.toStdString();
}

void CSettingsView::on_spinBoxNetworkPortLobby_valueChanged(int arg1)
{
	Settings node = settings.write["lobby"]["port"];
	node->Integer() = arg1;
}

void CSettingsView::on_sliderControllerSticksAcceleration_valueChanged(int value)
{
	Settings node = settings.write["input"]["configAxisScale"];
	node->Integer() = value / 100.0;
}

void CSettingsView::on_sliderControllerSticksSensitivity_valueChanged(int value)
{
	Settings node = settings.write["input"]["configAxisSpeed"];
	node->Integer() = value;
}
