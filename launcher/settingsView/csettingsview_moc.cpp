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
#include "../vcmiqt/jsonutils.h"
#include "../languages.h"

#include <QFileInfo>
#include <QGuiApplication>

#include "../../lib/CConfigHandler.h"

#ifndef VCMI_MOBILE
#include <SDL2/SDL.h>
#endif

static QString resolutionToString(const QSize & resolution)
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
	"auto",
	"none",
	"xbrz2",
	"xbrz3",
	"xbrz4"
};

static constexpr std::array downscalingFilterTypes =
{
	"nearest",
	"linear",
	"best"
};

MainWindow * CSettingsView::getMainWindow()
{
	foreach(QWidget *w, qApp->allWidgets())
		if(QMainWindow* mainWin = qobject_cast<QMainWindow*>(w))
			return dynamic_cast<MainWindow *>(mainWin);
	return nullptr;
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

void CSettingsView::setCheckbuttonState(QToolButton * button, bool checked)
{
	button->setChecked(checked);
	updateCheckbuttonText(button);
}

void CSettingsView::updateCheckbuttonText(QToolButton * button)
{
	if (button->isChecked())
		button->setText(tr("On"));
	else
		button->setText(tr("Off"));
}

void CSettingsView::loadSettings()
{
#ifdef VCMI_MOBILE
	ui->comboBoxFullScreen->hide();
	ui->labelFullScreen->hide();

	if(!persistentStorage["gui"]["tutorialCompleted0"].Bool() && !persistentStorage["gui"]["tutorialCompleted1"].Bool())
	{
		ui->labelResetTutorialTouchscreen->hide();
		ui->pushButtonResetTutorialTouchscreen->hide();		
	}
#else
	ui->labelReservedArea->hide();
	ui->sliderReservedArea->hide();
	ui->labelRelativeCursorMode->hide();
	ui->buttonRelativeCursorMode->hide();
	ui->sliderRelativeCursorSpeed->hide();
	ui->labelRelativeCursorSpeed->hide();
	ui->buttonHapticFeedback->hide();
	ui->labelHapticFeedback->hide();
	ui->labelResetTutorialTouchscreen->hide();
	ui->pushButtonResetTutorialTouchscreen->hide();
	ui->labelAllowPortrait->hide();
	ui->buttonAllowPortrait->hide();
	if (settings["video"]["realFullscreen"].Bool())
		ui->comboBoxFullScreen->setCurrentIndex(2);
	else
		ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
#endif
#ifndef VCMI_ANDROID
	ui->buttonHandleBackRightMouseButton->hide();
	ui->labelHandleBackRightMouseButton->hide();
#endif
	fillValidScalingRange();

	ui->buttonScalingAuto->setChecked(settings["video"]["resolution"]["scaling"].Integer() == 0);
	if (settings["video"]["resolution"]["scaling"].Integer() == 0)
		ui->spinBoxInterfaceScaling->setValue(100);
	else
		ui->spinBoxInterfaceScaling->setValue(settings["video"]["resolution"]["scaling"].Float());

	ui->spinBoxFramerateLimit->setValue(settings["video"]["targetfps"].Float());
	ui->spinBoxFramerateLimit->setDisabled(settings["video"]["vsync"].Bool());
	ui->sliderReservedArea->setValue(std::round(settings["video"]["reservedWidth"].Float() * 100));

	ui->comboBoxFriendlyAI->setCurrentText(QString::fromStdString(settings["server"]["friendlyAI"].String()));
	ui->comboBoxNeutralAI->setCurrentText(QString::fromStdString(settings["server"]["neutralAI"].String()));
	ui->comboBoxEnemyAI->setCurrentText(QString::fromStdString(settings["server"]["enemyAI"].String()));

	ui->comboBoxEnemyPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["playerAI"].String()));
	ui->comboBoxAlliedPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["alliedAI"].String()));

	ui->spinBoxNetworkPort->setValue(settings["server"]["localPort"].Integer());

	ui->lineEditRepositoryDefault->setText(QString::fromStdString(settings["launcher"]["defaultRepositoryURL"].String()));
	ui->lineEditRepositoryExtra->setText(QString::fromStdString(settings["launcher"]["extraRepositoryURL"].String()));

	ui->lineEditRepositoryDefault->setEnabled(settings["launcher"]["defaultRepositoryEnabled"].Bool());
	ui->lineEditRepositoryExtra->setEnabled(settings["launcher"]["extraRepositoryEnabled"].Bool());

	ui->spinBoxAutoSaveLimit->setValue(settings["general"]["autosaveCountLimit"].Integer());

	ui->lineEditAutoSavePrefix->setText(QString::fromStdString(settings["general"]["savePrefix"].String()));
	ui->lineEditAutoSavePrefix->setEnabled(settings["general"]["useSavePrefix"].Bool());

	Languages::fillLanguages(ui->comboBoxLanguage, false);
	fillValidRenderers();

	std::string upscalingFilter = settings["video"]["upscalingFilter"].String();
	int upscalingFilterIndex = vstd::find_pos(upscalingFilterTypes, upscalingFilter);
	ui->comboBoxUpscalingFilter->setCurrentIndex(upscalingFilterIndex);

	std::string downscalingFilter = settings["video"]["downscalingFilter"].String();
	int downscalingFilterIndex = vstd::find_pos(downscalingFilterTypes, downscalingFilter);
	ui->comboBoxDownscalingFilter->setCurrentIndex(downscalingFilterIndex);

	ui->sliderMusicVolume->setValue(settings["general"]["music"].Integer());
	ui->sliderSoundVolume->setValue(settings["general"]["sound"].Integer());
	ui->sliderRelativeCursorSpeed->setValue(settings["general"]["relativePointerSpeedMultiplier"].Integer());
	ui->sliderLongTouchDuration->setValue(settings["general"]["longTouchTimeMilliseconds"].Integer());
	ui->slideToleranceDistanceMouse->setValue(settings["input"]["mouseToleranceDistance"].Integer());
	ui->buttonHandleBackRightMouseButton->setChecked(settings["input"]["handleBackRightMouseButton"].Bool());
	ui->sliderToleranceDistanceTouch->setValue(settings["input"]["touchToleranceDistance"].Integer());
	ui->sliderToleranceDistanceController->setValue(settings["input"]["shortcutToleranceDistance"].Integer());
	ui->sliderControllerSticksSensitivity->setValue(settings["input"]["controllerAxisSpeed"].Integer());
	ui->sliderControllerSticksAcceleration->setValue(settings["input"]["controllerAxisScale"].Float() * 100);
	ui->lineEditGameLobbyHost->setText(QString::fromStdString(settings["lobby"]["hostname"].String()));
	ui->spinBoxNetworkPortLobby->setValue(settings["lobby"]["port"].Integer());
	ui->buttonVSync->setChecked(settings["video"]["vsync"].Bool());

	if (settings["video"]["fontsType"].String() == "auto")
		ui->buttonFontAuto->setChecked(true);
	else if (settings["video"]["fontsType"].String() == "original")
		ui->buttonFontOriginal->setChecked(true);
	else
		ui->buttonFontScalable->setChecked(true);

	if (settings["mods"]["validation"].String() == "off")
		ui->buttonValidationOff->setChecked(true);
	else if (settings["mods"]["validation"].String() == "basic")
		ui->buttonValidationBasic->setChecked(true);
	else
		ui->buttonValidationFull->setChecked(true);

	loadToggleButtonSettings();
}

