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

#include "../../lib/CConfigHandler.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/modding/CModVersion.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"

#include <future>

void CModListView::setupModModel()
{
	static const QString repositoryCachePath = CLauncherDirs::downloadsPath() + "/repositoryCache.json";
	const auto &cachedRepositoryData = JsonUtils::jsonFromFile(repositoryCachePath);

	modStateModel = std::make_shared<ModStateModel>();
	if (!cachedRepositoryData.isNull())
		modStateModel->setRepositoryData(cachedRepositoryData);

	modModel = new ModStateItemModel(modStateModel, this);
	manager = std::make_unique<ModStateController>(modStateModel);
}

void CModListView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		modModel->reloadViewModel();
	}
	QWidget::changeEvent(event);
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

	ui->allModsView->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(ui->allModsView, SIGNAL(customContextMenuRequested(const QPoint &)),
		this, SLOT(onCustomContextMenu(const QPoint &)));

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

	ui->uninstallButton->setIcon(QIcon{":/icons/mod-delete.png"});
	ui->enableButton->setIcon(QIcon{":/icons/mod-enabled.png"});
	ui->disableButton->setIcon(QIcon{":/icons/mod-disabled.png"});
	ui->updateButton->setIcon(QIcon{":/icons/mod-update.png"});
	ui->installButton->setIcon(QIcon{":/icons/mod-download.png"});

	ui->splitter->setStyleSheet("QSplitter::handle {background: palette('window');}");

	disableModInfo();
	setupModModel();
	setupFilterModel();
	setupModsView();

	ui->progressWidget->setVisible(false);
	dlManager = nullptr;

	modModel->reloadViewModel();
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

void CModListView::reload(const QString & modToSelect)
{
	modStateModel->reloadLocalState();
	modModel->reloadViewModel();

	if (!modToSelect.isEmpty())
	{
		QModelIndexList matches = filterModel->match(filterModel->index(0, 0), ModRoles::ModNameRole, modToSelect, 1, Qt::MatchExactly | Qt::MatchRecursive);

		if (!matches.isEmpty())
			ui->allModsView->setCurrentIndex(matches.first());
	}
}

