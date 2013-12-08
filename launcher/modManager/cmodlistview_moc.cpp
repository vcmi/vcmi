#include "StdInc.h"
#include "cmodlistview_moc.h"
#include "ui_cmodlistview_moc.h"

#include <QJsonArray>
#include <QCryptographicHash>

#include "cmodlistmodel_moc.h"
#include "cmodmanager.h"
#include "cdownloadmanager_moc.h"
#include "../launcherdirs.h"

#include "../../lib/CConfigHandler.h"

void CModListView::setupModModel()
{
	modModel = new CModListModel();
	manager = new CModManager(modModel);
}

void CModListView::setupFilterModel()
{
	filterModel = new CModFilterModel(modModel);

	filterModel->setFilterKeyColumn(-1); // filter across all columns
	filterModel->setSortCaseSensitivity(Qt::CaseInsensitive); // to make it more user-friendly
}

void CModListView::setupModsView()
{
	ui->allModsView->setModel(filterModel);
	// input data is not sorted - sort it before display
	ui->allModsView->sortByColumn(ModFields::TYPE, Qt::AscendingOrder);
	ui->allModsView->setColumnWidth(ModFields::STATUS_ENABLED, 30);
	ui->allModsView->setColumnWidth(ModFields::STATUS_UPDATE, 30);
	ui->allModsView->setColumnWidth(ModFields::NAME, 120);
	ui->allModsView->setColumnWidth(ModFields::SIZE, 60);
	ui->allModsView->setColumnWidth(ModFields::VERSION, 60);

	connect( ui->allModsView->selectionModel(), SIGNAL( currentRowChanged( const QModelIndex &, const QModelIndex & )),
	         this, SLOT( modSelected( const QModelIndex &, const QModelIndex & )));

	connect( filterModel, SIGNAL( modelReset()),
	         this, SLOT( modelReset()));

	selectMod(filterModel->rowCount() > 0 ? 0 : -1);
}

CModListView::CModListView(QWidget *parent) :
	QWidget(parent),
    settingsListener(settings.listen["launcher"]["repositoryURL"]),
	ui(new Ui::CModListView)
{
	settingsListener([&](const JsonNode &){ repositoriesChanged = true; });
	ui->setupUi(this);

	setupModModel();
	setupFilterModel();
	setupModsView();

	ui->progressWidget->setVisible(false);
	dlManager = nullptr;
	loadRepositories();
}

void CModListView::loadRepositories()
{
	manager->resetRepositories();
	for (auto entry : settings["launcher"]["repositoryURL"].Vector())
	{
		QString str = QString::fromUtf8(entry.String().c_str());

		// URL must be encoded to something else to get rid of symbols illegal in file names
		auto hashed = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5);
		auto hashedStr = QString::fromUtf8(hashed.toHex());

		downloadFile(hashedStr + ".json", str, "repository index");
	}
}

CModListView::~CModListView()
{
	delete ui;
}

void CModListView::showEvent(QShowEvent * event)
{
	QWidget::showEvent(event);
	if (repositoriesChanged)
	{
		repositoriesChanged = false;
		loadRepositories();
	}
}

void CModListView::showModInfo()
{
	ui->modInfoWidget->show();
	ui->hideModInfoButton->setArrowType(Qt::RightArrow);
}

void CModListView::hideModInfo()
{
	ui->modInfoWidget->hide();
	ui->hideModInfoButton->setArrowType(Qt::LeftArrow);
}

static QString replaceIfNotEmpty(QVariant value, QString pattern)
{
	if (value.canConvert<QStringList>())
		return pattern.arg(value.toStringList().join(", "));

	if (value.canConvert<QString>())
		return pattern.arg(value.toString());

	// all valid types of data should have been filtered by code above
	assert(!value.isValid());

	return "";
}

static QVariant sizeToString(QVariant value)
{
	if (value.canConvert<QString>())
	{
		static QString symbols = "kMGTPE";
		auto number = value.toUInt();
		size_t i=0;

		while (number >= 1000)
		{
			number /= 1000;
			i++;
		}
		return QVariant(QString("%1 %2B").arg(number).arg(symbols.at(i)));
	}
	return value;
}

static QString replaceIfNotEmpty(QStringList value, QString pattern)
{
	if (!value.empty())
		return pattern.arg(value.join(", "));
	return "";
}

