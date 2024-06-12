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

#include "cmodlistmodel_moc.h"
#include "cmodmanager.h"
#include "cdownloadmanager_moc.h"
#include "../settingsView/csettingsview_moc.h"
#include "../launcherdirs.h"
#include "../jsonutils.h"
#include "../helper.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/Languages.h"
#include "../../lib/modding/CModVersion.h"

static double mbToBytes(double mb)
{
	return mb * 1024 * 1024;
}

void CModListView::setupModModel()
{
	modModel = new CModListModel(this);
	manager = std::make_unique<CModManager>(modModel);
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
			for(const auto & ending : QStringList({".zip", ".h3m", ".h3c", ".vmap", ".vcmp", ".json"}))
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
			manualInstallFile(url);
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

	setupModModel();
	setupFilterModel();
	setupModsView();

	ui->progressWidget->setVisible(false);
	dlManager = nullptr;

	if(settings["launcher"]["autoCheckRepositories"].Bool())
	{
		loadRepositories();
	}
	else
	{
		manager->resetRepositories();
	}

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
	manager->resetRepositories();

	QStringList repositories;

	if (settings["launcher"]["defaultRepositoryEnabled"].Bool())
		repositories.push_back(QString::fromStdString(settings["launcher"]["defaultRepositoryURL"].String()));

	if (settings["launcher"]["extraRepositoryEnabled"].Bool())
		repositories.push_back(QString::fromStdString(settings["launcher"]["extraRepositoryURL"].String()));

	for(auto entry : repositories)
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
	if(value.canConvert<QStringList>())
		return pattern.arg(value.toStringList().join(", "));

	if(value.canConvert<QString>())
		return pattern.arg(value.toString());

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

QString CModListView::genChangelogText(CModEntry & mod)
{
	QString headerTemplate = "<p><span style=\" font-weight:600;\">%1: </span></p>";
	QString entryBegin = "<p align=\"justify\"><ul>";
	QString entryEnd = "</ul></p>";
	QString entryLine = "<li>%1</li>";
	//QString versionSeparator = "<hr/>";

	QString result;

	QVariantMap changelog = mod.getValue("changelog").toMap();
	QList<QString> versions = changelog.keys();

	std::sort(versions.begin(), versions.end(), [](QString lesser, QString greater)
	{
		return CModVersion::fromString(lesser.toStdString()) < CModVersion::fromString(greater.toStdString());
	});
	std::reverse(versions.begin(), versions.end());

	for(auto & version : versions)
	{
		result += headerTemplate.arg(version);
		result += entryBegin;
		for(auto & line : changelog.value(version).toStringList())
			result += entryLine.arg(line);
		result += entryEnd;
	}
	return result;
}

QStringList CModListView::getModNames(QStringList input)
{
	QStringList result;

	for(const auto & modID : input)
	{
		auto mod = modModel->getMod(modID.toLower());

		QString modName = mod.getValue("name").toString();

		if (modName.isEmpty())
			result += modID.toLower();
		else
			result += modName;
	}

	return result;
}

QString CModListView::genModInfoText(CModEntry & mod)
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

	result += replaceIfNotEmpty(mod.getValue("name"), lineTemplate.arg(tr("Mod name")));
	result += replaceIfNotEmpty(mod.getValue("installedVersion"), lineTemplate.arg(tr("Installed version")));
	result += replaceIfNotEmpty(mod.getValue("latestVersion"), lineTemplate.arg(tr("Latest version")));

	if(mod.getValue("localSizeBytes").isValid())
		result += replaceIfNotEmpty(CModEntry::sizeToString(mod.getValue("localSizeBytes").toDouble()), lineTemplate.arg(tr("Size")));
	if((mod.isAvailable() || mod.isUpdateable()) && mod.getValue("downloadSize").isValid())
		result += replaceIfNotEmpty(CModEntry::sizeToString(mbToBytes(mod.getValue("downloadSize").toDouble())), lineTemplate.arg(tr("Download size")));
	
	result += replaceIfNotEmpty(mod.getValue("author"), lineTemplate.arg(tr("Authors")));

	if(mod.getValue("licenseURL").isValid())
		result += urlTemplate.arg(tr("License")).arg(mod.getValue("licenseURL").toString()).arg(mod.getValue("licenseName").toString());

	if(mod.getValue("contact").isValid())
		result += urlTemplate.arg(tr("Contact")).arg(mod.getValue("contact").toString()).arg(mod.getValue("contact").toString());

	//compatibility info
	if(!mod.isCompatible())
	{
		auto compatibilityInfo = mod.getValue("compatibility").toMap();
		auto minStr = compatibilityInfo.value("min").toString();
		auto maxStr = compatibilityInfo.value("max").toString();

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

	QStringList supportedLanguages;
	QVariant baseLanguageVariant = mod.getBaseValue("language");
	QString baseLanguageID = baseLanguageVariant.isValid() ? baseLanguageVariant.toString() : "english";

	bool needToShowSupportedLanguages = false;

	for(const auto & language : Languages::getLanguageList())
	{
		QString languageID = QString::fromStdString(language.identifier);

		if (languageID != baseLanguageID && !mod.getValue(languageID).isValid())
			continue;

		if (languageID != baseLanguageID)
			needToShowSupportedLanguages = true;

		supportedLanguages += QApplication::translate("Language", language.nameEnglish.c_str());
	}

	if(needToShowSupportedLanguages)
		result += replaceIfNotEmpty(supportedLanguages, lineTemplate.arg(tr("Languages")));

	result += replaceIfNotEmpty(getModNames(mod.getDependencies()), lineTemplate.arg(tr("Required mods")));
	result += replaceIfNotEmpty(getModNames(mod.getConflicts()), lineTemplate.arg(tr("Conflicting mods")));
	result += replaceIfNotEmpty(mod.getValue("description"), textTemplate.arg(tr("Description")));

	result += "<p></p>"; // to get some empty space

	QString unknownDeps = tr("This mod can not be installed or enabled because the following dependencies are not present");
	QString blockingMods = tr("This mod can not be enabled because the following mods are incompatible with it");
	QString hasActiveDependentMods = tr("This mod cannot be disabled because it is required by the following mods");
	QString hasDependentMods = tr("This mod cannot be uninstalled or updated because it is required by the following mods");
	QString thisIsSubmod = tr("This is a submod and it cannot be installed or uninstalled separately from its parent mod");

	QString notes;

	notes += replaceIfNotEmpty(getModNames(findInvalidDependencies(mod.getName())), listTemplate.arg(unknownDeps));
	notes += replaceIfNotEmpty(getModNames(findBlockingMods(mod.getName())), listTemplate.arg(blockingMods));
	if(mod.isEnabled())
		notes += replaceIfNotEmpty(getModNames(findDependentMods(mod.getName(), true)), listTemplate.arg(hasActiveDependentMods));
	if(mod.isInstalled())
		notes += replaceIfNotEmpty(getModNames(findDependentMods(mod.getName(), false)), listTemplate.arg(hasDependentMods));

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
	if(!index.isValid())
	{
		disableModInfo();
	}
	else
	{
		const auto modName = index.data(ModRoles::ModNameRole).toString();
		auto mod = modModel->getMod(modName);

		ui->modInfoBrowser->setHtml(genModInfoText(mod));
		ui->changelogBrowser->setHtml(genChangelogText(mod));

		Helper::enableScrollBySwiping(ui->modInfoBrowser);
		Helper::enableScrollBySwiping(ui->changelogBrowser);

		bool hasInvalidDeps = !findInvalidDependencies(modName).empty();
		bool hasBlockingMods = !findBlockingMods(modName).empty();
		bool hasDependentMods = !findDependentMods(modName, true).empty();

		ui->disableButton->setVisible(mod.isEnabled());
		ui->enableButton->setVisible(mod.isDisabled());
		ui->installButton->setVisible(mod.isAvailable() && !mod.isSubmod());
		ui->uninstallButton->setVisible(mod.isInstalled() && !mod.isSubmod());
		ui->updateButton->setVisible(mod.isUpdateable());

		// Block buttons if action is not allowed at this time
		// TODO: automate handling of some of these cases instead of forcing player
		// to resolve all conflicts manually.
		ui->disableButton->setEnabled(!hasDependentMods && !mod.isEssential());
		ui->enableButton->setEnabled(!hasBlockingMods && !hasInvalidDeps);
		ui->installButton->setEnabled(!hasInvalidDeps);
		ui->uninstallButton->setEnabled(!hasDependentMods && !mod.isEssential());
		ui->updateButton->setEnabled(!hasInvalidDeps && !hasDependentMods);

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
	switch(index)
	{
	case 0:
		filterModel->setTypeFilter(ModStatus::MASK_NONE, ModStatus::MASK_NONE);
		break;
	case 1:
		filterModel->setTypeFilter(ModStatus::MASK_NONE, ModStatus::INSTALLED);
		break;
	case 2:
		filterModel->setTypeFilter(ModStatus::INSTALLED, ModStatus::INSTALLED);
		break;
	case 3:
		filterModel->setTypeFilter(ModStatus::UPDATEABLE, ModStatus::UPDATEABLE);
		break;
	case 4:
		filterModel->setTypeFilter(ModStatus::ENABLED | ModStatus::INSTALLED, ModStatus::ENABLED | ModStatus::INSTALLED);
		break;
	case 5:
		filterModel->setTypeFilter(ModStatus::INSTALLED, ModStatus::ENABLED | ModStatus::INSTALLED);
		break;
	}
}

QStringList CModListView::findInvalidDependencies(QString mod)
{
	QStringList ret;
	for(QString requirement : modModel->getRequirements(mod))
	{
		if(!modModel->hasMod(requirement) && !modModel->hasMod(requirement.split(QChar('.'))[0]))
			ret += requirement;
	}
	return ret;
}

QStringList CModListView::findBlockingMods(QString modUnderTest)
{
	QStringList ret;
	auto required = modModel->getRequirements(modUnderTest);

	for(QString name : modModel->getModList())
	{
		auto mod = modModel->getMod(name);

		if(mod.isEnabled())
		{
			// one of enabled mods have requirement (or this mod) marked as conflict
			for(auto conflict : mod.getConflicts())
			{
				if(required.contains(conflict))
					ret.push_back(name);
			}
		}
	}

	return ret;
}

QStringList CModListView::findDependentMods(QString mod, bool excludeDisabled)
{
	QStringList ret;
	for(QString modName : modModel->getModList())
	{
		auto current = modModel->getMod(modName);

		if(!current.isInstalled() || !current.isVisible())
			continue;

		if(current.getDependencies().contains(mod, Qt::CaseInsensitive))
		{
			if(!(current.isDisabled() && excludeDisabled))
				ret += modName;
		}
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
	assert(findBlockingMods(modName).empty());
	assert(findInvalidDependencies(modName).empty());

	for(auto & name : modModel->getRequirements(modName))
	{
		if(modModel->getMod(name).isDisabled())
			manager->enableMod(name);
	}
	emit modsChanged();
}

void CModListView::on_disableButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	disableModByName(modName);
	
	checkManagerErrors();
}

void CModListView::disableModByName(QString modName)
{
	if(modModel->hasMod(modName) && modModel->getMod(modName).isEnabled())
		manager->disableMod(modName);

	emit modsChanged();
}

void CModListView::on_updateButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	assert(findInvalidDependencies(modName).empty());

	for(auto & name : modModel->getRequirements(modName))
	{
		auto mod = modModel->getMod(name);
		// update required mod, install missing (can be new dependency)
		if(mod.isUpdateable() || !mod.isInstalled())
			downloadFile(name + ".zip", mod.getValue("download").toString(), name, mbToBytes(mod.getValue("downloadSize").toDouble()));
	}
}

void CModListView::on_uninstallButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();
	// NOTE: perhaps add "manually installed" flag and uninstall those dependencies that don't have it?

	if(modModel->hasMod(modName) && modModel->getMod(modName).isInstalled())
	{
		if(modModel->getMod(modName).isEnabled())
			manager->disableMod(modName);
		manager->uninstallMod(modName);
	}
	
	emit modsChanged();
	checkManagerErrors();
}