void CModListView::loadRepositories()
{
	accumulatedRepositoryData.clear();

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

	QString translationMismatch = tr("This mod cannot be enabled because it translates into a different language.");
	QString notInstalledDeps = tr("This mod can not be enabled because the following dependencies are not present");
	QString unavailableDeps = tr("This mod can not be installed because the following dependencies are not present");
	QString thisIsSubmod = tr("This is a submod and it cannot be installed or uninstalled separately from its parent mod");

	QString notes;

	QStringList notInstalledDependencies = this->getModsToInstall(mod.getID());
	QStringList unavailableDependencies = this->findUnavailableMods(notInstalledDependencies);

	if (mod.isInstalled())
		notes += replaceIfNotEmpty(getModNames(mod.getID(), notInstalledDependencies), listTemplate.arg(notInstalledDeps));
	else
		notes += replaceIfNotEmpty(getModNames(mod.getID(), unavailableDependencies), listTemplate.arg(unavailableDeps));

	if(mod.isSubmod())
		notes += noteTemplate.arg(thisIsSubmod);

	if (mod.isTranslation() && CGeneralTextHandler::getPreferredLanguage() != mod.getBaseLanguage().toStdString())
		notes += noteTemplate.arg(translationMismatch);

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

auto CModListView::buttonEnabledState(QString modName, ModState & mod)
{
	struct result {
		bool disableVisible;
		bool enableVisible;
		bool installVisible;
		bool uninstallVisible;
		bool updateVisible;
		bool directoryVisible;
		bool repositoryVisible;
		bool disableEnabled;
		bool enableEnabled;
		bool installEnabled;
		bool uninstallEnabled;
		bool updateEnabled;
		bool directoryEnabled;
		bool repositoryEnabled;
	} res;

	QStringList notInstalledDependencies = getModsToInstall(modName);
	QStringList unavailableDependencies = findUnavailableMods(notInstalledDependencies);
	bool translationMismatch = mod.isTranslation() && CGeneralTextHandler::getPreferredLanguage() != mod.getBaseLanguage().toStdString();
	bool modIsBeingDownloaded = enqueuedModDownloads.contains(mod.getID());

	res.disableVisible = modStateModel->isModInstalled(mod.getID()) && modStateModel->isModEnabled(mod.getID());
	res.enableVisible = modStateModel->isModInstalled(mod.getID()) && !modStateModel->isModEnabled(mod.getID());
	res.installVisible = mod.isAvailable() && !mod.isSubmod();
	res.uninstallVisible = mod.isInstalled() && !mod.isSubmod();
	res.updateVisible = mod.isUpdateAvailable();
#ifndef VCMI_MOBILE
	res.directoryVisible = mod.isInstalled();
#else
	res.directoryVisible = false;
#endif
	res.repositoryVisible = !mod.getDownloadUrl().isEmpty();

	// Block buttons if action is not allowed at this time
	res.disableEnabled = true;
	res.enableEnabled = notInstalledDependencies.empty() && !translationMismatch;
	res.installEnabled = unavailableDependencies.empty() && !modIsBeingDownloaded;
	res.uninstallEnabled = true;
	res.updateEnabled = unavailableDependencies.empty() && !modIsBeingDownloaded;
	res.directoryEnabled = true;
	res.repositoryEnabled = true;

	return res;
}

void CModListView::onCustomContextMenu(const QPoint &point)
{
	QModelIndex index = ui->allModsView->indexAt(point);
	if(!index.isValid())
		return;

	const auto modName = index.data(ModRoles::ModNameRole).toString();
	auto mod = modStateModel->getMod(modName);

	auto contextMenu = new QMenu(tr("Context menu"), this);
	QList<QAction*> actions;

	auto addContextEntry = [this, &contextMenu, &actions, mod](bool visible, bool enabled, QIcon icon, QString name, std::function<void(ModState)> function){
		if(!visible)
			return;

		actions.append(new QAction(name, this));
		connect(actions.back(), &QAction::triggered, this, [mod, function](){ function(mod); });
		contextMenu->addAction(actions.back());
		actions.back()->setEnabled(enabled);
		actions.back()->setIcon(icon);
	};

	auto state = buttonEnabledState(modName, mod);

	addContextEntry(
		state.disableVisible, state.disableEnabled, QIcon{":/icons/mod-disabled.png"},
		tr("Disable"),
		[this](ModState mod){ disableModByName(mod.getID()); }
	);
	addContextEntry(
		state.enableVisible, state.enableEnabled, QIcon{":/icons/mod-enabled.png"},
		tr("Enable"),
		[this](ModState mod){ enableModByName(mod.getID());
	});
	addContextEntry(
		state.installVisible, state.installEnabled, QIcon{":/icons/mod-download.png"},
		tr("Install"),
		[this](ModState mod){ doInstallMod(mod.getID()); }
	);
	addContextEntry(
		state.uninstallVisible, state.uninstallEnabled, QIcon{":/icons/mod-delete.png"},
		tr("Uninstall"),
		[this](ModState mod){ doUninstallMod(mod.getID()); }
	);
	addContextEntry(
		state.updateVisible, state.updateEnabled, QIcon{":/icons/mod-update.png"},
		tr("Update"),
		[this](ModState mod){ doUpdateMod(mod.getID()); }
	);
	addContextEntry(
		state.directoryVisible, state.directoryEnabled, QIcon{":/icons/menu-mods.png"},
		tr("Open directory"),
		[this](ModState mod){ openModDictionary(mod.getID()); }
	);
	addContextEntry(
		state.repositoryVisible, state.repositoryEnabled, QIcon{":/icons/about-project.png"},
		tr("Open repository"),
		[](ModState mod){
			QUrl url(mod.getDownloadUrl());
			QString repoUrl = QString("%1://%2/%3/%4")
				.arg(url.scheme())
				.arg(url.host())
				.arg(url.path().split('/')[1])
				.arg(url.path().split('/')[2]);
			QDesktopServices::openUrl(repoUrl);
		}
	);

	contextMenu->exec(ui->allModsView->viewport()->mapToGlobal(point));
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

		auto state = buttonEnabledState(modName, mod);

		ui->disableButton->setVisible(state.disableVisible);
		ui->enableButton->setVisible(state.enableVisible);
		ui->installButton->setVisible(state.installVisible);
		ui->uninstallButton->setVisible(state.uninstallVisible);
		ui->updateButton->setVisible(state.updateVisible);

		// Block buttons if action is not allowed at this time
		ui->disableButton->setEnabled(state.disableEnabled);
		ui->enableButton->setEnabled(state.enableEnabled);
		ui->installButton->setEnabled(state.installEnabled);
		ui->uninstallButton->setEnabled(state.uninstallEnabled);
		ui->updateButton->setEnabled(state.updateEnabled);

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

QStringList CModListView::findUnavailableMods(QStringList candidates)
{
	QStringList invalidMods;

	for(QString modName : candidates)
	{
		if(!modStateModel->isModExists(modName))
			invalidMods.push_back(modName);
	}
	return invalidMods;
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
	modModel->modChanged(modName);
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
	modModel->modChanged(modName);
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
		{
			QString topParent = modStateModel->getTopParent(potentialToInstall);
			if (modStateModel->isModInstalled(topParent))
			{
				if (modStateModel->isModUpdateAvailable(topParent))
					potentialToInstall = modStateModel->getTopParent(potentialToInstall);
				// else - potentially broken mod that depends on non-existing submod
			}
			else
				potentialToInstall = modStateModel->getTopParent(potentialToInstall);
		}

		if (!modStateModel->isModInstalled(potentialToInstall))
			result.push_back(potentialToInstall);

		if (modStateModel->isModExists(potentialToInstall))
		{
			QStringList dependencies = modStateModel->getMod(potentialToInstall).getDependencies();
			for (const auto & dependency : dependencies)
			{
				if (!processed.contains(dependency) && !candidates.contains(dependency))
					candidates.push_back(dependency);
			}
		}
	}
	result.removeDuplicates();
	return result;
}

void CModListView::on_updateButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();
	doUpdateMod(modName);

	ui->updateButton->setEnabled(false);
}

