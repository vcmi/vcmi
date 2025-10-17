/*
 * firstlaunch_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "firstlaunch_moc.h"
#include "ui_firstlaunch_moc.h"

#include "mainwindow_moc.h"
#include "modManager/cmodlistview_moc.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../vcmiqt/MessageBox.h"
#include "../helper.h"
#include "../languages.h"
#include "../innoextract.h"

#ifdef VCMI_IOS
#include "iOS_utils.h"
#endif

#ifdef VCMI_ANDROID
#include <QtAndroid>
#endif

// Unified progress overlay
class ProgressOverlay final : public QWidget
{
public:
    explicit ProgressOverlay(QWidget *parent, int topOffsetPx = 50)
        : QWidget(parent)
    {
        // Match parent background to feel native
        setAutoFillBackground(true);
        QPalette pal = palette();
        pal.setColor(QPalette::Window, parent->palette().color(QPalette::Window));
        setPalette(pal);

        // Cover whole view except top offset
        setGeometry(parent->rect().adjusted(0, topOffsetPx, 0, 0));

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        titleLabel = new QLabel("", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("font-size: 16px; font-weight: 600;");

        fileLabel = new QLabel("", this);
        fileLabel->setAlignment(Qt::AlignCenter);
        fileLabel->setWordWrap(true);

        progressBar = new QProgressBar(this);
        progressBar->setMinimumHeight(18);
        setIndeterminate(true); // start indeterminate

        layout->addStretch();
        layout->addWidget(titleLabel);
        layout->addWidget(fileLabel);
        layout->addWidget(progressBar);
        layout->addStretch();

		Helper::keepScreenOn(true);
    }

    ~ProgressOverlay() override
    {
        Helper::keepScreenOn(false);
    }

    void setTitle(const QString &t)        { titleLabel->setText(t); }
    void setFileName(const QString &name)  { fileLabel->setText(name); }

    // Switch between indeterminate and determinate progress
    void setIndeterminate(bool on)         { on ? progressBar->setRange(0,0) : progressBar->setRange(0,100); }
    void setRange(int max)                 { progressBar->setRange(0, qMax(1, max)); progressBar->setValue(0); }
    void setValue(int v)                   { progressBar->setValue(v); }

private:
    QLabel *titleLabel = nullptr;
    QLabel *fileLabel = nullptr;
    QProgressBar *progressBar = nullptr;
};

// Create and show overlay immediately
static inline ProgressOverlay* createOverlay(QWidget *parent, const QString &title, bool indeterminate = true)
{
    auto *overlay = new ProgressOverlay(parent, 50);
    overlay->setTitle(title);
    overlay->setIndeterminate(indeterminate);
    overlay->show();
    qApp->processEvents(); // paint before heavy work
    return overlay;
}

FirstLaunchView::FirstLaunchView(QWidget * parent)
	: QWidget(parent)
	, ui(std::make_unique<Ui::FirstLaunchView>())
{
	ui->setupUi(this);

	enterSetup();
	activateTabLanguage();

	ui->lineEditDataSystem->setText(pathToQString(boost::filesystem::absolute(VCMIDirs::get().dataPaths().front())));
	ui->lineEditDataUser->setText(pathToQString(boost::filesystem::absolute(VCMIDirs::get().userDataPath())));

	Helper::enableScrollBySwiping(ui->listWidgetLanguage);

#ifdef VCMI_MOBILE
	// This directory is not accessible to players without rooting of their device
	ui->lineEditDataSystem->hide();
#endif

#ifndef ENABLE_INNOEXTRACT
	ui->pushButtonGogInstall->hide();
	ui->labelDataGogTitle->hide();
	ui->labelDataGogDescr->hide();
#endif
}

FirstLaunchView::~FirstLaunchView() = default;

void FirstLaunchView::on_buttonTabLanguage_clicked()
{
	activateTabLanguage();
}

void FirstLaunchView::on_buttonTabHeroesData_clicked()
{
	activateTabHeroesData();
}

void FirstLaunchView::on_buttonTabModPreset_clicked()
{
	activateTabModPreset();
}

void FirstLaunchView::on_listWidgetLanguage_currentRowChanged(int currentRow)
{
	languageSelected(ui->listWidgetLanguage->item(currentRow)->data(Qt::UserRole).toString());
}

void FirstLaunchView::changeEvent(QEvent * event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		Languages::fillLanguages(ui->listWidgetLanguage, false);
	}
	QWidget::changeEvent(event);
}

void FirstLaunchView::on_pushButtonLanguageNext_clicked()
{
	activateTabHeroesData();
}

void FirstLaunchView::on_pushButtonDataNext_clicked()
{
	activateTabModPreset();
}

void FirstLaunchView::on_pushButtonDataBack_clicked()
{
	activateTabLanguage();
}

void FirstLaunchView::on_pushButtonDataSearch_clicked()
{
	heroesDataUpdate();
}

void FirstLaunchView::on_pushButtonDataCopy_clicked()
{
    // iOS can't display modal dialogs when called directly on button press
    // https://bugreports.qt.io/browse/QTBUG-98651
    MessageBoxCustom::showDialog(this, [this]{
        Helper::nativeFolderPicker(this, [this](const QString &picked){
            if(!picked.isEmpty())
            	copyHeroesData(picked, false);
        });
    });
}

void FirstLaunchView::on_pushButtonGogInstall_clicked()
{
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	MessageBoxCustom::showDialog(this, [this]{extractGogData();});
}

void FirstLaunchView::enterSetup()
{
	Languages::fillLanguages(ui->listWidgetLanguage, false);
}

void FirstLaunchView::setSetupProgress(int progress)
{
	ui->buttonTabLanguage->setDisabled(progress < 1);
	ui->buttonTabHeroesData->setDisabled(progress < 2);
	ui->buttonTabModPreset->setDisabled(progress < 3);
}

void FirstLaunchView::activateTabLanguage()
{
	setSetupProgress(1);
	ui->installerTabs->setCurrentIndex(0);
	ui->buttonTabLanguage->setChecked(true);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabModPreset->setChecked(false);
}

void FirstLaunchView::activateTabHeroesData()
{
	setSetupProgress(2);
	ui->installerTabs->setCurrentIndex(1);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabHeroesData->setChecked(true);
	ui->buttonTabModPreset->setChecked(false);

	if(heroesDataUpdate())
	{
		activateTabModPreset();
		return;
	}

	QString installPath = getHeroesInstallDir();
	if(!installPath.isEmpty())
	{
		auto reply = QMessageBox::question(this, tr("Heroes III installation found!"), tr("Copy data to VCMI folder?"), QMessageBox::Yes | QMessageBox::No);
		if(reply == QMessageBox::Yes)
			copyHeroesData(installPath, false);
	}
}

void FirstLaunchView::activateTabModPreset()
{
	setSetupProgress(3);
	ui->installerTabs->setCurrentIndex(2);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabModPreset->setChecked(true);

	modPresetUpdate();
}

void FirstLaunchView::exitSetup(bool goToMods)
{
	if(auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow()))
		mainWindow->exitSetup(goToMods);
}

// Tab Language
void FirstLaunchView::languageSelected(const QString & selectedLanguage)
{
	Settings node = settings.write["general"]["language"];
	node->String() = selectedLanguage.toStdString();

	if(auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow()))
		mainWindow->updateTranslation();
}

bool FirstLaunchView::heroesDataUpdate()
{
	bool detected = heroesDataDetect();
	if(detected)
		heroesDataDetected();
	else
		heroesDataMissing();
	return detected;
}

void FirstLaunchView::heroesDataMissing()
{
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, QColor(200, 50, 50));
	ui->lineEditDataSystem->setPalette(newPalette);
	ui->lineEditDataUser->setPalette(newPalette);

	ui->labelDataManualTitle->setVisible(true);
	ui->labelDataManualDescr->setVisible(true);
	ui->pushButtonDataSearch->setVisible(true);

#ifdef VCMI_ANDROID
	// selecting directory with ACTION_OPEN_DOCUMENT_TREE is available only since API level 21
	const bool canUseDataCopy = QtAndroid::androidSdkVersion() >= 21;
#elif defined(VCMI_IOS)
	// selecting directory through UIDocumentPickerViewController is available only since iOS 13
	const bool canUseDataCopy = iOS_utils::isOsVersionAtLeast(13);
#else
	const bool canUseDataCopy = true;
#endif

	ui->labelDataCopyTitle->setVisible(canUseDataCopy);
	ui->labelDataCopyDescr->setVisible(canUseDataCopy);
	ui->pushButtonDataCopy->setVisible(canUseDataCopy);

#ifdef ENABLE_INNOEXTRACT
	ui->pushButtonGogInstall->setVisible(true);
	ui->labelDataGogTitle->setVisible(true);
	ui->labelDataGogDescr->setVisible(true);
#endif

	ui->labelDataFound->setVisible(false);
	ui->pushButtonDataNext->setEnabled(false);
}

void FirstLaunchView::heroesDataDetected()
{
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, QColor(50, 200, 50));
	ui->lineEditDataSystem->setPalette(newPalette);
	ui->lineEditDataUser->setPalette(newPalette);

	ui->pushButtonDataSearch->setVisible(false);
	ui->pushButtonDataCopy->setVisible(false);

	ui->labelDataManualTitle->setVisible(false);
	ui->labelDataManualDescr->setVisible(false);
	ui->labelDataCopyTitle->setVisible(false);
	ui->labelDataCopyDescr->setVisible(false);

#ifdef ENABLE_INNOEXTRACT
	ui->pushButtonGogInstall->setVisible(false);
	ui->labelDataGogTitle->setVisible(false);
	ui->labelDataGogDescr->setVisible(false);
#endif

	ui->labelDataFound->setVisible(true);
	ui->pushButtonDataNext->setEnabled(true);

	CGeneralTextHandler::detectInstallParameters();
}

// Tab Heroes III Data
bool FirstLaunchView::heroesDataDetect()
{
	// user might have copied files to one of our data path.
	// perform full reinitialization of virtual filesystem
	CResourceHandler::destroy();
	CResourceHandler::initialize();
	CResourceHandler::load("config/filesystem.json");

	// use file from lod archive to check presence of H3 data. Very rough estimate, but will work in majority of cases
	bool heroesDataFoundROE = CResourceHandler::get()->existsResource(ResourcePath("DATA/GENRLTXT.TXT"));
	bool heroesDataFoundSOD = CResourceHandler::get()->existsResource(ResourcePath("DATA/TENTCOLR.TXT"));

	return heroesDataFoundROE && heroesDataFoundSOD;
}

QString FirstLaunchView::getHeroesInstallDir()
{
#ifdef VCMI_WINDOWS
	QVector<QPair<QString, QString>> regKeys = {
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\GOG.com\\Games\\1207658787",                                            "path"    }, // Gog on x86 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1207658787",                               "path"    }, // Gog on x64 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\New World Computing\\Heroes of Might and Magic® III\\1.0",              "AppPath" }, // H3 Complete on x86 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\New World Computing\\Heroes of Might and Magic® III\\1.0", "AppPath" }, // H3 Complete on x64 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\New World Computing\\Heroes of Might and Magic III\\1.0",               "AppPath" }, // some localized H3 on x86 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\New World Computing\\Heroes of Might and Magic III\\1.0",  "AppPath" }, // some localized H3 on x64 system
	};

	for(auto & regKey : regKeys)
	{
		QString path = QSettings(regKey.first, QSettings::NativeFormat).value(regKey.second).toString();
		if(!path.isEmpty())
			return path;
	}
#endif
	return QString{};
}

void FirstLaunchView::extractGogData()
{
#ifdef ENABLE_INNOEXTRACT
	auto fileSelection = [this](QString filter, QString startPath = {}) {
		QString titleSel = tr("Select %1 file...", "param is file extension").arg(filter);
#if defined(VCMI_MOBILE)
		filter = tr("GOG file (*.*)");
		QMessageBox::information(this, tr("File selection"), titleSel);
#endif
		QString file = QFileDialog::getOpenFileName(this, titleSel, startPath.isEmpty() ? QDir::homePath() : startPath, filter);
		if(file.isEmpty())
			return QString{};
		return file;
	};

	QString filterBin = tr("GOG data") + " (*.bin)";
	QString filterExe = tr("GOG installer") + " (*.exe)";

	QString fileBin = fileSelection(filterBin);
	if(fileBin.isEmpty())
		return;

	QString fileExe = fileSelection(filterExe, QFileInfo(fileBin).absolutePath());
	if(fileExe.isEmpty())
		return;

	QTimer::singleShot(100, this, [this, fileBin, fileExe](){ // background to make sure FileDialog is closed...
		extractGogDataAsync(fileBin, fileExe);
		setEnabled(true);
		heroesDataUpdate();
	});
#endif
}

bool performCopyFlow(const QString &path, FirstLaunchView *self, ProgressOverlay *overlay, bool removeSource = false)
{
    // 1) Scan -> "src \t Target \t Name"
    overlay->setTitle(QObject::tr("Scanning selected folder..."));
    overlay->setIndeterminate(true);

    const QStringList items = Helper::findFilesForCopy(path);
    if(items.isEmpty())
	{
        overlay->deleteLater();
        QMessageBox::critical(self, QObject::tr("Heroes III data not found!"), QObject::tr("Failed to detect valid Heroes III data in chosen directory.\nPlease select the directory with installed Heroes III data."));
        return false;
    }

    // 2) Validate signature
	// TODO: Find proper way for pure SoD check in import or way to block pure RoE / AB
	//       Or prepare RoE / AB Ban mod and allow VCMI to with any H3 version
    auto validate = [](const QStringList &items)->QString {
        bool anyLOD=false;
		bool anySOD=false;
		bool anyHD=false;
		
        for(const QString &line : items)
		{
            const auto part = line.split('\t');
            if(part.size() < 3 || part[1].compare("Data", Qt::CaseInsensitive) != 0)
				continue;
			
            const QString &name = part[2];
            if(name.endsWith(".lod", Qt::CaseInsensitive))
			{
                anyLOD = true;
                if(name.startsWith("H3ab", Qt::CaseInsensitive))
                    anySOD = true;
            }
			
			if(name.endsWith(".pak", Qt::CaseInsensitive))
                anyHD = true;
        }
		
        if(anySOD) return {};

        if(!anyLOD)
            return QObject::tr("Failed to detect valid Heroes III data in chosen directory.\nPlease select the directory with installed Heroes III data.");

        if(anyHD)
            return QObject::tr("Heroes III: HD Edition files are not supported by VCMI.\nPlease select the directory with Heroes III: Complete Edition or Heroes III: Shadow of Death.");

        return QObject::tr("Unknown or unsupported Heroes III version found.\nPlease select the directory with Heroes III: Complete Edition or Heroes III: Shadow of Death.");
    };

    const QString err = validate(items);
    if(!err.isEmpty())
	{
        overlay->deleteLater();
        QMessageBox::critical(self, QObject::tr("Heroes III data not found!"), err);
        return false;
    }

    // 3) Plan destination, create target dirs on demand
    QDir targetRoot = pathToQString(VCMIDirs::get().userDataPath());
    QSet<QString> created;

    struct CopyItem { QString src, dst; };
    QVector<CopyItem> plan;
	plan.reserve(items.size());

    for(const QString &line : items)
	{
        const auto part = line.split('\t');
        if(part.size() < 3)
			continue;

        const QString &src  = part[0];
        const QString &tgt  = part[1]; // Data / Maps / Mp3
        const QString &file = part[2];

        if(tgt.compare("Data", Qt::CaseInsensitive)!=0 && tgt.compare("Maps", Qt::CaseInsensitive)!=0 && tgt.compare("Mp3",  Qt::CaseInsensitive)!=0)
            continue;

        if(!created.contains(tgt))
		{
            QDir{}.mkpath(targetRoot.filePath(tgt));
            created.insert(tgt);
        }

        const QDir dstDir = targetRoot.filePath(tgt);
        plan.push_back({ src, dstDir.filePath(file) });
    }

    if(plan.isEmpty())
	{
        overlay->deleteLater();
        QMessageBox::critical(self, QObject::tr("Heroes III data not found!"), QObject::tr("No matching files found in the selected folder."));
        return false;
    }

    // 4) Copy with progress
    overlay->setTitle(QObject::tr("Importing Heroes III data..."));
    overlay->setIndeterminate(false);
    overlay->setRange(plan.size());

    for(int i = 0; i < plan.size(); ++i)
	{
        overlay->setFileName(QFileInfo(plan[i].dst).fileName());
        overlay->setValue(i + 1);
        qApp->processEvents();

        if (QFile::exists(plan[i].dst))
            QFile::remove(plan[i].dst);

        Helper::performNativeCopy(plan[i].src, plan[i].dst);

        logGlobal->info("Copying '%s' -> '%s'", plan[i].src.toStdString(), plan[i].dst.toStdString());
    }

    // 5) Optional cleanup
    if(removeSource)
        QDir(path).removeRecursively();

    overlay->deleteLater();
    return true;
}

void FirstLaunchView::extractGogDataAsync(QString filePathBin, QString filePathExe)
{
    logGlobal->info("Extracting gog data from '%s' and '%s'", filePathBin.toStdString(), filePathExe.toStdString());

#ifdef ENABLE_INNOEXTRACT
    //  Show overlay immediately so the UI doesn't look frozen
    auto *overlay = createOverlay(this, tr("Checking installer..."), true);
    overlay->setFileName(QFileInfo(filePathExe).fileName());
    overlay->raise();
    qApp->processEvents();

    // Defer heavy work to next event-loop tick to ensure overlay is painted
    QTimer::singleShot(0, this, [this, overlay, filePathBin, filePathExe]() mutable
    {
        const QString filterBin = tr("GOG data") + " (*.bin)";
        const QString filterExe = tr("GOG installer") + " (*.exe)";

        // 1) Prepare temp dir
        QDir tempDir(pathToQString(VCMIDirs::get().userDataPath()));
        if(tempDir.cd("tmp"))
        {
            logGlobal->info("Cleaning up old temp data");
            tempDir.removeRecursively();
            tempDir.cdUp();
        }
        tempDir.mkdir("tmp");
        if(!tempDir.cd("tmp"))
        {
            overlay->deleteLater();
            return; // should not happen - safety bail out
        }

        const QString tmpFileExe = tempDir.filePath("h3_gog.exe");
        const QString tmpFileBin = tempDir.filePath("h3_gog-1.bin");

        // 2) Copy selected files into tmp
        logGlobal->info("Performing native copy...");
        Helper::performNativeCopy(filePathExe, tmpFileExe);
        Helper::performNativeCopy(filePathBin, tmpFileBin);
        logGlobal->info("Native copy completed");

        // 3) Sanity checks
        auto checkMagic = [](QString filename, QString filter, QByteArray magic)
        {
            logGlobal->info("Checking file %s", filename.toStdString());

            QFile tmpFile(filename);
            if(!tmpFile.open(QIODevice::ReadOnly))
            {
                logGlobal->info("File cannot be opened: %s", tmpFile.errorString().toStdString());
                return QObject::tr("Failed to open file: %1").arg(tmpFile.errorString());
            }

            QByteArray magicFile = tmpFile.read(magic.length());
            if(!magicFile.startsWith(magic))
            {
                logGlobal->info("Invalid file selected: %s", filter.toStdString());
                return QObject::tr("You have to select %1 file!", "param is file extension").arg(filter);
            }

            logGlobal->info("Checking file %s", filename.toStdString());
            return QString();
        };

        QString errorText{};

        if(errorText.isEmpty())
            errorText = checkMagic(tmpFileBin, filterBin, QByteArray{"idska32"});

        if(errorText.isEmpty())
            errorText = checkMagic(tmpFileExe, filterExe, QByteArray{"MZ"});

        logGlobal->info("Installing exe '%s' ('%s')", tmpFileExe.toStdString(), filePathExe.toStdString());
        logGlobal->info("Installing bin '%s' ('%s')", tmpFileBin.toStdString(), filePathBin.toStdString());

        auto isGogGalaxyExe = [](QString fileToTest)
        {
            QFile file(fileToTest);
            quint64 fileSize = file.size();

            if(fileSize > 10 * 1024 * 1024)
                return false; // avoid loading big files; Galaxy exe is smaller...

            if(!file.open(QIODevice::ReadOnly))
                return false;

            QByteArray data = file.readAll();
            const QByteArray magicId{reinterpret_cast<const char*>(u"GOG Galaxy"), 20};
            return data.contains(magicId);
        };

        if(errorText.isEmpty())
        {
            if(isGogGalaxyExe(tmpFileExe))
            {
                logGlobal->info("GOG Galaxy detected! Aborting...");
                errorText = tr("You've provided a GOG Galaxy installer! This file doesn't contain the game. Please download the offline backup game installer!");
            }
        }

        // Extract
        if(errorText.isEmpty())
        {
            overlay->setTitle(tr("Extracting installer..."));
            overlay->setIndeterminate(false);
            overlay->setRange(100);
            overlay->setValue(0);

            logGlobal->info("Performing extraction using innoextract...");

            errorText = Innoextract::extract(tmpFileExe, tempDir.path(), [overlay](float progress){
                overlay->setValue(int(progress * 100));
                qApp->processEvents();
            });

            logGlobal->info("Extraction done!");
        }

        // 5) Post-extract verification and error reporting
        QString hashError;
        if(!errorText.isEmpty())
            hashError = Innoextract::getHashError(tmpFileExe, tmpFileBin, filePathExe, filePathBin);

        QStringList dirData = tempDir.entryList({"data"}, QDir::Filter::Dirs);
        if(!errorText.isEmpty() || dirData.empty() || QDir(tempDir.filePath(dirData.front())).entryList({"*.lod"}, QDir::Filter::Files).empty())
        {
            overlay->deleteLater();

            if(!errorText.isEmpty())
            {
                logGlobal->error("Gog installer extraction failure! Reason: %s", errorText.toStdString());
                QMessageBox::critical(this, tr("Extracting error!"), errorText, QMessageBox::Ok, QMessageBox::Ok);
                if(!hashError.isEmpty())
                {
                    logGlobal->error("Hash error: %s", hashError.toStdString());
                    QMessageBox::critical(this, tr("Hash error!"), hashError, QMessageBox::Ok, QMessageBox::Ok);
                }
            }
            else
            {
                QMessageBox::critical(this, tr("No Heroes III data!"),  tr("Selected files do not contain Heroes III data!"), QMessageBox::Ok, QMessageBox::Ok);
            }
            tempDir.removeRecursively();
            return;
        }

        logGlobal->info("Copying provided game files...");

        // 6) Reuse overlay for copy phase
        overlay->setTitle(tr("Importing Heroes III data..."));
        overlay->setFileName({});
        overlay->setRange(100); // performCopyFlow will reset to plan size internally
        overlay->setValue(0);

        if(performCopyFlow(tempDir.path(), this, overlay, true))
        {
            if(heroesDataUpdate())
                activateTabModPreset();
        }
    });
#endif
}

void FirstLaunchView::copyHeroesData(const QString &path, bool removeSource)
{
    auto *overlay = createOverlay(this, tr("Scanning selected folder..."), true);

    auto work = [this, path, overlay]() {
        if(performCopyFlow(path, this, overlay, false))
            if(heroesDataUpdate())
                activateTabModPreset();
    };

    QTimer::singleShot(0, this, work);
}

// Tab Mod Preset
void FirstLaunchView::modPresetUpdate()
{
	bool translationExists = !findTranslationModName().isEmpty();

	ui->labelPresetLanguageDescr->setVisible(translationExists);
	ui->buttonPresetLanguage->setVisible(translationExists);

	ui->buttonPresetLanguage->setVisible(checkCanInstallTranslation());
	ui->buttonPresetExtras->setVisible(checkCanInstallExtras());
	ui->buttonPresetDemo->setVisible(checkCanInstallDemo());
	ui->buttonPresetHota->setVisible(checkCanInstallHota());
	ui->buttonPresetWog->setVisible(checkCanInstallWog());
	ui->buttonPresetTow->setVisible(checkCanInstallTow());
	ui->buttonPresetFod->setVisible(checkCanInstallFod());

	ui->labelPresetLanguageDescr->setVisible(checkCanInstallTranslation());
	ui->labelPresetExtrasDescr->setVisible(checkCanInstallExtras());
	ui->labelPresetDemoDescr->setVisible(checkCanInstallDemo());
	ui->labelPresetHotaDescr->setVisible(checkCanInstallHota());
	ui->labelPresetWogDescr->setVisible(checkCanInstallWog());
	ui->labelPresetTowDescr->setVisible(checkCanInstallTow());
	ui->labelPresetFodDescr->setVisible(checkCanInstallFod());

	// we can't install anything - either repository checkout is off or all recommended mods are already installed
	if (!checkCanInstallTranslation() && !checkCanInstallExtras() && !checkCanInstallDemo() && !checkCanInstallHota() && !checkCanInstallWog() && !checkCanInstallTow() && !checkCanInstallFod())
		exitSetup(false);
}

QString FirstLaunchView::findTranslationModName()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow());
	auto status = mainWindow->getTranslationStatus();

	if (status == ETranslationStatus::ACTIVE || status == ETranslationStatus::NOT_AVAILABLE)
		return QString();

	QString preferredlanguage = QString::fromStdString(settings["general"]["language"].String());
	return getModView()->getTranslationModName(preferredlanguage);
}

bool FirstLaunchView::checkCanInstallTranslation()
{
	QString modName = findTranslationModName();

	if(modName.isEmpty())
		return false;

	return checkCanInstallMod(modName);
}

bool FirstLaunchView::checkCanInstallExtras()
{
	return checkCanInstallMod("vcmi-extras");
}

bool FirstLaunchView::checkCanInstallDemo()
{
    if(!checkCanInstallMod("demo-support"))
        return false;

    QDir userRoot = pathToQString(VCMIDirs::get().userDataPath());
    QDir dataDir(userRoot.filePath(QStringLiteral("Data")));
	QDir mapsDir(userRoot.filePath(QStringLiteral("Maps")));

    bool hasDemoMap = false;
    QStringList mapFiles = mapsDir.entryList(QDir::Files | QDir::Readable);
    for(const QString &name : mapFiles)
        if(name.compare(QStringLiteral("h3demo.h3m"), Qt::CaseInsensitive) == 0)
            hasDemoMap = true;
	
    QStringList files = dataDir.entryList(QDir::Files | QDir::Readable);
    for(const QString &name : files)
    {
        if(name.compare(QStringLiteral("H3ab_spr.lod"), Qt::CaseInsensitive) == 0)
        {
            QFile lod(dataDir.filePath(name));
            quint64 fileSize = lod.size();
			logGlobal->error("H3ab_spr.lod size: %s", static_cast<unsigned long long>(fileSize));
            if(fileSize < 8000000 && hasDemoMap) // 8 MB + Demo map = Merged Windows and MacOS Demo
            	return true;
        }
    }
    return false;
}

bool FirstLaunchView::checkCanInstallHota()
{
	return checkCanInstallMod("hota");
}

bool FirstLaunchView::checkCanInstallWog()
{
	return checkCanInstallMod("wake-of-gods");
}

bool FirstLaunchView::checkCanInstallTow()
{
	return checkCanInstallMod("tides-of-war");
}

bool FirstLaunchView::checkCanInstallFod()
{
	return checkCanInstallMod("fallen-of-the-depth");
}

CModListView * FirstLaunchView::getModView()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow());

	assert(mainWindow);
	if (!mainWindow)
		return nullptr;

	return mainWindow->getModView();
}

bool FirstLaunchView::checkCanInstallMod(const QString & modID)
{
	return getModView() && getModView()->isModAvailable(modID);
}

void FirstLaunchView::on_pushButtonPresetBack_clicked()
{
	activateTabHeroesData();
}

void FirstLaunchView::on_pushButtonPresetNext_clicked()
{
	QStringList modsToInstall;

	if (ui->buttonPresetLanguage->isChecked() && checkCanInstallTranslation())
		modsToInstall.push_back(findTranslationModName());

	if (ui->buttonPresetExtras->isChecked() && checkCanInstallExtras())
		modsToInstall.push_back("vcmi-extras");

	if (ui->buttonPresetDemo->isChecked() && checkCanInstallDemo())
		modsToInstall.push_back("demo-support");
	
	if (ui->buttonPresetWog->isChecked() && checkCanInstallWog())
		modsToInstall.push_back("wake-of-gods");

	if (ui->buttonPresetHota->isChecked() && checkCanInstallHota())
		modsToInstall.push_back("hota");

	if (ui->buttonPresetTow->isChecked() && checkCanInstallTow())
		modsToInstall.push_back("tides-of-war");

	if (ui->buttonPresetFod->isChecked() && checkCanInstallFod())
		modsToInstall.push_back("fallen-of-the-depth");

	bool goToMods = !modsToInstall.empty();
	exitSetup(goToMods);

	for (auto const & modName : modsToInstall)
		getModView()->doInstallMod(modName);
}

void FirstLaunchView::on_pushButtonDiscord_clicked()
{
	QDesktopServices::openUrl(QUrl("https://discord.gg/chBT42V"));
}

void FirstLaunchView::on_pushButtonGithub_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi"));
}