void CModListView::on_installButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	assert(findInvalidDependencies(modName).empty());

	for(auto & name : modModel->getRequirements(modName))
	{
		auto mod = modModel->getMod(name);
		if(mod.isAvailable())
			downloadFile(name + ".zip", mod.getValue("download").toString(), name, mbToBytes(mod.getValue("downloadSize").toDouble()));
		else if(!mod.isEnabled())
			enableModByName(name);
	}

	for(auto & name : modModel->getMod(modName).getConflicts())
	{
		auto mod = modModel->getMod(name);
		if(mod.isEnabled())
		{
			//TODO: consider reverse dependencies disabling
			//TODO: consider if it may be possible for subdependencies to block disabling conflicting mod?
			//TODO: consider if it may be possible to get subconflicts that will block disabling conflicting mod?
			disableModByName(name);
		}
	}
}

void CModListView::on_installFromFileButton_clicked()
{
	QString filter = tr("All supported files") + " (*.h3m *.vmap *.h3c *.vcmp *.zip *.json);;" + tr("Maps") + " (*.h3m *.vmap);;" + tr("Campaigns") + " (*.h3c *.vcmp);;" + tr("Configs") + " (*.json);;" + tr("Mods") + " (*.zip)";
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Select files (configs, mods, maps, campaigns) to install..."), QDir::homePath(), filter);

	for (const auto & file : files)
	{
		QUrl url = QUrl::fromLocalFile(file);
		manualInstallFile(url);
	}
}