QString CModListView::genModInfoText(CModEntry &mod)
{
	QString prefix = "<p><span style=\" font-weight:600;\">%1: </span>"; // shared prefix
	QString lineTemplate = prefix + "%2</p>";
	QString urlTemplate  = prefix + "<a href=\"%2\"><span style=\" text-decoration: underline; color:#0000ff;\">%2</span></a></p>";
	QString textTemplate = prefix + "</p><p align=\"justify\">%2</p>";
	QString noteTemplate = "<p align=\"justify\">%1: %2</p>";

	QString result;

	result += "<html><body>";
	result += replaceIfNotEmpty(mod.getValue("name"), lineTemplate.arg("Mod name"));
	result += replaceIfNotEmpty(mod.getValue("installedVersion"), lineTemplate.arg("Installed version"));
	result += replaceIfNotEmpty(mod.getValue("latestVersion"), lineTemplate.arg("Latest version"));
	result += replaceIfNotEmpty(sizeToString(mod.getValue("size")), lineTemplate.arg("Download size"));
	result += replaceIfNotEmpty(mod.getValue("author"), lineTemplate.arg("Authors"));
	result += replaceIfNotEmpty(mod.getValue("contact"), urlTemplate.arg("Home"));
	result += replaceIfNotEmpty(mod.getValue("depends"), lineTemplate.arg("Required mods"));
	result += replaceIfNotEmpty(mod.getValue("conflicts"), lineTemplate.arg("Conflicting mods"));
	result += replaceIfNotEmpty(mod.getValue("description"), textTemplate.arg("Description"));

	result += "<p></p>"; // to get some empty space

	QString unknownDeps  = "This mod can not be installed or enabled because following dependencies are not present";
	QString blockingMods = "This mod can not be enabled because following mods are incompatible with this mod";
	QString hasActiveDependentMods = "This mod can not be disabled because it is required to run following mods";
	QString hasDependentMods = "This mod can not be uninstalled or updated because it is required to run following mods";

	QString notes;

	notes += replaceIfNotEmpty(findInvalidDependencies(mod.getName()), noteTemplate.arg(unknownDeps));
	notes += replaceIfNotEmpty(findBlockingMods(mod.getName()), noteTemplate.arg(blockingMods));
	if (mod.isEnabled())
		notes += replaceIfNotEmpty(findDependentMods(mod.getName(), true), noteTemplate.arg(hasActiveDependentMods));
	if (mod.isInstalled())
		notes += replaceIfNotEmpty(findDependentMods(mod.getName(), false), noteTemplate.arg(hasDependentMods));

	if (notes.size())
		result += textTemplate.arg("Notes").arg(notes);

	result += "</body></html>";
	return result;
}

void CModListView::enableModInfo()
{
	showModInfo();
	ui->hideModInfoButton->setEnabled(true);
}

void CModListView::disableModInfo()
{
	hideModInfo();
	ui->hideModInfoButton->setEnabled(false);
}

void CModListView::selectMod(int index)
{
	if (index < 0)
	{
		disableModInfo();
	}
	else
	{
		enableModInfo();

		auto mod = modModel->getMod(modModel->modIndexToName(index));

		ui->textBrowser->setHtml(genModInfoText(mod));

		bool hasInvalidDeps = !findInvalidDependencies(modModel->modIndexToName(index)).empty();
		bool hasBlockingMods = !findBlockingMods(modModel->modIndexToName(index)).empty();
		bool hasDependentMods = !findDependentMods(modModel->modIndexToName(index), true).empty();

		ui->disableButton->setVisible(mod.isEnabled());
		ui->enableButton->setVisible(mod.isDisabled());
		ui->installButton->setVisible(mod.isAvailable());
		ui->uninstallButton->setVisible(mod.isInstalled());
		ui->updateButton->setVisible(mod.isUpdateable());

		// Block buttons if action is not allowed at this time
		// TODO: automate handling of some of these cases instead of forcing player
		// to resolve all conflicts manually.
		ui->disableButton->setEnabled(!hasDependentMods);
		ui->enableButton->setEnabled(!hasBlockingMods && !hasInvalidDeps);
		ui->installButton->setEnabled(!hasInvalidDeps);
		ui->uninstallButton->setEnabled(!hasDependentMods);
		ui->updateButton->setEnabled(!hasInvalidDeps && !hasDependentMods);
	}
}

void CModListView::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Escape && ui->modInfoWidget->isVisible() )
	{
		ui->modInfoWidget->hide();
	}
	else
	{
		return QWidget::keyPressEvent(event);
	}
}

void CModListView::modSelected(const QModelIndex & current, const QModelIndex & )
{
	selectMod(filterModel->mapToSource(current).row());
}

void CModListView::on_hideModInfoButton_clicked()
{
	if (ui->modInfoWidget->isVisible())
		hideModInfo();
	else
		showModInfo();
}

void CModListView::on_allModsView_doubleClicked(const QModelIndex &index)
{
	showModInfo();
	selectMod(filterModel->mapToSource(index).row());
}

