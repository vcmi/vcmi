/*
 * cmodlistview_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "cmodlistview_moc.h"
#include "ui_cmodlistview_moc.h"
#include "imageviewer_moc.h"
#include "../mainwindow_moc.h"

#include <QJsonArray>
#include <QCryptographicHash>
#include <QRegularExpression>

#include "modstatemodel.h"
#include "modstateitemmodel_moc.h"
#include "modstatecontroller.h"
#include "cdownloadmanager_moc.h"
#include "chroniclesextractor.h"
#include "../settingsView/csettingsview_moc.h"
#include "../vcmiqt/launcherdirs.h"
#include "../vcmiqt/jsonutils.h"
#include "../helper.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/modding/CModVersion.h"
#include "../../lib/filesystem/Filesystem.h"

#include <future>

void CModListView::setupModModel()
{
	static const QString repositoryCachePath = CLauncherDirs::downloadsPath() + "/repositoryCache.json";
	const auto &cachedRepositoryData = JsonUtils::jsonFromFile(repositoryCachePath);

	modStateModel = std::make_shared<ModStateModel>();
	if (!cachedRepositoryData.isNull())
		modStateModel->appendRepositories(cachedRepositoryData);

	modModel = new ModStateItemModel(modStateModel, this);
	manager = std::make_unique<ModStateController>(modStateModel);
}

void CModListView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		modModel->reloadRepositories();
	}
	QWidget::changeEvent(event);
}

void CModListView::dragEnterEvent(QDragEnterEvent* event)
{
	if(event->mimeData()->hasUrls())
		for(const auto & url : event->mimeData()->urls())
			for(const auto & ending : QStringList({".zip", ".h3m", ".h3c", ".vmap", ".vcmp", ".json", ".exe"}))
				if(url.fileName().endsWith(ending, Qt::CaseInsensitive))
				{
					event->acceptProposedAction();
					return;
				}
}

void CModListView::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();

	if(mimeData->hasUrls())
	{
		const QList<QUrl> urlList = mimeData->urls();
		for (const auto & url : urlList)
			manualInstallFile(url.toLocalFile());
	}
}

void CModListView::setupFilterModel()
{
	filterModel = new CModFilterModel(modModel, this);

	filterModel->setFilterKeyColumn(-1); // filter across all columns
	filterModel->setSortCaseSensitivity(Qt::CaseInsensitive); // to make it more user-friendly
	filterModel->setDynamicSortFilter(true);
}

void CModListView::setupModsView()
{
	ui->allModsView->setModel(filterModel);
	// input data is not sorted - sort it before display
	ui->allModsView->sortByColumn(ModFields::TYPE, Qt::AscendingOrder);

	ui->allModsView->header()->setSectionResizeMode(ModFields::STATUS_ENABLED, QHeaderView::Fixed);
	ui->allModsView->header()->setSectionResizeMode(ModFields::STATUS_UPDATE, QHeaderView::Fixed);

	QSettings s(Ui::teamName, Ui::appName);
	auto state = s.value("AllModsView/State").toByteArray();
	if(!state.isNull()) //read last saved settings
	{
		ui->allModsView->header()->restoreState(state);
	}
	else //default //TODO: default high-DPI scaling
	{
		ui->allModsView->setColumnWidth(ModFields::NAME, 220);
		ui->allModsView->setColumnWidth(ModFields::TYPE, 75);
	}

	ui->allModsView->resizeColumnToContents(ModFields::STATUS_ENABLED);
	ui->allModsView->resizeColumnToContents(ModFields::STATUS_UPDATE);

	ui->allModsView->setUniformRowHeights(true);

	connect(ui->allModsView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&,const QModelIndex&)),
		this, SLOT(modSelected(const QModelIndex&,const QModelIndex&)));

	connect(filterModel, SIGNAL(modelReset()),
		this, SLOT(modelReset()));

	connect(modModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
		this, SLOT(dataChanged(QModelIndex,QModelIndex)));
}

CModListView::CModListView(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::CModListView)
{
	ui->setupUi(this);

	setAcceptDrops(true);

	ui->uninstallButton->setIcon(QIcon{":/icons/mod-delete.png"});
	ui->enableButton->setIcon(QIcon{":/icons/mod-enabled.png"});
	ui->disableButton->setIcon(QIcon{":/icons/mod-disabled.png"});
	ui->updateButton->setIcon(QIcon{":/icons/mod-update.png"});
	ui->installButton->setIcon(QIcon{":/icons/mod-download.png"});

	ui->splitter->setStyleSheet("QSplitter::handle {background: palette('window');}");

	setupModModel();
	setupFilterModel();
	setupModsView();

	ui->progressWidget->setVisible(false);
	dlManager = nullptr;

	if(settings["launcher"]["autoCheckRepositories"].Bool())
		loadRepositories();

#ifdef VCMI_MOBILE
	for(auto * scrollWidget : {
		(QAbstractItemView*)ui->allModsView,
		(QAbstractItemView*)ui->screenshotsList})
	{
		Helper::enableScrollBySwiping(scrollWidget);

		scrollWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
		scrollWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
#endif
}

void CModListView::loadRepositories()
{
	QStringList repositories;

	if (settings["launcher"]["defaultRepositoryEnabled"].Bool())
		repositories.push_back(QString::fromStdString(settings["launcher"]["defaultRepositoryURL"].String()));

	if (settings["launcher"]["extraRepositoryEnabled"].Bool())
		repositories.push_back(QString::fromStdString(settings["launcher"]["extraRepositoryURL"].String()));

	for(const auto & entry : repositories)
	{
		if (entry.isEmpty())
			continue;

		// URL must be encoded to something else to get rid of symbols illegal in file names
		auto hashed = QCryptographicHash::hash(entry.toUtf8(), QCryptographicHash::Md5);
		auto hashedStr = QString::fromUtf8(hashed.toHex());

		downloadFile(hashedStr + ".json", entry, tr("mods repository index"));
	}
}

CModListView::~CModListView()
{
	QSettings s(Ui::teamName, Ui::appName);
	s.setValue("AllModsView/State", ui->allModsView->header()->saveState());

	delete ui;
}

static QString replaceIfNotEmpty(QVariant value, QString pattern)
{
	if(value.canConvert<QString>())
	{
		if (value.toString().isEmpty())
			return "";
		else
			return pattern.arg(value.toString());
	}

	if(value.canConvert<QStringList>())
	{
		if (value.toStringList().isEmpty())
			return "";
		else
			return pattern.arg(value.toStringList().join(", "));
	}

	// all valid types of data should have been filtered by code above
	assert(!value.isValid());

	return "";
}

static QString replaceIfNotEmpty(QStringList value, QString pattern)
{
	if(!value.empty())
		return pattern.arg(value.join(", "));
	return "";
}

QString CModListView::genChangelogText(const ModState & mod)
{
	QString headerTemplate = "<p><span style=\" font-weight:600;\">%1: </span></p>";
	QString entryBegin = "<p align=\"justify\"><ul>";
	QString entryEnd = "</ul></p>";
	QString entryLine = "<li>%1</li>";
	//QString versionSeparator = "<hr/>";

	QString result;

	QMap<QString, QStringList> changelog = mod.getChangelog();
	QList<QString> versions = changelog.keys();

	std::sort(versions.begin(), versions.end(), [](QString lesser, QString greater)
	{
		return CModVersion::fromString(lesser.toStdString()) < CModVersion::fromString(greater.toStdString());
	});
	std::reverse(versions.begin(), versions.end());

	for(const auto & version : versions)
	{
		result += headerTemplate.arg(version);
		result += entryBegin;
		for(const auto & line : changelog.value(version))
			result += entryLine.arg(line);
		result += entryEnd;
	}
	return result;
}

QStringList CModListView::getModNames(QString queryingModID, QStringList input)
{
	QStringList result;

	auto queryingMod = modStateModel->getMod(queryingModID);

	for(const auto & modID : input)
	{
		if (modStateModel->isModExists(modID) && modStateModel->getMod(modID).isHidden())
			continue;

		QString parentModID = modStateModel->getTopParent(modID);
		QString displayName;

		if (modStateModel->isSubmod(modID) && queryingMod.getParentID() != parentModID )
		{
			// show in form "parent mod (submod)"

			QString parentDisplayName = parentModID;
			QString submodDisplayName = modID;

			if (modStateModel->isModExists(parentModID))
				parentDisplayName = modStateModel->getMod(parentModID).getName();

			if (modStateModel->isModExists(modID))
				submodDisplayName = modStateModel->getMod(modID).getName();

			displayName = QString("%1 (%2)").arg(submodDisplayName, parentDisplayName);
		}
		else
		{
			// show simply as mod name
			displayName = modID;
			if (modStateModel->isModExists(modID))
				displayName = modStateModel->getMod(modID).getName();
		}
		result += displayName;
	}
	return result;
}

QString CModListView::genModInfoText(const ModState & mod)
{
	QString prefix = "<p><span style=\" font-weight:600;\">%1: </span>"; // shared prefix
	QString redPrefix = "<p><span style=\" font-weight:600; color:red\">%1: </span>"; // shared prefix
	QString lineTemplate = prefix + "%2</p>";
	QString urlTemplate = prefix + "<a href=\"%2\">%3</a></p>";
	QString textTemplate = prefix + "</p><p align=\"justify\">%2</p>";
	QString listTemplate = "<p align=\"justify\">%1: %2</p>";
	QString noteTemplate = "<p align=\"justify\">%1</p>";
	QString incompatibleString = redPrefix + tr("Mod is incompatible") + "</p>";
	QString supportedVersions = redPrefix + "%2 %3 %4</p>";

	QString result;

	result += replaceIfNotEmpty(mod.getName(), lineTemplate.arg(tr("Mod name")));
	if (mod.isUpdateAvailable())
	{
		result += replaceIfNotEmpty(mod.getInstalledVersion(), lineTemplate.arg(tr("Installed version")));
		result += replaceIfNotEmpty(mod.getRepositoryVersion(), lineTemplate.arg(tr("Latest version")));
	}
	else
	{
		if (mod.isInstalled())
			result += replaceIfNotEmpty(mod.getInstalledVersion(), lineTemplate.arg(tr("Installed version")));
		else
			result += replaceIfNotEmpty(mod.getRepositoryVersion(), lineTemplate.arg(tr("Latest version")));
	}

	if (mod.isInstalled())
		result += replaceIfNotEmpty(modStateModel->getInstalledModSizeFormatted(mod.getID()), lineTemplate.arg(tr("Size")));

	if((!mod.isInstalled() || mod.isUpdateAvailable()) && !mod.getDownloadSizeFormatted().isEmpty())
		result += replaceIfNotEmpty(mod.getDownloadSizeFormatted(), lineTemplate.arg(tr("Download size")));
	
	result += replaceIfNotEmpty(mod.getAuthors(), lineTemplate.arg(tr("Authors")));

	if(!mod.getLicenseName().isEmpty())
		result += urlTemplate.arg(tr("License")).arg(mod.getLicenseUrl()).arg(mod.getLicenseName());

	if(!mod.getContact().isEmpty())
		result += urlTemplate.arg(tr("Contact")).arg(mod.getContact()).arg(mod.getContact());

	//compatibility info
	if(!mod.isCompatible())
	{
		auto compatibilityInfo = mod.getCompatibleVersionRange();
		auto minStr = compatibilityInfo.first;
		auto maxStr = compatibilityInfo.second;

		result += incompatibleString.arg(tr("Compatibility"));
		if(minStr == maxStr)
			result += supportedVersions.arg(tr("Required VCMI version"), minStr, "", "");
		else
		{
			if(minStr.isEmpty() || maxStr.isEmpty())
			{
				if(minStr.isEmpty())
					result += supportedVersions.arg(tr("Supported VCMI version"), maxStr, ", ", tr("please upgrade mod"));
				else
					result += supportedVersions.arg(tr("Required VCMI version"), minStr, " ", tr("or newer"));
			}
			else
				result += supportedVersions.arg(tr("Supported VCMI versions"), minStr, " - ", maxStr);
		}
	}

	QVariant baseLanguageVariant = mod.getBaseLanguage();
	QString baseLanguageID = baseLanguageVariant.isValid() ? baseLanguageVariant.toString() : "english";

	QStringList supportedLanguages = mod.getSupportedLanguages();

	if(supportedLanguages.size() > 1)
	{
		QStringList supportedLanguagesTranslated;

		for (const auto & languageID : supportedLanguages)
			supportedLanguagesTranslated += QApplication::translate("Language", Languages::getLanguageOptions(languageID.toStdString()).nameEnglish.c_str());

		result += replaceIfNotEmpty(supportedLanguagesTranslated, lineTemplate.arg(tr("Languages")));
	}

	QStringList conflicts = mod.getConflicts();
	for (const auto & otherMod : modStateModel->getAllMods())
	{
		QStringList otherConflicts = modStateModel->getMod(otherMod).getConflicts();

		if (otherConflicts.contains(mod.getID()) && !conflicts.contains(otherMod))
			conflicts.push_back(otherMod);
	}

	result += replaceIfNotEmpty(getModNames(mod.getID(), mod.getDependencies()), lineTemplate.arg(tr("Required mods")));
	result += replaceIfNotEmpty(getModNames(mod.getID(), conflicts), lineTemplate.arg(tr("Conflicting mods")));
	result += replaceIfNotEmpty(mod.getDescription(), textTemplate.arg(tr("Description")));

	result += "<p></p>"; // to get some empty space

	QString unknownDeps = tr("This mod can not be installed or enabled because the following dependencies are not present");
	QString thisIsSubmod = tr("This is a submod and it cannot be installed or uninstalled separately from its parent mod");

	QString notes;

	notes += replaceIfNotEmpty(getModNames(mod.getID(), findInvalidDependencies(mod.getID())), listTemplate.arg(unknownDeps));

	if(mod.isSubmod())
		notes += noteTemplate.arg(thisIsSubmod);

	if(notes.size())
		result += textTemplate.arg(tr("Notes")).arg(notes);

	return result;
}

void CModListView::disableModInfo()
{
	ui->disableButton->setVisible(false);
	ui->enableButton->setVisible(false);
	ui->installButton->setVisible(false);
	ui->uninstallButton->setVisible(false);
	ui->updateButton->setVisible(false);
}

void CModListView::dataChanged(const QModelIndex & topleft, const QModelIndex & bottomRight)
{
	selectMod(ui->allModsView->currentIndex());
}

void CModListView::selectMod(const QModelIndex & index)
{
	ui->tabWidget->setCurrentIndex(0);

	if(!index.isValid())
	{
		disableModInfo();
	}
	else
	{
		const auto modName = index.data(ModRoles::ModNameRole).toString();
		auto mod = modStateModel->getMod(modName);

		ui->tabWidget->setTabEnabled(1, !mod.getChangelog().isEmpty());
		ui->tabWidget->setTabEnabled(2, !mod.getScreenshots().isEmpty());

		ui->modInfoBrowser->setHtml(genModInfoText(mod));
		ui->changelogBrowser->setHtml(genChangelogText(mod));

		Helper::enableScrollBySwiping(ui->modInfoBrowser);
		Helper::enableScrollBySwiping(ui->changelogBrowser);

		//FIXME: this function should be recursive
		//FIXME: ensure that this is also reflected correctly in "Notes" section of mod description
		bool hasInvalidDeps = !findInvalidDependencies(modName).empty();

		ui->disableButton->setVisible(modStateModel->isModInstalled(mod.getID()) && modStateModel->isModEnabled(mod.getID()));
		ui->enableButton->setVisible(modStateModel->isModInstalled(mod.getID()) && !modStateModel->isModEnabled(mod.getID()));
		ui->installButton->setVisible(mod.isAvailable() && !mod.isSubmod());
		ui->uninstallButton->setVisible(mod.isInstalled() && !mod.isSubmod());
		ui->updateButton->setVisible(mod.isUpdateAvailable());

		// Block buttons if action is not allowed at this time
		ui->disableButton->setEnabled(true);
		ui->enableButton->setEnabled(!hasInvalidDeps);
		ui->installButton->setEnabled(!hasInvalidDeps);
		ui->uninstallButton->setEnabled(true);
		ui->updateButton->setEnabled(!hasInvalidDeps);

		loadScreenshots();
	}
}

void CModListView::modSelected(const QModelIndex & current, const QModelIndex &)
{
	selectMod(current);
}

void CModListView::on_allModsView_activated(const QModelIndex & index)
{
	selectMod(index);
	loadScreenshots();
}

void CModListView::on_lineEdit_textChanged(const QString & arg1)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	auto baseStr = QRegularExpression::wildcardToRegularExpression(arg1, QRegularExpression::UnanchoredWildcardConversion);
#else
	auto baseStr = QRegularExpression::wildcardToRegularExpression(arg1);
	//Hack due to lack QRegularExpression::UnanchoredWildcardConversion in Qt5
	baseStr.chop(3);
	baseStr.remove(0,5);
#endif
	QRegularExpression regExp{baseStr, QRegularExpression::CaseInsensitiveOption};
	filterModel->setFilterRegularExpression(regExp);
}

void CModListView::on_comboBox_currentIndexChanged(int index)
{
	auto enumIndex = static_cast<ModFilterMask>(index);
	filterModel->setTypeFilter(enumIndex);
}

QStringList CModListView::findInvalidDependencies(QString mod)
{
	QStringList ret;
	for(QString requirement : modStateModel->getMod(mod).getDependencies())
	{
		if(modStateModel->isModExists(requirement))
			continue;

		if(modStateModel->isSubmod(requirement))
		{
			QString parentModID = modStateModel->getTopParent(requirement);

			if (modStateModel->isModExists(parentModID) && !modStateModel->isModInstalled(parentModID))
				continue;
		}

		ret += requirement;
	}
	return ret;
}

void CModListView::on_enableButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();
	
	enableModByName(modName);
	
	checkManagerErrors();
}

void CModListView::enableModByName(QString modName)
{
	manager->enableMods({modName});
	modModel->reloadRepositories();
}

void CModListView::on_disableButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	disableModByName(modName);
	
	checkManagerErrors();
}

void CModListView::disableModByName(QString modName)
{
	manager->disableMod(modName);
	modModel->reloadRepositories();
}

QStringList CModListView::getModsToInstall(QString mod)
{
	QStringList result;
	QStringList candidates;
	QStringList processed;

	candidates.push_back(mod);
	while (!candidates.empty())
	{
		QString potentialToInstall = candidates.back();
		candidates.pop_back();
		processed.push_back(potentialToInstall);

		if (modStateModel->isSubmod(potentialToInstall))
			potentialToInstall = modStateModel->getTopParent(potentialToInstall);

		if (!modStateModel->isModExists(potentialToInstall))
			throw std::runtime_error("Attempt to install non-existing mod! Mod name:" + potentialToInstall.toStdString());

		if (modStateModel->isModInstalled(potentialToInstall))
			continue;

		result.push_back(potentialToInstall);

		QStringList dependencies = modStateModel->getMod(potentialToInstall).getDependencies();
		for (const auto & dependency : dependencies)
		{
			if (!processed.contains(dependency) && !candidates.contains(dependency))
				candidates.push_back(dependency);
		}
	}
	assert(result.removeDuplicates() == 0);
	return result;
}

void CModListView::on_updateButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	assert(findInvalidDependencies(modName).empty());

	for(const auto & name : getModsToInstall(modName))
	{
		auto mod = modStateModel->getMod(name);
		// update required mod, install missing (can be new dependency)
		if(mod.isUpdateAvailable() || !mod.isInstalled())
			downloadFile(name + ".zip", mod.getDownloadUrl(), name, mod.getDownloadSizeMegabytes());
	}
}

void CModListView::on_uninstallButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	if(modStateModel->isModExists(modName) && modStateModel->getMod(modName).isInstalled())
	{
		if(modStateModel->isModEnabled(modName))
			manager->disableMod(modName);
		manager->uninstallMod(modName);
		modModel->reloadRepositories();
	}
	
	checkManagerErrors();
}

void CModListView::on_installButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	assert(findInvalidDependencies(modName).empty());

	for(const auto & name : getModsToInstall(modName))
	{
		auto mod = modStateModel->getMod(name);
		if(mod.isAvailable())
			downloadFile(name + ".zip", mod.getDownloadUrl(), name, mod.getDownloadSizeMegabytes());
		else if(!modStateModel->isModEnabled(name))
			enableModByName(name);
	}
}

void CModListView::on_installFromFileButton_clicked()
{
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	QTimer::singleShot(0, this, [this]
	{
		QString filter = tr("All supported files") + " (*.h3m *.vmap *.h3c *.vcmp *.zip *.json *.exe);;" + 
			tr("Maps") + " (*.h3m *.vmap);;" + 
			tr("Campaigns") + " (*.h3c *.vcmp);;" + 
			tr("Configs") + " (*.json);;" + 
			tr("Mods") + " (*.zip);;" + 
			tr("Gog files") + " (*.exe)";
#if defined(VCMI_MOBILE)
		filter = tr("All files (*.*)"); //Workaround for sometimes incorrect mime for some extensions (e.g. for exe)
#endif
		QStringList files = QFileDialog::getOpenFileNames(this, tr("Select files (configs, mods, maps, campaigns, gog files) to install..."), QDir::homePath(), filter);

		for(const auto & file : files)
		{
			manualInstallFile(file);
		}
	});
}

void CModListView::manualInstallFile(QString filePath)
{
	QString fileName = QFileInfo{filePath}.fileName();
	if(filePath.endsWith(".zip", Qt::CaseInsensitive))
		downloadFile(fileName.toLower()
			// mod name currently comes from zip file -> remove suffixes from github zip download
			.replace(QRegularExpression("-[0-9a-f]{40}"), "")
			.replace(QRegularExpression("-vcmi-.+\\.zip"), ".zip")
			.replace("-main.zip", ".zip")
			, QUrl::fromLocalFile(filePath), "mods");
	else if(filePath.endsWith(".json", Qt::CaseInsensitive))
	{
		QDir configDir(QString::fromStdString(VCMIDirs::get().userConfigPath().string()));
		QStringList configFile = configDir.entryList({fileName}, QDir::Filter::Files); // case insensitive check
		if(!configFile.empty())
		{
			auto dialogResult = QMessageBox::warning(this, tr("Replace config file?"), tr("Do you want to replace %1?").arg(configFile[0]), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if(dialogResult == QMessageBox::Yes)
			{
				const auto configFilePath = configDir.filePath(configFile[0]);
				QFile::remove(configFilePath);
				QFile::copy(filePath, configFilePath);

				// reload settings
				Helper::loadSettings();
				for(const auto widget : qApp->allWidgets())
					if(auto settingsView = qobject_cast<CSettingsView *>(widget))
						settingsView->loadSettings();

				modStateModel->reloadLocalState();
				modModel->reloadRepositories();
			}
		}
	}
	else
		downloadFile(fileName, QUrl::fromLocalFile(filePath), fileName);
}

void CModListView::downloadFile(QString file, QString url, QString description, qint64 size)
{
	downloadFile(file, QUrl{url}, description, size);
}

void CModListView::downloadFile(QString file, QUrl url, QString description, qint64 size)
{
	if(!dlManager)
	{
		dlManager = new CDownloadManager();
		ui->progressWidget->setVisible(true);
		connect(dlManager, SIGNAL(downloadProgress(qint64,qint64)),
			this, SLOT(downloadProgress(qint64,qint64)));

		connect(dlManager, SIGNAL(finished(QStringList,QStringList,QStringList)),
			this, SLOT(downloadFinished(QStringList,QStringList,QStringList)));

		connect(manager.get(), SIGNAL(extractionProgress(qint64,qint64)),
			this, SLOT(extractionProgress(qint64,qint64)));

		connect(modModel, &ModStateItemModel::dataChanged, filterModel, &QAbstractItemModel::dataChanged);

		const auto progressBarFormat = tr("Downloading %1. %p% (%v MB out of %m MB) finished").arg(description);
		ui->progressBar->setFormat(progressBarFormat);
	}

	dlManager->downloadFile(url, file, size);
}

void CModListView::downloadProgress(qint64 current, qint64 max)
{
	// display progress, in megabytes
	ui->progressBar->setVisible(true);
	ui->progressBar->setMaximum(max / (1024 * 1024));
	ui->progressBar->setValue(current / (1024 * 1024));
}

void CModListView::extractionProgress(qint64 current, qint64 max)
{
	// display progress, in extracted files
	ui->progressBar->setVisible(true);
	ui->progressBar->setMaximum(max);
	ui->progressBar->setValue(current);
}

void CModListView::downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors)
{
	QString title = tr("Download failed");
	QString firstLine = tr("Unable to download all files.\n\nEncountered errors:\n\n");
	QString lastLine = tr("\n\nInstall successfully downloaded?");
	bool doInstallFiles = false;

	// if all files were d/loaded there should be no errors. And on failure there must be an error
	assert(failedFiles.empty() == errors.empty());

	if(savedFiles.empty())
	{
		// no successfully downloaded mods
		QMessageBox::warning(this, title, firstLine + errors.join("\n"), QMessageBox::Ok, QMessageBox::Ok);
	}
	else if(!failedFiles.empty())
	{
		// some mods were not downloaded
		int result = QMessageBox::warning (this, title, firstLine + errors.join("\n") + lastLine,
		                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

		if(result == QMessageBox::Yes)
			doInstallFiles = true;
	}
	else
	{
		// everything OK
		doInstallFiles = true;
	}

	dlManager->deleteLater();
	dlManager = nullptr;
	
	ui->progressBar->setMaximum(0);
	ui->progressBar->setValue(0);

	if(doInstallFiles)
		installFiles(savedFiles);
	
	hideProgressBar();
}

void CModListView::hideProgressBar()
{
	if(dlManager == nullptr) // it was not recreated meanwhile
	{
		ui->progressWidget->setVisible(false);
		ui->progressBar->setMaximum(0);
		ui->progressBar->setValue(0);
	}
}

void CModListView::installFiles(QStringList files)
{
	QStringList mods;
	QStringList maps;
	QStringList images;
	QStringList exe;
	JsonNode repository;

	// TODO: some better way to separate zip's with mods and downloaded repository files
	for(QString filename : files)
	{
		if(filename.endsWith(".zip", Qt::CaseInsensitive))
			mods.push_back(filename);
		else if(filename.endsWith(".h3m", Qt::CaseInsensitive) || filename.endsWith(".h3c", Qt::CaseInsensitive) || filename.endsWith(".vmap", Qt::CaseInsensitive) || filename.endsWith(".vcmp", Qt::CaseInsensitive))
			maps.push_back(filename);
		if(filename.endsWith(".exe", Qt::CaseInsensitive))
			exe.push_back(filename);
		else if(filename.endsWith(".json", Qt::CaseInsensitive))
		{
			//download and merge additional files
			const auto &repoData = JsonUtils::jsonFromFile(filename);
			if(repoData["name"].isNull())
			{
				// This is main repository index. Download all referenced mods
				for(const auto & [modName, modJson] : repoData.Struct())
				{
					auto modNameLower = boost::algorithm::to_lower_copy(modName);
					auto modJsonUrl = modJson["mod"];
					if(!modJsonUrl.isNull())
						downloadFile(QString::fromStdString(modName + ".json"), QString::fromStdString(modJsonUrl.String()), tr("mods repository index"));

					repository[modNameLower] = modJson;
				}
			}
			else
			{
				// This is json of a single mod. Extract name of mod and add it to repo
				auto modName = QFileInfo(filename).baseName().toStdString();
				auto modNameLower = boost::algorithm::to_lower_copy(modName);
				repository[modNameLower] = repoData;
			}
		}
		else if(filename.endsWith(".png", Qt::CaseInsensitive))
			images.push_back(filename);
	}

	if (!repository.isNull())
	{
		manager->appendRepositories(repository);
		modModel->reloadRepositories();

		static const QString repositoryCachePath = CLauncherDirs::downloadsPath() + "/repositoryCache.json";
		JsonUtils::jsonToFile(repositoryCachePath, modStateModel->getRepositoryData());
	}

	if(!mods.empty())
		installMods(mods);

	if(!maps.empty())
		installMaps(maps);

	if(!exe.empty())
	{
		ui->progressBar->setFormat(tr("Installing chronicles"));

		float prog = 0.0;

		auto futureExtract = std::async(std::launch::async, [this, exe, &prog]()
		{
			ChroniclesExtractor ce(this, [&prog](float progress) { prog = progress; });
			ce.installChronicles(exe);
			return true;
		});
		
		while(futureExtract.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)
		{
			emit extractionProgress(static_cast<int>(prog * 1000.f), 1000);
			qApp->processEvents();
		}
		
		if(futureExtract.get())
		{
			//update
			modStateModel->reloadLocalState();
			modModel->reloadRepositories();
		}
	}

	if(!images.empty())
		loadScreenshots();
}

void CModListView::installMods(QStringList archives)
{
	QStringList modNames;
	QStringList modsToEnable;

	for(QString archive : archives)
	{
		// get basename out of full file name
		//                remove path                  remove extension
		QString modName = archive.section('/', -1, -1).section('.', 0, 0);

		modNames.push_back(modName);
	}

	// uninstall old version of mod, if installed
	for(QString mod : modNames)
	{
		if(modStateModel->getMod(mod).isInstalled())
		{
			if (modStateModel->isModEnabled(mod))
				modsToEnable.push_back(mod);

			manager->uninstallMod(mod);
		}
		else
		{
			// installation of previously not present mod -> enable it
			modsToEnable.push_back(mod);
		}
	}

	for(int i = 0; i < modNames.size(); i++)
	{
		ui->progressBar->setFormat(tr("Installing mod %1").arg(modNames[i]));
		manager->installMod(modNames[i], archives[i]);
	}

	manager->enableMods(modsToEnable);

	checkManagerErrors();

	for(QString archive : archives)
		QFile::remove(archive);
}

void CModListView::installMaps(QStringList maps)
{
	const auto destDir = CLauncherDirs::mapsPath() + QChar{'/'};

	for(QString map : maps)
	{
		QFile(map).rename(destDir + map.section('/', -1, -1));
	}
}

void CModListView::on_refreshButton_clicked()
{
	loadRepositories();
}

void CModListView::on_pushButton_clicked()
{
	delete dlManager;
	dlManager = nullptr;
	hideProgressBar();
}

void CModListView::modelReset()
{
	selectMod(filterModel->rowCount() > 0 ? filterModel->index(0, 0) : QModelIndex());
}

void CModListView::checkManagerErrors()
{
	QString errors = manager->getErrors().join('\n');
	if(errors.size() != 0)
	{
		QString title = tr("Operation failed");
		QString description = tr("Encountered errors:\n") + errors;
		QMessageBox::warning(this, title, description, QMessageBox::Ok, QMessageBox::Ok);
	}
}

void CModListView::on_tabWidget_currentChanged(int index)
{
	loadScreenshots();
}

void CModListView::loadScreenshots()
{
	if(ui->tabWidget->currentIndex() == 2)
	{
		if(!ui->allModsView->currentIndex().isValid())
		{
			// select the first mod, so we can access its data
			ui->allModsView->setCurrentIndex(filterModel->index(0, 0));
		}
		
		ui->screenshotsList->clear();
		QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();
		assert(modStateModel->isModExists(modName)); //should be filtered out by check above

		for(QString url : modStateModel->getMod(modName).getScreenshots())
		{
			// URL must be encoded to something else to get rid of symbols illegal in file names
			const auto hashed = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5);
			const auto fileName = QString{QLatin1String{"%1.png"}}.arg(QLatin1String{hashed.toHex()});

			const auto fullPath = QString{QLatin1String{"%1/%2"}}.arg(CLauncherDirs::downloadsPath(), fileName);
			QPixmap pixmap(fullPath);
			if(pixmap.isNull())
			{
				// image file not exists or corrupted - try to redownload
				downloadFile(fileName, url, tr("screenshots"));
			}
			else
			{
				// managed to load cached image
				QIcon icon(pixmap);
				auto * item = new QListWidgetItem(icon, QString(tr("Screenshot %1")).arg(ui->screenshotsList->count() + 1));
				ui->screenshotsList->addItem(item);
			}
		}
	}
}

void CModListView::on_screenshotsList_clicked(const QModelIndex & index)
{
	if(index.isValid())
	{
		QIcon icon = ui->screenshotsList->item(index.row())->icon();
		auto pixmap = icon.pixmap(icon.availableSizes()[0]);
		ImageViewer::showPixmap(pixmap, this);
	}
}

void CModListView::doInstallMod(const QString & modName)
{
	assert(findInvalidDependencies(modName).empty());

	for(const auto & name : modStateModel->getMod(modName).getDependencies())
	{
		auto mod = modStateModel->getMod(name);
		if(!mod.isInstalled())
			downloadFile(name + ".zip", mod.getDownloadUrl(), name, mod.getDownloadSizeMegabytes());
	}
}

bool CModListView::isModAvailable(const QString & modName)
{
	return !modStateModel->isModInstalled(modName);
}

bool CModListView::isModEnabled(const QString & modName)
{
	return modStateModel->isModEnabled(modName);
}

bool CModListView::isModInstalled(const QString & modName)
{
	auto mod = modStateModel->getMod(modName);
	return mod.isInstalled();
}

QString CModListView::getTranslationModName(const QString & language)
{
	for(const auto & modName : modStateModel->getAllMods())
	{
		auto mod = modStateModel->getMod(modName);

		if (!mod.isTranslation())
			continue;

		if (mod.getBaseLanguage() != language)
			continue;

		return modName;
	}

	return QString();
}

void CModListView::on_allModsView_doubleClicked(const QModelIndex &index)
{
	if(!index.isValid())
		return;
	
	auto modName = index.data(ModRoles::ModNameRole).toString();
	auto mod = modStateModel->getMod(modName);
	
	bool hasInvalidDeps = !findInvalidDependencies(modName).empty();
	
	if(!hasInvalidDeps && mod.isAvailable() && !mod.isSubmod())
	{
		on_installButton_clicked();
		return;
	}

	if(!hasInvalidDeps && mod.isUpdateAvailable() && index.column() == ModFields::STATUS_UPDATE)
	{
		on_updateButton_clicked();
		return;
	}
	
	if(index.column() == ModFields::NAME)
	{
		if(ui->allModsView->isExpanded(index))
			ui->allModsView->collapse(index);
		else
			ui->allModsView->expand(index);
		
		return;
	}

	if(!hasInvalidDeps && !modStateModel->isModEnabled(modName))
	{
		on_enableButton_clicked();
		return;
	}

	if(modStateModel->isModEnabled(modName))
	{
		on_disableButton_clicked();
		return;
	}
}