void CModListView::manualInstallFile(QUrl url)
{
	QString urlStr = url.toString();
	QString fileName = url.fileName();
	if(urlStr.endsWith(".zip", Qt::CaseInsensitive))
		downloadFile(fileName.toLower()
			// mod name currently comes from zip file -> remove suffixes from github zip download
			.replace(QRegularExpression("-[0-9a-f]{40}"), "")
			.replace(QRegularExpression("-vcmi-.+\\.zip"), ".zip")
			.replace("-main.zip", ".zip")
			, urlStr, "mods", 0);
	else if(urlStr.endsWith(".json", Qt::CaseInsensitive))
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
				QFile::copy(url.toLocalFile(), configFilePath);

				// reload settings
				Helper::loadSettings();
				for(auto widget : qApp->allWidgets())
					if(auto settingsView = qobject_cast<CSettingsView *>(widget))
						settingsView->loadSettings();
				manager->loadMods();
				manager->loadModSettings();
			}
		}
	}
	else
		downloadFile(fileName, urlStr, fileName, 0);
}

void CModListView::downloadFile(QString file, QString url, QString description, qint64 size)
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
		
		connect(modModel, &CModListModel::dataChanged, filterModel, &QAbstractItemModel::dataChanged);

		
		QString progressBarFormat = tr("Downloading %s%. %p% (%v MB out of %m MB) finished");

		progressBarFormat.replace("%s%", description);
		ui->progressBar->setFormat(progressBarFormat);
	}

	dlManager->downloadFile(QUrl(url), file, size);
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
	emit modsChanged();
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
	QVector<QVariantMap> repositories;

	// TODO: some better way to separate zip's with mods and downloaded repository files
	for(QString filename : files)
	{
		if(filename.endsWith(".zip", Qt::CaseInsensitive))
			mods.push_back(filename);
		else if(filename.endsWith(".h3m", Qt::CaseInsensitive) || filename.endsWith(".h3c", Qt::CaseInsensitive) || filename.endsWith(".vmap", Qt::CaseInsensitive) || filename.endsWith(".vcmp", Qt::CaseInsensitive))
			maps.push_back(filename);
		else if(filename.endsWith(".json", Qt::CaseInsensitive))
		{
			//download and merge additional files
			auto repoData = JsonUtils::JsonFromFile(filename).toMap();
			if(repoData.value("name").isNull())
			{
				for(const auto & key : repoData.keys())
				{
					auto modjson = repoData[key].toMap().value("mod");
					if(!modjson.isNull())
					{
						downloadFile(key + ".json", modjson.toString(), tr("mods repository index"));
					}
				}
			}
			else
			{
				auto modn = QFileInfo(filename).baseName();
				QVariantMap temp;
				temp[modn] = repoData;
				repoData = temp;
			}
			repositories.push_back(repoData);
		}
		else if(filename.endsWith(".png", Qt::CaseInsensitive))
			images.push_back(filename);
	}

	if (!repositories.empty())
		manager->loadRepositories(repositories);

	if(!mods.empty())
		installMods(mods);

	if(!maps.empty())
		installMaps(maps);

	if(!images.empty())
		loadScreenshots();
}