void CModListView::doUpdateMod(const QString & modName)
{
	auto targetMod = modStateModel->getMod(modName);

	if(targetMod.isUpdateAvailable())
		downloadMod(targetMod);

	for(const auto & name : getModsToInstall(modName))
	{
		auto mod = modStateModel->getMod(name);
		// update required mod, install missing (can be new dependency)
		if(mod.isUpdateAvailable() || !mod.isInstalled())
			downloadMod(mod);
	}
}

void CModListView::openModDictionary(const QString & modName)
{
	QString tmp = modName;
	tmp.replace(".", "/Mods/");

	ResourcePath resID(std::string("Mods/") + tmp.toStdString(), EResType::DIRECTORY);
	// Get location of the mod, in case-insensitive way
	QString modDir = pathToQString(*CResourceHandler::get()->getResourceName(resID));

	Helper::revealDirectoryInFileBrowser(modDir);
}

void CModListView::on_uninstallButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	doUninstallMod(modName);

	checkManagerErrors();
}

void CModListView::on_installButton_clicked()
{
	QString modName = ui->allModsView->currentIndex().data(ModRoles::ModNameRole).toString();

	doInstallMod(modName);

	ui->installButton->setEnabled(false);
}

void CModListView::downloadMod(const ModState & mod)
{
	if (enqueuedModDownloads.contains(mod.getID()))
		return;

	enqueuedModDownloads.push_back(mod.getID());
	downloadFile(mod.getID() + ".zip", mod.getDownloadUrl(), mod.getName(), mod.getDownloadSizeBytes());
}