void CModListView::on_lineEdit_textChanged(const QString &arg1)
{
	QRegExp regExp(arg1, Qt::CaseInsensitive, QRegExp::Wildcard);
	filterModel->setFilterRegExp(regExp);
}

void CModListView::on_comboBox_currentIndexChanged(int index)
{
	switch (index)
	{
	break; case 0: filterModel->setTypeFilter(ModStatus::MASK_NONE, ModStatus::MASK_NONE);
	break; case 1: filterModel->setTypeFilter(ModStatus::MASK_NONE, ModStatus::INSTALLED);
	break; case 2: filterModel->setTypeFilter(ModStatus::INSTALLED, ModStatus::INSTALLED);
	break; case 3: filterModel->setTypeFilter(ModStatus::UPDATEABLE, ModStatus::UPDATEABLE);
	break; case 4: filterModel->setTypeFilter(ModStatus::ENABLED | ModStatus::INSTALLED, ModStatus::ENABLED | ModStatus::INSTALLED);
	break; case 5: filterModel->setTypeFilter(ModStatus::INSTALLED, ModStatus::ENABLED | ModStatus::INSTALLED);
	}
}

QStringList CModListView::findInvalidDependencies(QString mod)
{
	QStringList ret;
	for (QString requrement : modModel->getRequirements(mod))
	{
		if (!modModel->hasMod(requrement))
			ret += requrement;
	}
	return ret;
}

QStringList CModListView::findBlockingMods(QString mod)
{
	QStringList ret;
	auto required = modModel->getRequirements(mod);

	for (QString name : modModel->getModList())
	{
		auto mod = modModel->getMod(name);

		if (mod.isEnabled())
		{
			// one of enabled mods have requirement (or this mod) marked as conflict
			for (auto conflict : mod.getValue("conflicts").toStringList())
				if (required.contains(conflict))
					ret.push_back(name);
		}
	}

	return ret;
}

QStringList CModListView::findDependentMods(QString mod, bool excludeDisabled)
{
	QStringList ret;
	for (QString modName : modModel->getModList())
	{
		auto current = modModel->getMod(modName);

		if (!current.isInstalled())
			continue;

		if (current.getValue("depends").toStringList().contains(mod) &&
		    !(current.isDisabled() && excludeDisabled))
			ret += modName;
	}
	return ret;
}

void CModListView::on_enableButton_clicked()
{
	QString modName = modModel->modIndexToName(filterModel->mapToSource(ui->allModsView->currentIndex()).row());

	assert(findBlockingMods(modName).empty());
	assert(findInvalidDependencies(modName).empty());

	for (auto & name : modModel->getRequirements(modName))
		if (modModel->getMod(name).isDisabled())
			manager->enableMod(name);
	checkManagerErrors();
}

void CModListView::on_disableButton_clicked()
{
	QString modName = modModel->modIndexToName(filterModel->mapToSource(ui->allModsView->currentIndex()).row());

	if (modModel->hasMod(modName) &&
	    modModel->getMod(modName).isEnabled())
			manager->disableMod(modName);

	checkManagerErrors();
}

void CModListView::on_updateButton_clicked()
{
	QString modName = modModel->modIndexToName(filterModel->mapToSource(ui->allModsView->currentIndex()).row());

	assert(findInvalidDependencies(modName).empty());

	for (auto & name : modModel->getRequirements(modName))
	{
		auto mod = modModel->getMod(name);
		// update required mod, install missing (can be new dependency)
		if (mod.isUpdateable() || !mod.isInstalled())
			downloadFile(name + ".zip", mod.getValue("download").toString(), "mods");
	}
}

void CModListView::on_uninstallButton_clicked()
{
	QString modName = modModel->modIndexToName(filterModel->mapToSource(ui->allModsView->currentIndex()).row());
	// NOTE: perhaps add "manually installed" flag and uninstall those dependencies that don't have it?

	if (modModel->hasMod(modName) &&
	    modModel->getMod(modName).isInstalled())
	{
		if (modModel->getMod(modName).isEnabled())
			manager->disableMod(modName);
		manager->uninstallMod(modName);
	}
	checkManagerErrors();
}

void CModListView::on_installButton_clicked()
{
	QString modName = modModel->modIndexToName(filterModel->mapToSource(ui->allModsView->currentIndex()).row());

	assert(findInvalidDependencies(modName).empty());

	for (auto & name : modModel->getRequirements(modName))
	{
		auto mod = modModel->getMod(name);
		if (!mod.isInstalled())
			downloadFile(name + ".zip", mod.getValue("download").toString(), "mods");
	}
}