void CSettingsView::loadToggleButtonSettings()
{
	setCheckbuttonState(ui->buttonShowIntro, settings["video"]["showIntro"].Bool());
	setCheckbuttonState(ui->buttonAllowPortrait, settings["video"]["allowPortrait"].Bool());
	setCheckbuttonState(ui->buttonAutoCheck, settings["launcher"]["autoCheckRepositories"].Bool());

	setCheckbuttonState(ui->buttonRepositoryDefault, settings["launcher"]["defaultRepositoryEnabled"].Bool());
	setCheckbuttonState(ui->buttonRepositoryExtra, settings["launcher"]["extraRepositoryEnabled"].Bool());

	setCheckbuttonState(ui->buttonIgnoreSslErrors, settings["launcher"]["ignoreSslErrors"].Bool());
	setCheckbuttonState(ui->buttonAutoSave, settings["general"]["saveFrequency"].Integer() > 0);

	setCheckbuttonState(ui->buttonAutoSavePrefix, settings["general"]["useSavePrefix"].Bool());

	setCheckbuttonState(ui->buttonRelativeCursorMode, settings["general"]["userRelativePointer"].Bool());
	setCheckbuttonState(ui->buttonHapticFeedback, settings["general"]["hapticFeedback"].Bool());

	setCheckbuttonState(ui->buttonHandleBackRightMouseButton, settings["input"]["handleBackRightMouseButton"].Bool());

	std::string cursorType = settings["video"]["cursor"].String();
	int cursorTypeIndex = vstd::find_pos(cursorTypesList, cursorType);
	setCheckbuttonState(ui->buttonCursorType, cursorTypeIndex);
	ui->sliderScalingCursor->setDisabled(cursorType == "software"); // Not supported
	ui->labelScalingCursorValue->setDisabled(cursorType == "software"); // Not supported

	int fontScalingPercentage = settings["video"]["fontScalingFactor"].Float() * 100;
	ui->sliderScalingFont->setValue(fontScalingPercentage / 5);

	int cursorScalingPercentage = settings["video"]["cursorScalingFactor"].Float() * 100;
	ui->sliderScalingCursor->setValue(cursorScalingPercentage / 5);

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

void CSettingsView::on_buttonAutoCheck_toggled(bool value)
{
	Settings node = settings.write["launcher"]["autoCheckRepositories"];
	node->Bool() = value;
	updateCheckbuttonText(ui->buttonAutoCheck);
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

void CSettingsView::on_buttonShowIntro_toggled(bool value)
{
	Settings node = settings.write["video"]["showIntro"];
	node->Bool() = value;
	updateCheckbuttonText(ui->buttonShowIntro);
}

void CSettingsView::on_buttonAllowPortrait_toggled(bool value)
{
	Settings node = settings.write["video"]["allowPortrait"];
	node->Bool() = value;
	updateCheckbuttonText(ui->buttonAllowPortrait);
}

void CSettingsView::on_buttonAutoSave_toggled(bool value)
{
	Settings node = settings.write["general"]["saveFrequency"];
	node->Integer() = value ? 1 : 0;
	updateCheckbuttonText(ui->buttonAutoSave);
}

void CSettingsView::on_comboBoxLanguage_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["language"];
	QString selectedLanguage = ui->comboBoxLanguage->itemData(index).toString();
	node->String() = selectedLanguage.toStdString();

	getMainWindow()->updateTranslation();
}

void CSettingsView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		Languages::fillLanguages(ui->comboBoxLanguage, false);
		loadTranslation();
		loadToggleButtonSettings();
	}
	QWidget::changeEvent(event);
}

