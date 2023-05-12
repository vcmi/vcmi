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
#include "../updatedialog_moc.h"

#include <QFileInfo>
#include <QGuiApplication>

#include "../../lib/CConfigHandler.h"
#include "../../lib/VCMIDirs.h"

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
	"auto",
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
	if (settings["video"]["realFullscreen"].Bool())
		ui->comboBoxFullScreen->setCurrentIndex(2);
	else
		ui->comboBoxFullScreen->setCurrentIndex(settings["video"]["fullscreen"].Bool());
#endif

	ui->comboBoxFriendlyAI->setCurrentText(QString::fromStdString(settings["server"]["friendlyAI"].String()));
	ui->comboBoxNeutralAI->setCurrentText(QString::fromStdString(settings["server"]["neutralAI"].String()));
	ui->comboBoxEnemyAI->setCurrentText(QString::fromStdString(settings["server"]["enemyAI"].String()));
	ui->comboBoxPlayerAI->setCurrentText(QString::fromStdString(settings["server"]["playerAI"].String()));

	ui->spinBoxNetworkPort->setValue(settings["server"]["port"].Integer());

	ui->comboBoxAutoCheck->setCurrentIndex(settings["launcher"]["autoCheckRepositories"].Bool());

	JsonNode urls = settings["launcher"]["repositoryURL"];
	ui->plainTextEditRepos->blockSignals(true); // Do not report loading as change of data
	ui->plainTextEditRepos->clear();
	for(auto entry : urls.Vector())
		ui->plainTextEditRepos->appendPlainText(QString::fromUtf8(entry.String().c_str()));
	ui->plainTextEditRepos->blockSignals(false);

	ui->lineEditUserDataDir->setText(pathToQString(VCMIDirs::get().userDataPath()));
	ui->lineEditGameDir->setText(pathToQString(VCMIDirs::get().binaryPath()));
	ui->lineEditTempDir->setText(pathToQString(VCMIDirs::get().userLogsPath()));

	ui->comboBoxAutoSave->setCurrentIndex(settings["general"]["saveFrequency"].Integer() > 0 ? 1 : 0);

	Languages::fillLanguages(ui->comboBoxLanguage, false);

	std::string cursorType = settings["video"]["cursor"].String();
	size_t cursorTypeIndex = boost::range::find(cursorTypesList, cursorType) - cursorTypesList;
	ui->comboBoxCursorType->setCurrentIndex((int)cursorTypeIndex);
}

void CSettingsView::fillValidResolutions()
{
	fillValidResolutionsForScreen(ui->comboBoxDisplayIndex->isVisible() ? ui->comboBoxDisplayIndex->currentIndex() : 0);
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

	QVector<QSize> resolutions = findAvailableResolutions(screenIndex);

	for(const auto & entry : resolutions)
		ui->comboBoxResolution->addItem(resolutionToString(entry));

	int resX = settings["video"]["resolution"]["width"].Integer();
	int resY = settings["video"]["resolution"]["height"].Integer();
	int resIndex = ui->comboBoxResolution->findText(resolutionToString({resX, resY}));
	ui->comboBoxResolution->setCurrentIndex(resIndex);

	ui->comboBoxResolution->blockSignals(false);

	// if selected resolution no longer exists, force update value to the largest (last) resolution
	if(resIndex == -1)
		ui->comboBoxResolution->setCurrentIndex(ui->comboBoxResolution->count() - 1);
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

	Settings node = settings.write["video"]["resolution"];
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
