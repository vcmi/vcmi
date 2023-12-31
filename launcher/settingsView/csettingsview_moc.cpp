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
#include "../jsonutils.h"
#include "../languages.h"
#include "../launcherdirs.h"

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

static const std::string cursorTypesList[] =
{
	"hardware",
	"software"
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
	ui->spinBoxReservedArea->hide();
	if (settings["video"]["realFullscreen"].Bool())
		ui->comboBoxFullScreen->setCurrentIndex(2);
	else
		ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
#endif
	fillValidScalingRange();

	ui->spinBoxInterfaceScaling->setValue(settings["video"]["resolution"]["scaling"].Float());
	ui->spinBoxFramerateLimit->setValue(settings["video"]["targetfps"].Float());
	ui->spinBoxFramerateLimit->setDisabled(settings["video"]["vsync"].Bool());
	ui->checkBoxVSync->setChecked(settings["video"]["vsync"].Bool());
	ui->spinBoxReservedArea->setValue(std::round(settings["video"]["reservedWidth"].Float() * 100));

	ui->comboBoxFriendlyAI->setCurrentText(QString::fromStdString(settings["server"]["friendlyAI"].String()));
	ui->comboBoxNeutralAI->setCurrentText(QString::fromStdString(settings["server"]["neutralAI"].String()));
	ui->comboBoxEnemyAI->setCurrentText(QString::fromStdString(settings["server"]["enemyAI"].String()));

	ui->comboBoxEnemyPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["playerAI"].String()));
	ui->comboBoxAlliedPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["alliedAI"].String()));

	ui->spinBoxNetworkPort->setValue(settings["server"]["port"].Integer());

	ui->comboBoxAutoCheck->setCurrentIndex(settings["launcher"]["autoCheckRepositories"].Bool());

	ui->lineEditRepositoryDefault->setText(QString::fromStdString(settings["launcher"]["defaultRepositoryURL"].String()));
	ui->lineEditRepositoryExtra->setText(QString::fromStdString(settings["launcher"]["extraRepositoryURL"].String()));

	ui->lineEditRepositoryDefault->setEnabled(settings["launcher"]["defaultRepositoryEnabled"].Bool());
	ui->lineEditRepositoryExtra->setEnabled(settings["launcher"]["extraRepositoryEnabled"].Bool());

	ui->checkBoxRepositoryDefault->setChecked(settings["launcher"]["defaultRepositoryEnabled"].Bool());
	ui->checkBoxRepositoryExtra->setChecked(settings["launcher"]["extraRepositoryEnabled"].Bool());

	ui->comboBoxAutoSave->setCurrentIndex(settings["general"]["saveFrequency"].Integer() > 0 ? 1 : 0);

    ui->spinBoxAutoSaveLimit->setValue(settings["general"]["autosaveCountLimit"].Integer());

    ui->checkBoxAutoSavePrefix->setChecked(settings["general"]["useSavePrefix"].Bool());

    ui->lineEditAutoSavePrefix->setText(QString::fromStdString(settings["general"]["savePrefix"].String()));
    ui->lineEditAutoSavePrefix->setEnabled(settings["general"]["useSavePrefix"].Bool());

	Languages::fillLanguages(ui->comboBoxLanguage, false);

	std::string cursorType = settings["video"]["cursor"].String();
	size_t cursorTypeIndex = boost::range::find(cursorTypesList, cursorType) - cursorTypesList;
	ui->comboBoxCursorType->setCurrentIndex((int)cursorTypeIndex);
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
	ui->comboBoxResolution->blockSignals(true); // avoid saving wrong resolution after adding first item from the list
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

	ui->comboBoxResolution->blockSignals(false);
}
#else
void CSettingsView::fillValidResolutionsForScreen(int screenIndex)
{
	// resolutions are not selectable on mobile platforms
	ui->comboBoxResolution->hide();
	ui->labelResolution->hide();
}
#endif

CSettingsView::CSettingsView(QWidget * parent)
	: QWidget(parent), ui(new Ui::CSettingsView)
{
	ui->setupUi(this);

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

void CSettingsView::on_listWidgetSettings_currentRowChanged(int currentRow)
{
	QVector<QWidget*> targetWidgets = {
		ui->labelGeneral,
		ui->labelVideo,
		ui->labelArtificialIntelligence,
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

void CSettingsView::loadTranslation()
{
	Languages::fillLanguages(ui->comboBoxLanguageBase, true);

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

void CSettingsView::on_comboBoxLanguageBase_currentIndexChanged(int index)
{
	Settings node = settings.write["general"]["gameDataLanguage"];
	QString selectedLanguage = ui->comboBoxLanguageBase->itemData(index).toString();
	node->String() = selectedLanguage.toStdString();
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

void CSettingsView::on_lineEditAutoSavePrefix_textEdited(const QString &arg1)
{
    Settings node = settings.write["general"]["savePrefix"];
    node->String() = arg1.toStdString();
}

void CSettingsView::on_spinBoxReservedArea_valueChanged(int arg1)
{
	Settings node = settings.write["video"]["reservedWidth"];
	node->Float() = float(arg1) / 100; // percentage -> ratio
}