void CSettingsView::showEvent(QShowEvent * event)
{
	loadTranslation();
	QWidget::showEvent(event);
}

void CSettingsView::on_buttonCursorType_toggled(bool value)
{
	Settings node = settings.write["video"]["cursor"];
	node->String() = cursorTypesList[value ? 1 : 0];
	updateCheckbuttonText(ui->buttonCursorType);
	ui->sliderScalingCursor->setDisabled(value == 1); // Not supported
	ui->labelScalingCursorValue->setDisabled(value == 1); // Not supported
}

void CSettingsView::loadTranslation()
{
	QString baseLanguage = Languages::getHeroesDataLanguage();

	auto * mainWindow = getMainWindow();

	if (!mainWindow)
		return;

	auto translationStatus = mainWindow->getTranslationStatus();
	bool showTranslation = translationStatus == ETranslationStatus::DISABLED || translationStatus == ETranslationStatus::NOT_INSTALLLED;

	ui->labelTranslation->setVisible(showTranslation);
	ui->labelTranslationStatus->setVisible(showTranslation);
	ui->pushButtonTranslation->setVisible(showTranslation);
	ui->pushButtonTranslation->setVisible(translationStatus != ETranslationStatus::ACTIVE);

	if (translationStatus == ETranslationStatus::ACTIVE)
	{
		ui->labelTranslationStatus->setText(tr("Active"));
	}

	if (translationStatus == ETranslationStatus::DISABLED)
	{
		ui->labelTranslationStatus->setText(tr("Disabled"));
		ui->pushButtonTranslation->setText(tr("Enable"));
	}

	if (translationStatus == ETranslationStatus::NOT_INSTALLLED)
	{
		ui->labelTranslationStatus->setText(tr("Not Installed"));
		ui->pushButtonTranslation->setText(tr("Install"));
	}
}