void CModListView::installMods(QStringList archives)
{
	QStringList modNames;

	for(QString archive : archives)
	{
		// get basename out of full file name
		//                remove path                  remove extension
		QString modName = archive.section('/', -1, -1).section('.', 0, 0);

		modNames.push_back(modName);
	}

	QStringList modsToEnable;

	// disable mod(s), to properly recalculate dependencies, if changed
	for(QString mod : boost::adaptors::reverse(modNames))
	{
		CModEntry entry = modModel->getMod(mod);
		if(entry.isInstalled())
		{
			// enable mod if installed and enabled
			if(entry.isEnabled())
				modsToEnable.push_back(mod);
		}
		else
		{
			// enable mod if m
			if(settings["launcher"]["enableInstalledMods"].Bool())
				modsToEnable.push_back(mod);
		}
	}

	// uninstall old version of mod, if installed
	for(QString mod : boost::adaptors::reverse(modNames))
	{
		if(modModel->getMod(mod).isInstalled())
			manager->uninstallMod(mod);
	}

	for(int i = 0; i < modNames.size(); i++)
	{
		ui->progressBar->setFormat(tr("Installing mod %1").arg(modNames[i]));
		manager->installMod(modNames[i], archives[i]);
	}

	std::function<void(QString)> enableMod;

	enableMod = [&](QString modName)
	{
		auto mod = modModel->getMod(modName);
		if(mod.isInstalled() && !mod.getValue("keepDisabled").toBool())
		{
			for (auto const & dependencyName : mod.getDependencies())
			{
				auto dependency = modModel->getMod(dependencyName);
				if(dependency.isDisabled())
					manager->enableMod(dependencyName);
			}

			if(mod.isDisabled() && manager->enableMod(modName))
			{
				for(QString child : modModel->getChildren(modName))
					enableMod(child);
			}
		}
	};

	for(QString mod : modsToEnable)
	{
		enableMod(mod);
	}

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
		ui->screenshotsList->clear();
		QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();
		assert(modModel->hasMod(modName)); //should be filtered out by check above

		for(QString url : modModel->getMod(modName).getValue("screenshots").toStringList())
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

const CModList & CModListView::getModList() const
{
	assert(modModel);
	return *modModel;
}

void CModListView::doInstallMod(const QString & modName)
{
	assert(findInvalidDependencies(modName).empty());

	for(auto & name : modModel->getRequirements(modName))
	{
		auto mod = modModel->getMod(name);
		if(!mod.isInstalled())
			downloadFile(name + ".zip", mod.getValue("download").toString(), name, mbToBytes(mod.getValue("downloadSize").toDouble()));
	}
}

bool CModListView::isModAvailable(const QString & modName)
{
	auto mod = modModel->getMod(modName);
	return mod.isAvailable();
}

bool CModListView::isModEnabled(const QString & modName)
{
	auto mod = modModel->getMod(modName);
	return mod.isEnabled();
}

QString CModListView::getTranslationModName(const QString & language)
{
	for(const auto & modName : modModel->getModList())
	{
		auto mod = modModel->getMod(modName);

		if (!mod.isTranslation())
			continue;

		if (mod.getBaseValue("language").toString() != language)
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
	auto mod = modModel->getMod(modName);
	
	bool hasInvalidDeps = !findInvalidDependencies(modName).empty();
	bool hasBlockingMods = !findBlockingMods(modName).empty();
	bool hasDependentMods = !findDependentMods(modName, true).empty();
	
	if(!hasInvalidDeps && mod.isAvailable() && !mod.isSubmod())
	{
		on_installButton_clicked();
		return;
	}

	if(!hasInvalidDeps && !hasDependentMods && mod.isUpdateable() && index.column() == ModFields::STATUS_UPDATE)
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

	if(!hasBlockingMods && !hasInvalidDeps && mod.isDisabled())
	{
		on_enableButton_clicked();
		return;
	}

	if(!hasDependentMods && !mod.isEssential() && mod.isEnabled())
	{
		on_disableButton_clicked();
		return;
	}
}