void CModListView::downloadFile(QString file, QUrl url, QString description, qint64 sizeBytes)
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

	Helper::keepScreenOn(true);
	dlManager->downloadFile(url, file, sizeBytes);
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

	enqueuedModDownloads.clear();
	dlManager->deleteLater();
	dlManager = nullptr;

	ui->progressBar->setMaximum(0);
	ui->progressBar->setValue(0);

	if(doInstallFiles)
		installFiles(savedFiles);

	Helper::keepScreenOn(false);
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
	bool repositoryFilesEnqueued = false;

	// TODO: some better way to separate zip's with mods and downloaded repository files
	for(QString filename : files)
	{
		QString realFilename = Helper::getRealPath(filename);

		if(realFilename.endsWith(".zip", Qt::CaseInsensitive))
		{
			ZipArchive archive(qstringToPath(realFilename));
			auto fileList = archive.listFiles();

			bool hasModJson = false;
			bool hasMaps = false;

			for (const auto& file : fileList)
			{
				QString lower = QString::fromStdString(file).toLower();

				// Check for mod.json anywhere in archive
				if (lower.endsWith("mod.json"))
					hasModJson = true;

				// Check for map files anywhere
				if (lower.endsWith(".h3m") || lower.endsWith(".h3c") || lower.endsWith(".vmap") || lower.endsWith(".vcmp"))
					hasMaps = true;
			}

			if (hasModJson)
				mods.push_back(filename);
			else if (hasMaps)
				maps.push_back(filename);
			else
				mods.push_back(filename);
		}
		else if(realFilename.endsWith(".h3m", Qt::CaseInsensitive) || realFilename.endsWith(".h3c", Qt::CaseInsensitive) || realFilename.endsWith(".vmap", Qt::CaseInsensitive) || realFilename.endsWith(".vcmp", Qt::CaseInsensitive))
			maps.push_back(filename);
		if(realFilename.endsWith(".exe", Qt::CaseInsensitive))
			exe.push_back(filename);
		else if(realFilename.endsWith(".json", Qt::CaseInsensitive))
		{
			//download and merge additional files
			JsonNode repoData = JsonUtils::jsonFromFile(filename);
			if(repoData["name"].isNull())
			{
				// MODS COMPATIBILITY: in 1.6, repository list contains mod list directly, in 1.7 it is located in 'availableMods' node
				const auto & availableRepositoryMods = repoData["availableMods"].isNull() ? repoData : repoData["availableMods"];

				// This is main repository index. Download all referenced mods
				for(const auto & [modName, modJson] : availableRepositoryMods.Struct())
				{
					auto modNameLower = boost::algorithm::to_lower_copy(modName);
					auto modJsonUrl = modJson["mod"];
					if(!modJsonUrl.isNull())
					{
						downloadFile(QString::fromStdString(modName + ".json"), QString::fromStdString(modJsonUrl.String()), tr("mods repository index"));
						repositoryFilesEnqueued = true;
					}

					accumulatedRepositoryData[modNameLower] = modJson;
				}
			}
			else
			{
				// This is json of a single mod. Extract name of mod and add it to repo
				auto modName = QFileInfo(filename).baseName().toStdString();
				auto modNameLower = boost::algorithm::to_lower_copy(modName);
				JsonUtils::merge(accumulatedRepositoryData[modNameLower], repoData);
			}
		}
		else if(realFilename.endsWith(".png", Qt::CaseInsensitive))
			images.push_back(filename);
	}

	if (!accumulatedRepositoryData.isNull() && !repositoryFilesEnqueued)
	{
		logGlobal->info("Installing repository: started");
		manager->setRepositoryData(accumulatedRepositoryData);
		modModel->reloadViewModel();
		accumulatedRepositoryData.clear();

		static const QString repositoryCachePath = CLauncherDirs::downloadsPath() + "/repositoryCache.json";
		JsonUtils::jsonToFile(repositoryCachePath, modStateModel->getRepositoryData());
		logGlobal->info("Installing repository: ended");
	}

	if(!mods.empty())
	{
		logGlobal->info("Installing mods: started");
		installMods(mods);
		logGlobal->info("Installing mods: ended");
	}

	if(!maps.empty())
	{
		logGlobal->info("Installing maps: started");
		installMaps(maps);
		logGlobal->info("Installing maps: ended");
	}

	if(!exe.empty())
	{
		logGlobal->info("Installing chronicles: started");
		ui->progressBar->setFormat(tr("Installing Heroes Chronicles"));
		ui->progressWidget->setVisible(true);
		ui->abortButton->setEnabled(false);

		float prog = 0.0;

		Helper::keepScreenOn(true);

		auto futureExtract = std::async(std::launch::async, [this, exe, &prog]()
		{
			ChroniclesExtractor ce(this, [&prog](float progress) { prog = progress; });
			ce.installChronicles(exe);
			return true;
		});

		while(futureExtract.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)
		{
			extractionProgress(static_cast<int>(prog * 1000.f), 1000);
			qApp->processEvents();
		}

		if(futureExtract.get())
		{
			Helper::keepScreenOn(false);
			hideProgressBar();
			ui->abortButton->setEnabled(true);
			ui->progressWidget->setVisible(false);
			//update
			reload("chronicles");
			if (modStateModel->isModExists("chronicles"))
				enableModByName("chronicles");
		}
		logGlobal->info("Installing chronicles: ended");
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

	if (!activatingPreset.isEmpty())
	{
		modStateModel->activatePreset(activatingPreset);
		activatingPreset.clear();
	}

	// uninstall old version of mod, if installed
	for(QString mod : modNames)
	{
		if(modStateModel->isModExists(mod) && modStateModel->getMod(mod).isInstalled())
		{
			logGlobal->info("Uninstalling old version of mod '%s'", mod.toStdString());
			if (modStateModel->isModEnabled(mod))
				modsToEnable.push_back(mod);

			manager->uninstallMod(mod);
			reload(mod);
		}
		else
		{
			// installation of previously not present mod -> enable it
			modsToEnable.push_back(mod);
		}
	}

	QString lastInstalled;

	for(int i = 0; i < modNames.size(); i++)
	{
		logGlobal->info("Installing mod '%s'", modNames[i].toStdString());
		ui->progressBar->setFormat(tr("Installing mod %1").arg(modStateModel->getMod(modNames[i]).getName()));

		manager->installMod(modNames[i], archives[i]);

		if (i == modNames.size() - 1 && modStateModel->isModExists(modNames[i]))
			lastInstalled = modStateModel->getMod(modNames[i]).getID();
	}


	reload(lastInstalled);

	if (!modsToEnable.empty())
	{
		manager->enableMods(modsToEnable);
	}

	checkManagerErrors();

	for(QString archive : archives)
	{
		logGlobal->info("Erasing archive '%s'", archive.toStdString());
		QFile::remove(archive);
	}
}