void CSettingsView::on_pushButtonTranslation_clicked()
{
	auto * mainWindow = getMainWindow();

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

void CSettingsView::on_pushButtonResetTutorialTouchscreen_clicked()
{
	Settings node0 = persistentStorage.write["gui"]["tutorialCompleted0"];
	node0->Bool() = false;
	Settings node1 = persistentStorage.write["gui"]["tutorialCompleted1"];
	node1->Bool() = false;

	ui->pushButtonResetTutorialTouchscreen->hide();
}

void CSettingsView::on_buttonRepositoryDefault_toggled(bool value)
{
	Settings node = settings.write["launcher"]["defaultRepositoryEnabled"];
	node->Bool() = value;
	ui->lineEditRepositoryDefault->setEnabled(value);
	updateCheckbuttonText(ui->buttonRepositoryDefault);
}

void CSettingsView::on_buttonRepositoryExtra_toggled(bool value)
{
	Settings node = settings.write["launcher"]["extraRepositoryEnabled"];
	node->Bool() = value;
	ui->lineEditRepositoryExtra->setEnabled(value);
	updateCheckbuttonText(ui->buttonRepositoryExtra);
}

void CSettingsView::on_lineEditRepositoryExtra_textEdited(const QString &arg1)
{
	Settings node = settings.write["launcher"]["extraRepositoryURL"];
	node->String() = arg1.toStdString();
}

void CSettingsView::on_spinBoxInterfaceScaling_valueChanged(int arg1)
{
	Settings node = settings.write["video"]["resolution"]["scaling"];
	node->Float() = ui->buttonScalingAuto->isChecked() ? 0 : arg1;
}

void CSettingsView::on_refreshRepositoriesButton_clicked()
{
	auto * mainWindow = getMainWindow();

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

void CSettingsView::on_buttonVSync_toggled(bool value)
{
	Settings node = settings.write["video"]["vsync"];
	node->Bool() = value;
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

void CSettingsView::on_buttonAutoSavePrefix_toggled(bool value)
{
	Settings node = settings.write["general"]["useSavePrefix"];
	node->Bool() = value;
	ui->lineEditAutoSavePrefix->setEnabled(value);
	updateCheckbuttonText(ui->buttonAutoSavePrefix);
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

void CSettingsView::on_sliderReservedArea_valueChanged(int arg1)
{
	Settings node = settings.write["video"]["reservedWidth"];
	node->Float() = float(arg1) / 100; // percentage -> ratio
}

void CSettingsView::on_comboBoxRendererType_currentTextChanged(const QString &arg1)
{
	Settings node = settings.write["video"]["driver"];
	node->String() = arg1.toStdString();
}

void CSettingsView::on_buttonIgnoreSslErrors_clicked(bool checked)
{
	Settings node = settings.write["launcher"]["ignoreSslErrors"];
	node->Bool() = checked;
	updateCheckbuttonText(ui->buttonIgnoreSslErrors);
}

void CSettingsView::on_comboBoxUpscalingFilter_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["upscalingFilter"];
	node->String() = upscalingFilterTypes[index];
}

void CSettingsView::on_comboBoxDownscalingFilter_currentIndexChanged(int index)
{
	Settings node = settings.write["video"]["downscalingFilter"];
	node->String() = downscalingFilterTypes[index];
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

void CSettingsView::on_buttonRelativeCursorMode_toggled(bool value)
{
	Settings node = settings.write["general"]["userRelativePointer"];
	node->Bool() = value;
	updateCheckbuttonText(ui->buttonRelativeCursorMode);
}

void CSettingsView::on_sliderRelativeCursorSpeed_valueChanged(int value)
{
	Settings node = settings.write["general"]["relativePointerSpeedMultiplier"];
	node->Float() = value / 100.0;
}

void CSettingsView::on_buttonHapticFeedback_toggled(bool value)
{
	Settings node = settings.write["general"]["hapticFeedback"];
	node->Bool() = value;
	updateCheckbuttonText(ui->buttonHapticFeedback);
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
	Settings node = settings.write["input"]["controllerAxisScale"];
	node->Integer() = value / 100.0;
}

void CSettingsView::on_sliderControllerSticksSensitivity_valueChanged(int value)
{
	Settings node = settings.write["input"]["controllerAxisSpeed"];
	node->Integer() = value;
}

void CSettingsView::on_sliderScalingFont_valueChanged(int value)
{
	int actualValuePercentage = value * 5;
	ui->labelScalingFontValue->setText(QString("%1%").arg(actualValuePercentage));
	Settings node = settings.write["video"]["fontScalingFactor"];
	node->Float() = actualValuePercentage / 100.0;
}

void CSettingsView::on_buttonFontAuto_clicked(bool checked)
{
	Settings node = settings.write["video"]["fontsType"];
	node->String() = "auto";
}

void CSettingsView::on_buttonFontScalable_clicked(bool checked)
{
	Settings node = settings.write["video"]["fontsType"];
	node->String() = "scalable";
}

void CSettingsView::on_buttonFontOriginal_clicked(bool checked)
{
	Settings node = settings.write["video"]["fontsType"];
	node->String() = "original";
}

void CSettingsView::on_buttonValidationOff_clicked(bool checked)
{
	Settings node = settings.write["mods"]["validation"];
	node->String() = "off";
}

void CSettingsView::on_buttonValidationBasic_clicked(bool checked)
{
	Settings node = settings.write["mods"]["validation"];
	node->String() = "basic";
}

void CSettingsView::on_buttonValidationFull_clicked(bool checked)
{
	Settings node = settings.write["mods"]["validation"];
	node->String() = "full";
}

void CSettingsView::on_sliderScalingCursor_valueChanged(int value)
{
	int actualValuePercentage = value * 5;
	ui->labelScalingCursorValue->setText(QString("%1%").arg(actualValuePercentage));
	Settings node = settings.write["video"]["cursorScalingFactor"];
	node->Float() = actualValuePercentage / 100.0;
}

void CSettingsView::on_buttonScalingAuto_toggled(bool checked)
{
	if (checked)
	{
		ui->spinBoxInterfaceScaling->hide();
	}
	else
	{
		ui->spinBoxInterfaceScaling->show();
		ui->spinBoxInterfaceScaling->setValue(100);
	}
	
	Settings node = settings.write["video"]["resolution"]["scaling"];
	node->Integer() = checked ? 0 : 100;
}

void CSettingsView::on_buttonHandleBackRightMouseButton_toggled(bool checked)
{
	Settings node = settings.write["input"]["handleBackRightMouseButton"];
	node->Bool() = checked;
	updateCheckbuttonText(ui->buttonHandleBackRightMouseButton);
}