void CModListView::downloadFile(QString file, QString url, QString description)
{
	if (!dlManager)
	{
		dlManager = new CDownloadManager();
		ui->progressWidget->setVisible(true);
		connect(dlManager, SIGNAL(downloadProgress(qint64,qint64)),
				this, SLOT(downloadProgress(qint64,qint64)));

		connect(dlManager, SIGNAL(finished(QStringList,QStringList,QStringList)),
				this, SLOT(downloadFinished(QStringList,QStringList,QStringList)));


		QString progressBarFormat = "Downloading %s%. %p% (%v KB out of %m KB) finished";

		progressBarFormat.replace("%s%", description);
		ui->progressBar->setFormat(progressBarFormat);
	}

	dlManager->downloadFile(QUrl(url), file);
}

void CModListView::downloadProgress(qint64 current, qint64 max)
{
	// display progress, in kilobytes
	ui->progressBar->setValue(current/1024);
	ui->progressBar->setMaximum(max/1024);
}

void CModListView::downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors)
{
	QString title = "Download failed";
	QString firstLine = "Unable to download all files.\n\nEncountered errors:\n\n";
	QString lastLine = "\n\nInstall successfully downloaded?";

	// if all files were d/loaded there should be no errors. And on failure there must be an error
	assert(failedFiles.empty() == errors.empty());

	if (savedFiles.empty())
	{
		// no successfully downloaded mods
		QMessageBox::warning(this, title, firstLine + errors.join("\n"), QMessageBox::Ok, QMessageBox::Ok );
	}
	else if (!failedFiles.empty())
	{
		// some mods were not downloaded
		int result = QMessageBox::warning (this, title, firstLine + errors.join("\n") + lastLine,
		                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

		if (result == QMessageBox::Yes)
			installFiles(savedFiles);
	}
	else
	{
		// everything OK
		installFiles(savedFiles);
	}

	// remove progress bar after some delay so user can see that download was complete and not interrupted.
	QTimer::singleShot(1000, this,  SLOT(hideProgressBar()));

	dlManager->deleteLater();
	dlManager = nullptr;
}

void CModListView::hideProgressBar()
{
	if (dlManager == nullptr) // it was not recreated meanwhile
	{
		ui->progressWidget->setVisible(false);
		ui->progressBar->setMaximum(0);
		ui->progressBar->setValue(0);
	}
}

void CModListView::installFiles(QStringList files)
{
	QStringList mods;

	// TODO: some better way to separate zip's with mods and downloaded repository files
	for (QString filename : files)
	{
		if (filename.contains(".zip"))
			mods.push_back(filename);
		if (filename.contains(".json"))
			manager->loadRepository(filename);
	}
	if (!mods.empty())
		installMods(mods);
}

void CModListView::installMods(QStringList archives)
{
	QStringList modNames;

	for (QString archive : archives)
	{
		// get basename out of full file name
		//                remove path                  remove extension
		QString modName = archive.section('/', -1, -1).section('.', 0, 0);

		modNames.push_back(modName);
	}

	QStringList modsToEnable;

	// disable mod(s), to properly recalculate dependencies, if changed
	for (QString mod : boost::adaptors::reverse(modNames))
	{
		CModEntry entry = modModel->getMod(mod);
		if (entry.isInstalled())
		{
			// enable mod if installed and enabled
			if (entry.isEnabled())
				modsToEnable.push_back(mod);
		}
		else
		{
			// enable mod if m
			if (settings["launcher"]["enableInstalledMods"].Bool())
				modsToEnable.push_back(mod);
		}
	}

	// uninstall old version of mod, if installed
	for (QString mod : boost::adaptors::reverse(modNames))
	{
		if (modModel->getMod(mod).isInstalled())
			manager->uninstallMod(mod);
	}

	for (int i=0; i<modNames.size(); i++)
		manager->installMod(modNames[i], archives[i]);

	if (settings["launcher"]["enableInstalledMods"].Bool())
	{
		for (QString mod : modNames)
			manager->enableMod(mod);
	}

	for (QString archive : archives)
		QFile::remove(archive);

	checkManagerErrors();
}

void CModListView::on_pushButton_clicked()
{
	delete dlManager;
	dlManager = nullptr;
	hideProgressBar();
}

void CModListView::modelReset()
{
	//selectMod(filterModel->mapToSource(ui->allModsView->currentIndex()).row());
	selectMod(filterModel->rowCount() > 0 ? 0 : -1);
}

void CModListView::checkManagerErrors()
{
	QString errors = manager->getErrors().join('\n');
	if (errors.size() != 0)
	{
		QString title = "Operation failed";
		QString description = "Encountered errors:\n" + errors;
		QMessageBox::warning(this, title, description, QMessageBox::Ok, QMessageBox::Ok );
	}
}