void CModListView::installMaps(QStringList maps)
{
	const auto destDir = CLauncherDirs::mapsPath() + QChar{'/'};
	int successCount = 0;
	QStringList failedMaps;

	// Pre-scan maps to count total conflicts (used for Yes to All/No to All)
	int conflictCount = 0;
	for (const QString& map : maps)
	{
		if (map.endsWith(".zip", Qt::CaseInsensitive))
		{
			ZipArchive archive(qstringToPath(map));
			for (const auto& file : archive.listFiles())
			{
				QString name = QString::fromStdString(file);
				if (name.endsWith(".h3m", Qt::CaseInsensitive) || name.endsWith(".h3c", Qt::CaseInsensitive) ||
					name.endsWith(".vmap", Qt::CaseInsensitive) || name.endsWith(".vcmp", Qt::CaseInsensitive))
				{
					if (QFile::exists(destDir + name))
						conflictCount++;
				}
			}
		}
		else
		{
		    QString srcPath = Helper::getRealPath(map);
		    QString fileName = QFileInfo(srcPath).fileName();
		    QString destFile = destDir + fileName;
			if (QFile::exists(destFile))
				conflictCount++;
		}
	}

	bool applyToAll = false;
	bool overwriteAll = false;

	auto askOverwrite = [&](const QString& name) -> bool {
		if (applyToAll)
			return overwriteAll;

		QMessageBox msgBox(this);
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setWindowTitle(tr("Map exists"));
		msgBox.setText(tr("Map '%1' already exists. Do you want to overwrite it?").arg(name));

		QPushButton* yes = msgBox.addButton(QMessageBox::Yes);
		msgBox.addButton(QMessageBox::No);

		QPushButton* yesAll = nullptr;
		QPushButton* noAll = nullptr;
		if (conflictCount > 1)
		{
			yesAll = msgBox.addButton(tr("Yes to All"), QMessageBox::YesRole);
			noAll = msgBox.addButton(tr("No to All"), QMessageBox::NoRole);
		}

		msgBox.exec();
		QAbstractButton* clicked = msgBox.clickedButton();

		if (clicked == yes)
			return true;
		if (clicked == yesAll)
		{
			applyToAll = true;
			overwriteAll = true;
			return true;
		}
		if (clicked == noAll)
		{
			applyToAll = true;
			overwriteAll = false;
			return false;
		}
		return false;
	};

	// Process each map file and archive
	for (const QString& map : maps)
	{
		if (map.endsWith(".zip", Qt::CaseInsensitive))
		{
			// ZIP archive
			ZipArchive archive(qstringToPath(map));
			for (const auto& file : archive.listFiles())
			{
				QString name = QString::fromStdString(file);
				if (!(name.endsWith(".h3m", Qt::CaseInsensitive) || name.endsWith(".h3c", Qt::CaseInsensitive) ||
					name.endsWith(".vmap", Qt::CaseInsensitive) || name.endsWith(".vcmp", Qt::CaseInsensitive)))
					continue;

				QString destFile = destDir + name;
				logGlobal->info("Importing map '%s' from ZIP '%s'", name.toStdString(), map.toStdString());

				if (QFile::exists(destFile))
				{
					if (!askOverwrite(name))
					{
						logGlobal->info("Skipped map '%s'", name.toStdString());
						continue;
					}
					QFile::remove(destFile);
				}

				if (archive.extract(qstringToPath(destDir), file))
					successCount++;
				else
				{
					logGlobal->warn("Failed to extract map '%s'", name.toStdString());
					failedMaps.push_back(name);
				}
			}
		}
		else
		{
			// Single map file
			QString srcPath = Helper::getRealPath(map);
			QString fileName = QFileInfo(srcPath).fileName();
			QString destFile = destDir + fileName;

			logGlobal->info("Importing map '%s'", srcPath.toStdString());

			if (QFile::exists(destFile))
			{
				if (!askOverwrite(fileName))
				{
					logGlobal->info("Skipped map '%s'", fileName.toStdString());
					continue;
				}
				QFile::remove(destFile);
			}

			if (Helper::performNativeCopy(map, destFile))
				successCount++;
			else
			{
				logGlobal->warn("Failed to copy map '%s'", fileName.toStdString());
				failedMaps.push_back(fileName);
			}
		}
	}

	if (successCount > 0)
		QMessageBox::information(this, tr("Import complete"), tr("%1 map(s) successfully imported.").arg(successCount));

	if (!failedMaps.isEmpty())
		QMessageBox::warning(this, tr("Import failed"), tr("Failed to import the following maps:\n%1").arg(failedMaps.join("\n")));
}

void CModListView::on_refreshButton_clicked()
{
	loadRepositories();
}

void CModListView::on_abortButton_clicked()
{
	delete dlManager;
	dlManager = nullptr;
	Helper::keepScreenOn(false);
	hideProgressBar();
}

void CModListView::modelReset()
{
	ui->allModsView->setCurrentIndex(filterModel->rowCount() > 0 ? filterModel->index(0, 0) : QModelIndex());
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
	if(ui->tabWidget->currentIndex() != 2)
		return;

	assert(ui->allModsView->currentIndex().isValid());


	if (!ui->allModsView->currentIndex().isValid())
		return;

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
	for(const auto & name : getModsToInstall(modName))
	{
		auto mod = modStateModel->getMod(name);
		if(mod.isAvailable())
			downloadMod(mod);
		else if(!modStateModel->isModEnabled(name))
			enableModByName(name);
	}
}

void CModListView::doUninstallMod(const QString & modName)
{
	if(modStateModel->isModExists(modName) && modStateModel->getMod(modName).isInstalled())
	{
		if(modStateModel->isModEnabled(modName))
			manager->disableMod(modName);
		manager->uninstallMod(modName);
		reload(modName);
	}
}

bool CModListView::isModAvailable(const QString & modName)
{
	return modStateModel->isModExists(modName) && !modStateModel->isModInstalled(modName);
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

QStringList CModListView::getInstalledChronicles()
{
	QStringList result;

	for(const auto & modName : modStateModel->getAllMods())
	{
		auto mod = modStateModel->getMod(modName);
		if (!mod.isInstalled())
			continue;

		if (mod.getTopParentID() != "chronicles")
			continue;

		result += modName;
	}

	return result;
}

QStringList CModListView::getUpdateableMods()
{
	QStringList result;

	for(const auto & modName : modStateModel->getAllMods())
	{
		auto mod = modStateModel->getMod(modName);
		if (!mod.isUpdateAvailable())
			continue;

		QStringList notInstalledDependencies = getModsToInstall(mod.getID());
		QStringList unavailableDependencies = findUnavailableMods(notInstalledDependencies);

		if (unavailableDependencies.empty())
			result.push_back(modName);
	}

	return result;
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

	QStringList notInstalledDependencies = this->getModsToInstall(mod.getID());
	QStringList unavailableDependencies = this->findUnavailableMods(notInstalledDependencies);

	if(unavailableDependencies.empty() && mod.isAvailable() && !mod.isSubmod())
	{
		on_installButton_clicked();
		return;
	}

	if(unavailableDependencies.empty() && mod.isUpdateAvailable() && index.column() == ModFields::STATUS_UPDATE)
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

	if(notInstalledDependencies.empty() && !modStateModel->isModEnabled(modName))
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

void CModListView::createNewPreset(const QString & presetName)
{
	modStateModel->createNewPreset(presetName);
}

void CModListView::deletePreset(const QString & presetName)
{
	modStateModel->deletePreset(presetName);
}

void CModListView::activatePreset(const QString & presetName)
{
	modStateModel->activatePreset(presetName);
	reload();
}

void CModListView::renamePreset(const QString & oldPresetName, const QString & newPresetName)
{
	modStateModel->renamePreset(oldPresetName, newPresetName);
}

QStringList CModListView::getAllPresets() const
{
	return modStateModel->getAllPresets();
}

QString CModListView::getActivePreset() const
{
	return modStateModel->getActivePreset();
}

JsonNode CModListView::exportCurrentPreset() const
{
	return modStateModel->exportCurrentPreset();
}

void CModListView::importPreset(const JsonNode & data)
{
	const auto & [presetName, modList] = modStateModel->importPreset(data);

	if (modList.empty())
	{
		modStateModel->activatePreset(presetName);
		modStateModel->reloadLocalState();
	}
	else
	{
		activatingPreset = presetName;
		for (const auto & modID : modList)
			doInstallMod(modID);
	}
}
