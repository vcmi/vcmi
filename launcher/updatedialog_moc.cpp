/*
 * updatedialog_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "updatedialog_moc.h"
#include "ui_updatedialog_moc.h"

#include "../lib/CConfigHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/VCMIDirs.h"
#include "helper.h"
#include "mainwindow_moc.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include <QSysInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QDir>
#include <QProgressBar>
#include <QVersionNumber>
#include <QRegularExpression>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QPainter>
#include <QTimer>
#include <QScreen>
#include <QProcessEnvironment>
#include <QFileDialog>

#ifdef VCMI_IOS
#include "iOS_utils.h"
#endif

// Helper to normalize channel key to stable/beta/develop
static QString normalizeChannel(const QString& text)
{
	const auto str = text.trimmed().toLower();
	if(str.contains("beta"))
		return "beta";

	if(str.contains("develop"))
		return "develop";

	return "stable";
}

static QString preferredTestingChannelFromBranch(const std::string &branchName)
{
	const QString normalizedBranch = normalizeChannel(QString::fromStdString(branchName));
	if(normalizedBranch == "beta" || normalizedBranch == "develop")
		return normalizedBranch;

	return {};
}

static QString actionButtonTextForPlatform()
{
#if defined(VCMI_ANDROID)
	if(!Helper::isInstalledFromGooglePlay())
		return QObject::tr("Download");

	return QObject::tr("Install");
#elif defined(VCMI_MAC)
	return QObject::tr("Download");
#elif defined(VCMI_IOS)
	return QObject::tr("Download");
#else
	return QObject::tr("Install");
#endif
}

static QString updateDialogPlatformInfo()
{
#if defined(VCMI_WINDOWS)
	return QObject::tr("You are running Windows. Release, Beta and Develop can coexist, but they share the VCMI data directory unless you configure separate custom paths in dirs.json.");
#elif defined(VCMI_ANDROID)
	return QObject::tr("You are running Android. Release and Beta/Develop can be installed, but Beta/Develop builds overwrite each other as VCMI Daily. Release and Beta/Develop do not share the VCMI data directory.");
#elif defined(VCMI_MAC)
	return QObject::tr("You are running macOS. Multiple VCMI versions can be installed side by side. Depending on how they are launched, versions may still use shared user data.");
#elif defined(VCMI_IOS)
	return QObject::tr("You are running iOS. Usually only one VCMI app installation is active at a time, and installing another build typically replaces the previous app.");
#elif defined(VCMI_UNIX)
	return QObject::tr("You are running Linux. Updates are currently supported only for AppImage builds.");
#else
	return QObject::tr("You are running an unsupported or unknown operating system. Platform-specific coexistence details are currently unavailable.");
#endif
}

static QString availabilityLine(int comparisonResult, const QString &version)
{
	if(comparisonResult > 0)
	{
		const QString label = !version.isEmpty() ? QObject::tr("New version %1 available").arg(version) : QObject::tr("New version available");
		return QString("<span style=\"color:#1F9D55;font-weight:600;\">%1</span>").arg(label.toHtmlEscaped());
	}

	if(comparisonResult < 0)
		return QString("<span style=\"color:#C53030;font-weight:600;\">%1</span>").arg(QObject::tr("Selected version is older than installed one").toHtmlEscaped());

	return QObject::tr("You are using latest version");
}

static QString noDownloadHelpText()
{
	return QObject::tr("Automatic updates are not supported for this platform/channel combination. Please install the updated version manually.");
}

static QString appendNoDownloadHelpText(const QString &existingMarkdown)
{
	if(existingMarkdown.trimmed().isEmpty())
		return noDownloadHelpText();

	return existingMarkdown + "\n\n" + noDownloadHelpText();
}


UpdateDialog::UpdateDialog(bool calledManually, QWidget *parent):
	QDialog(parent),
	ui(new Ui::UpdateDialog),
	calledManually(calledManually)
{
	ui->setupUi(this);
	ui->buildChannel->setItemData(0, "develop");
	ui->buildChannel->setItemData(1, "beta");

	ui->releaseChangelog->setReadOnly(true);
	ui->releaseChangelog->setOpenExternalLinks(true);
	Helper::enableScrollBySwiping(ui->releaseChangelog);

	ui->testingChangelog->setReadOnly(true);
	ui->testingChangelog->setOpenExternalLinks(true);
	Helper::enableScrollBySwiping(ui->testingChangelog);

#if defined(VCMI_MOBILE)
	const QSize originalDialogSize = size();
	setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
	setAutoFillBackground(false);
	setStyleSheet("QWidget#contentPanel { background: palette(window); border: 2px solid rgba(0,0,0,160); border-radius: 6px; }");

	if(ui->contentPanel && ui->hostLayout)
	{
		QSize panelSize = originalDialogSize;
		if(const QScreen * screen = QGuiApplication::primaryScreen())
			panelSize = panelSize.boundedTo(screen->availableGeometry().size() - QSize(24, 24));

		ui->contentPanel->setMinimumSize(panelSize);
		ui->contentPanel->setMaximumSize(panelSize);
		ui->hostLayout->setAlignment(ui->contentPanel, Qt::AlignCenter);
	}

	updateMobileHostGeometry();
	updateMobileBackdrop();
#else
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
#endif

	ui->progressBar->setHidden(true);
	ui->installButton->setText(actionButtonTextForPlatform());
	if(ui->infoButton)
	{
		ui->infoButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
		ui->infoButton->setText(tr("?"));
		QFont infoFont = ui->infoButton->font();
		infoFont.setBold(true);
		ui->infoButton->setFont(infoFont);
		ui->infoButton->setCursor(Qt::PointingHandCursor);
		ui->infoButton->setStyleSheet("QToolButton#infoButton { border: 1px solid palette(mid); border-radius: 14px; padding: 3px; background: palette(button); font-weight: 700; } QToolButton#infoButton:hover { background: palette(light); }");
		ui->infoButton->raise();
	}

	if(calledManually)
	{
#if defined(VCMI_MOBILE)
		setWindowModality(Qt::NonModal);
		ensureMobileBackdropCaptured();
		updateMobileHostGeometry();
#else
		setWindowModality(Qt::ApplicationModal);
#endif
		show();
	}

	if(settings["launcher"]["updateOnStartup"].Bool())
		ui->checkOnStartup->setCheckState(Qt::CheckState::Checked);

	if(settings["launcher"]["testingBuilds"].Bool())
		ui->testingBuilds->setCheckState(Qt::CheckState::Checked);

	currentVersion = GameConstants::VCMI_VERSION;
	currentCommit = GameConstants::VCMI_COMMIT;
	currentBranch = GameConstants::VCMI_BRANCH;

	setWindowTitle(tr("VCMI Updates Center"));
	ui->title->setText(tr("VCMI Updates Center"));

	// Testing builds info
	if(ui->testingBuilds->isChecked())
	{
		fetchChannel("beta");
		fetchChannel("develop");
		ui->tabWidget->setCurrentIndex(1);
	}

	fetchChannel("stable");
}

UpdateDialog::~UpdateDialog()
{
	delete ui;
}

void UpdateDialog::resizeEvent(QResizeEvent * event)
{
	QDialog::resizeEvent(event);
#if defined(VCMI_MOBILE)
	updateMobileBackdrop();
#endif
}

void UpdateDialog::updateMobileHostGeometry()
{
	if(QWidget * mainWindow = Helper::getMainWindow())
	{
		setGeometry(mainWindow->geometry());
		return;
	}

	if(const QScreen * screen = QGuiApplication::primaryScreen())
		setGeometry(screen->availableGeometry());
}

void UpdateDialog::ensureMobileBackdropCaptured()
{
	if(!mobileBackdrop.isNull())
		return;

	if(QWidget * mainWindow = Helper::getMainWindow())
		mobileBackdrop = mainWindow->grab();
}

void UpdateDialog::updateMobileBackdrop()
{
	if(!mobileBackdrop.isNull() || mobileBackdropCaptureScheduled || !isVisible())
		return;

	mobileBackdropCaptureScheduled = true;
	QTimer::singleShot(0, this, [this]()
	{
		mobileBackdropCaptureScheduled = false;
		if(!isVisible() || !mobileBackdrop.isNull())
			return;

		ensureMobileBackdropCaptured();
		if(!mobileBackdrop.isNull())
			update();
	});
}

void UpdateDialog::paintEvent(QPaintEvent * event)
{
#if defined(VCMI_MOBILE)
	Q_UNUSED(event);
	if(!mobileBackdrop.isNull())
	{
		QPainter painter(this);
		painter.drawPixmap(QPoint(0, 0), mobileBackdrop);
	}
	return;
#else
	QDialog::paintEvent(event);
#endif
}

void UpdateDialog::showEvent(QShowEvent * event)
{
	QDialog::showEvent(event);
#if defined(VCMI_MOBILE)
	updateMobileBackdrop();
#endif
}

void UpdateDialog::showUpdateDialog(bool isManually)
{
	auto * dialog = new UpdateDialog(isManually, Helper::getMainWindow());
	dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void UpdateDialog::on_tabWidget_currentChanged(int index)
{
	Q_UNUSED(index);
	updateAvailabilityNotice();
}

void UpdateDialog::on_checkOnStartup_stateChanged(int state)
{
	Q_UNUSED(state);
	Settings node = settings.write["launcher"]["updateOnStartup"];
	node->Bool() = ui->checkOnStartup->isChecked();
}

void UpdateDialog::on_testingBuilds_stateChanged(int state)
{
	Q_UNUSED(state);
	bool testing = ui->testingBuilds->isChecked();

	Settings node = settings.write["launcher"]["testingBuilds"];
	node->Bool() = testing;

	QLabel* versionLabel = testing ? ui->releaseVersion : ui->testingVersion;
	QTextBrowser* changelogBox = testing ? ui->releaseChangelog : ui->testingChangelog;

	// Additionally load the selected testing channel if enabled
	if(testing)
	{
		testingChannelAutoSelectPending = true;
		fetchChannel("beta");
		fetchChannel("develop");

		ui->buildChannel->setEnabled(true);
		ui->titleTesting->setEnabled(true);
		ui->testingChangelogTitle->setEnabled(true);
		//changelogBox->setEnabled(true);
	}
	else
	{
		ui->buildChannel->setDisabled(true);
		ui->titleTesting->setDisabled(true);
		ui->testingChangelogTitle->setDisabled(true);
		versionLabel->setText("");
		changelogBox->setMarkdown("");
		testingVersion.clear();
		testingUrl.clear();
		selectedTestingCommit.clear();
		selectedTestingBuildDate.clear();
		selectedTestingChannel.clear();
		testingOffer = false;
		testingChannelAutoSelectPending = true;
		if(ui->tabWidget)
			ui->tabWidget->setCurrentIndex(0);
		fetchChannel("stable");
		updateAvailabilityNotice();
	}
}

void UpdateDialog::on_testingBuilds_clicked(bool checked)
{
	if(!checked)
		return;

	QMessageBox::warning(this, tr("Testing versions warning"), tr("Be careful with installing Beta / Develop versions, they may be unstable."));
}

// Build filename for the selected update channel.
static QString filenameForChannel(const QString& channel)
{
	const QString name = channel.trimmed().toLower();

	if(name == "stable")
		return "vcmi-stable.json";

	if(name == "beta")
		return "vcmi-beta.json";

	return "vcmi-develop.json"; // default
}

void UpdateDialog::on_buildChannel_currentIndexChanged(int)
{
	// Only react when testing builds are enabled
	if(!ui->testingBuilds->isChecked())
		return;

	applySelectedTestingChannel();
}

// Map runtime OS/arch to JSON "download" key, e.g. "windows-x64"
static QString platformKeyFromRuntime()
{
#if defined(VCMI_WINDOWS)
	const auto arch = QSysInfo::buildCpuArchitecture(); // "x86_64","i386","arm64"
	if(arch == "x86_64")
		return "windows-x64";
	if(arch == "i386" || arch == "i686")
		return "windows-x86";
	if(arch == "arm64" || arch == "aarch64")
		return "windows-arm64";
	logGlobal->warn("Unknown Windows architecture '%s', falling back to windows-x64", arch.toStdString());
	return "windows-x64";

#elif defined(VCMI_MAC)
	const auto arch = QSysInfo::buildCpuArchitecture();
	if(arch == "arm64" || arch == "aarch64")
		return "macos-arm";
	if(arch == "x86_64")
		return "macos-intel";
	logGlobal->warn("Unknown macOS architecture '%s', falling back to macos-intel", arch.toStdString());
	return "macos-intel";

#elif defined(VCMI_ANDROID)
	const auto arch = QSysInfo::buildCpuArchitecture(); // "x86_64", "arm64-v8a","armeabi-v7a"
	if(arch == "x86_64")
		return "android-x64";
	if(arch == "arm64-v8a" || arch == "arm64" || arch == "aarch64")
		return "android-arm64-v8a";
	if(arch == "armeabi-v7a" || arch == "armv7" || arch == "arm")
		return "android-armeabi-v7a";
	logGlobal->warn("Unknown Android architecture '%s', falling back to android-arm64-v8a", arch.toStdString());
	return "android-arm64-v8a";

#elif defined(VCMI_IOS)
	return "ios-ios";

#elif defined(VCMI_UNIX)
	const auto arch = QSysInfo::buildCpuArchitecture();

	bool isAppImage = !qgetenv("APPIMAGE").isEmpty();
	if(!isAppImage)
	{
		logGlobal->warn("Auto Update on Linux is currently supported only for AppImage builds. Detected non-AppImage environment.");
		return {};
	}

	if(arch == "x86_64")
		return "linux-x86_64";

	if(arch == "arm64" || arch == "aarch64")
		return "linux-arm64";

	logGlobal->warn("Unknown Linux architecture '%s', falling back to linux-x86_64", arch.toStdString());
	return "linux-x86_64";

#else
	return {};
#endif
}

static QVersionNumber toVersion(QString version)
{
	// TODO: VCMI_VERSION vs VCMI_VERSION_STRING
	const QVersionNumber direct = QVersionNumber::fromString(version);
	if(!direct.isNull())
		return direct;

	static const QRegularExpression versionPattern(QStringLiteral(R"((\d+(?:\.\d+)+))"));
	const QRegularExpressionMatch match = versionPattern.match(version);
	if(!match.hasMatch())
		return {};

	return QVersionNumber::fromString(match.captured(1));
}

inline int cmpSemver(QString a, QString b)
{
	const QVersionNumber verA = toVersion(a);
	const QVersionNumber verB = toVersion(b);
	if(verA.isNull() && verB.isNull())
		return 0;
	if(verA.isNull())
		return -1;
	if(verB.isNull())
		return 1;

	// normalize - remove trailing zeros, e.g. 1.2.0 -> 1.2
	return QVersionNumber::compare(verA.normalized(), verB.normalized());
}

// Join base URL (may or may not end with /) with filename.
static QUrl joinBaseAndFile(const QString& base, const QString& file)
{
	QString b = base.trimmed();
	if (!b.endsWith('/'))
		b.append('/');
	return QUrl(b + file);
}

// Pick best download URL from "download" object
static QString pickDownloadUrl(const JsonNode &node)
{
    const auto platform = platformKeyFromRuntime().toStdString();
    if(node["download"][platform].getType() == JsonNode::JsonType::DATA_STRING)
        return QString::fromStdString(node["download"][platform].String());

    // last resort: first string in "download"
    for(const auto &kv : node["download"].Struct())
        if(kv.second.getType() == JsonNode::JsonType::DATA_STRING)
            return QString::fromStdString(kv.second.String());
    return {};
}

// Return first 7 characters of a commit-ish; gracefully handles empty/short strings.
static std::string commitShort(const std::string &str)
{
    if(str.size() <= 7)
		return str;

    return str.substr(0, 7);
}

static int compareWithInstalled(const std::string &currentVersion, const std::string &currentCommit, const QString &candidateVersion, const QString &candidateCommit, bool compareCommit)
{
	if(candidateVersion.isEmpty())
		return 0;

	const int versionCmp = cmpSemver(candidateVersion, QString::fromStdString(currentVersion));
	if(versionCmp != 0)
		return versionCmp;

	if(!compareCommit)
		return 0;

	const std::string currentSha = commitShort(currentCommit);
	const std::string candidateSha = commitShort(candidateCommit.toStdString());
	if(currentSha.empty() || candidateSha.empty() || currentSha == candidateSha)
		return 0;

	// For testing channels, any different commit with same version means different (newer) build candidate.
	return 1;
}

static int compareCandidateBuilds(const QString &leftVersion, const QString &leftBuildDate, const QString &rightVersion, const QString &rightBuildDate)
{
	const int versionCmp = cmpSemver(leftVersion, rightVersion);
	if(versionCmp != 0)
		return versionCmp;

	if(!leftBuildDate.isEmpty() && !rightBuildDate.isEmpty())
	{
		if(leftBuildDate > rightBuildDate)
			return 1;
		if(leftBuildDate < rightBuildDate)
			return -1;
	}

	return 0;
}


void UpdateDialog::fetchChannel(const QString& channel)
{
	const QString normalizedChannel = normalizeChannel(channel);
	const bool isTesting = (normalizedChannel != "stable"); // beta/develop -> testing area

	const QString base = QString::fromStdString(settings["launcher"]["updateConfigUrl"].String());
	const QUrl url = joinBaseAndFile(base, filenameForChannel(normalizedChannel));

	QNetworkReply* response = networkManager.get(QNetworkRequest(url));

	connect(response, &QNetworkReply::finished, [this, response, isTesting, normalizedChannel] {
		response->deleteLater();

		if(response->error() != QNetworkReply::NoError)
		{
			(isTesting ? ui->testingChangelog : ui->releaseChangelog)->setMarkdown(tr("Network error: %1").arg(response->errorString()));
			return;
		}

		const auto bytes = response->readAll();
		JsonNode node(reinterpret_cast<const std::byte*>(bytes.constData()), bytes.size(), "<network packet from update url>");
		loadFromJson(node, isTesting, normalizedChannel);
		}
	);
}

void UpdateDialog::loadFromJson(const JsonNode& node, bool testing, const QString& channel)
{
	// Validate schema
	if(node.getType() != JsonNode::JsonType::DATA_STRUCT || node["version"].getType() != JsonNode::JsonType::DATA_STRING || node["download"].getType() != JsonNode::JsonType::DATA_STRUCT)
	{
		//(testing ? ui->testingChangelog : ui->releaseChangelog)->setPlainText(tr("Invalid update JSON (missing 'version' or 'download')."));
		return;
	}

	// Choose target widgets based on 'testing'
	QLabel* versionLabel = testing ? ui->testingVersion : ui->releaseVersion;
	QTextBrowser* changelogBox = testing ? ui->testingChangelog : ui->releaseChangelog;
	QString &downloadURL = testing ? testingUrl : releaseUrl;
	QString &version = testing ? testingVersion : releaseVersion;

	const std::string newVersion = node["version"].String();
	const std::string newCommit = node["commit"].getType() == JsonNode::JsonType::DATA_STRING ? node["commit"].String() : "";
	const std::string buildDate = node["buildDate"].getType() == JsonNode::JsonType::DATA_STRING ? node["buildDate"].String() : "";
	const std::string changeLog = node["changeLog"].getType() == JsonNode::JsonType::DATA_STRING ? node["changeLog"].String() : "";

	// Decide if update is offered, but never early-return or close the dialog
	//const bool offer = compareWithInstalled(currentVersion, currentCommit, QString::fromStdString(newVersion), QString::fromStdString(newCommit), testing) > 0;

	// Populate UI
	if(versionLabel)
		versionLabel->setText(QString::fromStdString(newVersion));

	// Build the header first (Build + Commit), then an empty line, then the changelog body.
	QStringList headerLines;
	if(!buildDate.empty())
		headerLines << tr("Build date: %1").arg(QString::fromStdString(buildDate));
	if(!newCommit.empty())
		headerLines << tr("Commit: %1").arg(QString::fromStdString(commitShort(newCommit)));

	const QString body = QString::fromStdString(changeLog);

	QString logText;
	if(!headerLines.isEmpty())
		logText = headerLines.join("\n\n");

	logText += "<br/><br/>"; // blank line between header and body
	logText += body;

	if(changelogBox)
		changelogBox->setMarkdown(logText);

	// Download link
	const QString link = pickDownloadUrl(node);
	const bool offer = !link.isEmpty() && compareWithInstalled(currentVersion, currentCommit, QString::fromStdString(newVersion), QString::fromStdString(newCommit), testing) > 0;
	
	downloadURL = link;
	version = QString::fromStdString(newVersion);

	if(link.isEmpty())
		changelogBox->setMarkdown(appendNoDownloadHelpText(logText));

	if(testing)
	{
		const QString normalizedChannel = normalizeChannel(channel);
		TestingBuildState &targetState = normalizedChannel == "beta" ? betaState : developState;
		targetState.channel = normalizedChannel;
		targetState.version = QString::fromStdString(newVersion);
		targetState.commit = QString::fromStdString(newCommit);
		targetState.buildDate = QString::fromStdString(buildDate);
		targetState.downloadUrl = link;
		targetState.changelog = body;
		targetState.valid = true;

		refreshTestingBuildFromNewest();
	}
	else
	{
		releaseBuildDate = QString::fromStdString(buildDate);
		releaseOffer = offer;
		updateAvailabilityNotice();
	}

	const bool hasTestingOffer = ui->testingBuilds->isChecked() && testingOffer;
	if((releaseOffer || hasTestingOffer) && !calledManually)
	{
#if defined(VCMI_MOBILE)
		ensureMobileBackdropCaptured();
		updateMobileHostGeometry();
#endif
		this->show();
		this->raise();
		this->activateWindow();

		int recommendedTab = 0;
		if(hasTestingOffer && !releaseOffer)
			recommendedTab = 1;
		else if(hasTestingOffer && releaseOffer)
		{
			const int candidateCmp = compareCandidateBuilds(testingVersion, selectedTestingBuildDate, releaseVersion, releaseBuildDate);
			recommendedTab = candidateCmp > 0 ? 1 : 0;
		}
		ui->tabWidget->setCurrentIndex(recommendedTab);
	}
}

void UpdateDialog::refreshTestingBuildFromNewest()
{
	if(!ui->testingBuilds->isChecked())
		return;

	const TestingBuildState *newest = nullptr;
	if(betaState.valid && !developState.valid)
		newest = &betaState;
	else if(developState.valid && !betaState.valid)
		newest = &developState;
	else if(betaState.valid && developState.valid)
	{
		const int candidateCmp = compareCandidateBuilds(betaState.version, betaState.buildDate, developState.version, developState.buildDate);
		newest = candidateCmp >= 0 ? &betaState : &developState;
	}

	if(!newest)
	{
		testingVersion.clear();
		testingUrl.clear();
		selectedTestingCommit.clear();
		selectedTestingBuildDate.clear();
		selectedTestingChannel.clear();
		testingOffer = false;
		if(ui->testingVersion)
			ui->testingVersion->clear();
		if(ui->testingChangelog)
			ui->testingChangelog->setMarkdown(noDownloadHelpText());
		updateAvailabilityNotice();
		return;
	}

	const QString preferredTestingChannel = preferredTestingChannelFromBranch(currentBranch);
	const TestingBuildState *autoSelected = nullptr;
#if defined(VCMI_ANDROID)
	autoSelected = newest;
#else
	if(preferredTestingChannel == "beta" && betaState.valid)
		autoSelected = &betaState;
	else if(preferredTestingChannel == "develop" && developState.valid)
		autoSelected = &developState;
	else
		autoSelected = newest;
#endif

	if(testingChannelAutoSelectPending && autoSelected)
	{
		const int selectedIndex = ui->buildChannel->findData(autoSelected->channel);
		if(selectedIndex >= 0 && selectedIndex != ui->buildChannel->currentIndex())
			ui->buildChannel->setCurrentIndex(selectedIndex);
		testingChannelAutoSelectPending = false;
	}

	applySelectedTestingChannel();
}

void UpdateDialog::applySelectedTestingChannel()
{
	if(!ui->testingBuilds->isChecked())
		return;

	const QString selectedChannel = ui->buildChannel ? normalizeChannel(ui->buildChannel->currentData().toString()) : QString("develop");
	const TestingBuildState *selected = nullptr;
	if(selectedChannel == "beta" && betaState.valid)
		selected = &betaState;
	else if(selectedChannel == "develop" && developState.valid)
		selected = &developState;
	else if(developState.valid)
		selected = &developState;
	else if(betaState.valid)
		selected = &betaState;

	if(!selected)
		return;

	testingVersion = selected->version;
	testingUrl = selected->downloadUrl;
	selectedTestingCommit = selected->commit;
	selectedTestingBuildDate = selected->buildDate;
	selectedTestingChannel = selected->channel;

	if(ui->testingVersion)
		ui->testingVersion->setText(selected->version);

	QStringList headerLines;
	if(!selected->buildDate.isEmpty())
		headerLines << tr("Build date: %1").arg(selected->buildDate);
	if(!selected->commit.isEmpty())
		headerLines << tr("Commit: %1").arg(QString::fromStdString(commitShort(selected->commit.toStdString())));

	QString logText;
	if(!headerLines.isEmpty())
		logText = headerLines.join("\n\n");

	if(ui->testingChangelog)
	{
		if(selected->downloadUrl.isEmpty())
			ui->testingChangelog->setMarkdown(appendNoDownloadHelpText(logText));
		else
		{
			logText += "<br/><br/>";
			logText += selected->changelog;
			ui->testingChangelog->setMarkdown(logText);
		}
	}

	testingOffer = !testingUrl.isEmpty() && compareWithInstalled(currentVersion, currentCommit, testingVersion, selected->commit, true) > 0;
	updateAvailabilityNotice();

	testingOffer = compareWithInstalled(currentVersion, currentCommit, testingVersion, selected->commit, true) > 0;
	updateAvailabilityNotice();
}

void UpdateDialog::updateAvailabilityNotice()
{
	const bool testingTabSelected = ui->tabWidget->currentIndex() == 1 && ui->testingBuilds->isChecked();
	const bool selectedTesting = testingTabSelected && !testingVersion.isEmpty() && !testingUrl.isEmpty();

	QString version = selectedTesting ? testingVersion : releaseVersion;
	if(!version.isEmpty())
	{
		if(selectedTesting && !selectedTestingChannel.isEmpty())
			version += tr(" (%1)").arg(selectedTestingChannel);
		else
			version += tr(" (Release)");
	}

	const int comparisonResult = selectedTesting ? compareWithInstalled(currentVersion, currentCommit, testingVersion, selectedTestingCommit, true) : compareWithInstalled(currentVersion, currentCommit, releaseVersion, QString(), false);

	ui->downloadLink->setText(availabilityLine(comparisonResult, version));
}


void UpdateDialog::on_infoButton_clicked()
{
	const QString common = tr("On other operating systems, update channel installation and coexistence behavior may be different.");
	const QString details = updateDialogPlatformInfo();
	QMessageBox::information(this, tr("Update channels information"), details + "\n\n" + common);
}

bool UpdateDialog::handleIosInstallFlow()
{
#if defined(VCMI_IOS)
	if(iOS_utils::isTestFlightInstalled() || iOS_utils::isOsVersionAtLeast(16))
	{
		QDesktopServices::openUrl(QUrl(QStringLiteral("https://testflight.apple.com/join/pJWHSbmu")));
		return true;
	}

	QMessageBox messageBox(this);
	messageBox.setIcon(QMessageBox::Information);
	messageBox.setWindowTitle(tr("iOS installation"));
	messageBox.setText(tr("For iOS versions older than 16, install VCMI via AltStore."));
	messageBox.setInformativeText(tr("Open the iOS installation guide now?"));
	QPushButton * openGuideButton = messageBox.addButton(tr("Open guide"), QMessageBox::AcceptRole);
	messageBox.addButton(QMessageBox::Cancel);
	messageBox.exec();

	if(messageBox.clickedButton() == openGuideButton)
		QDesktopServices::openUrl(QUrl(QStringLiteral("https://vcmi.eu/players/Installation_iOS/")));

	return true;
#else
	return false;
#endif
}

bool UpdateDialog::selectedChannelIsTesting() const
{
	return ui->tabWidget->currentIndex() == 1 && ui->testingBuilds->isChecked();
}

QString UpdateDialog::selectedChannelDownloadUrl()
{
	if(selectedChannelIsTesting())
	{
		applySelectedTestingChannel(); // keep URL in sync with current dropdown choice
		return testingUrl;
	}

	return releaseUrl;
}

void UpdateDialog::startSelectedDownload(const QString &url)
{
	const QUrl parsedUrl(url);

#if defined(VCMI_MOBILE)
	// Always ask user where to save on mobile
	if(Helper::canUseFolderPicker())
	{
		Helper::nativeFolderPicker(this, [this, parsedUrl](QString picked){
			if(picked.isEmpty())
				return; // user cancelled
			startDownloadToCacheAndRun(parsedUrl, picked);
		});
	}
	else
	{
		QMessageBox::information(this, tr("Manual filename required"), tr("This fallback picker may not prefill a filename on some devices. If no filename is shown, create one manually (for example: vcmi.apk)."));
		const QString pickedPath = QFileDialog::getOpenFileName(this, tr("Select destination file"), QDir::homePath(), tr("All files (*.*)"));
		if(pickedPath.isEmpty())
			return;

		startDownloadToCacheAndRun(parsedUrl, pickedPath, true);
	}
#else
	startDownloadToCacheAndRun(parsedUrl);
#endif
}

void UpdateDialog::on_installButton_clicked()
{
	if(handleIosInstallFlow())
		return;

	const QString url = selectedChannelDownloadUrl();

	if(url.isEmpty())
	{
		ui->downloadLink->setText(noDownloadHelpText());
		return;
	}

#if defined(VCMI_ANDROID)
	if(Helper::isInstalledFromGooglePlay())
	{
		const QUrl playStoreUrl(QStringLiteral("market://details?id=is.xyz.vcmi"));
		if(!QDesktopServices::openUrl(playStoreUrl))
		{
			QDesktopServices::openUrl(QUrl(QStringLiteral("https://play.google.com/store/apps/details?id=is.xyz.vcmi")));
		}
		return;
	}
#endif
	startSelectedDownload(url);
}

void UpdateDialog::on_closeButton_clicked()
{
	close();
}

void UpdateDialog::startDownloadToCacheAndRun(const QUrl& url, const QString& target, bool targetIsFile)
{
#if !defined(VCMI_ANDROID)
	Q_UNUSED(targetIsFile);
#endif

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = networkManager.get(request);

    QProgressBar* progress = ui->progressBar;
    if(progress)
	{
        progress->setVisible(true);
        progress->setRange(0, 0);
        connect(reply, &QNetworkReply::downloadProgress, this, [progress](qint64 received, qint64 total) {
            if(!progress)
                return;

			if(total > 0)
			{
                progress->setRange(0, int(total));
                progress->setValue(int(received));
            }
        });
    }

#if defined(VCMI_ANDROID)
    connect(reply, &QNetworkReply::finished, this, [this, reply, progress, target, targetIsFile, requestedUrl = url] {
#else
    connect(reply, &QNetworkReply::finished, this, [this, reply, progress, target, requestedUrl = url] {
#endif
        reply->deleteLater();
        if(reply->error() != QNetworkReply::NoError)
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("Download failed: %1").arg(reply->errorString()));
            return;
        }

        const QString cacheDir = pathToQString(VCMIDirs::get().userCachePath());
        const QString fileName = QFileInfo(requestedUrl.path()).fileName();
        if(fileName.isEmpty())
        {
            if(progress)
                progress->setVisible(false);

            ui->downloadLink->setText(tr("Download URL does not contain a filename."));
            return;
        }

        const QString fullPath = QDir(cacheDir).filePath(fileName);

        QSaveFile out(fullPath);
        if(!out.open(QIODevice::WriteOnly))
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("Can't create file: %1").arg(out.errorString()));
            return;
        }

        const QByteArray data = reply->readAll();
        if(data.isEmpty())
        {
            if(progress)
                progress->setVisible(false);

            ui->downloadLink->setText(tr("Downloaded file is empty."));
            return;
        }

        if(out.write(data) != data.size())
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("Write failed: %1").arg(out.errorString()));
            return;
        }

        if(!out.commit())
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("Commit failed: %1").arg(out.errorString()));
            return;
        }

        // Not strictly needed on mobile, harmless elsewhere
        QFile::setPermissions(fullPath, QFile::permissions(fullPath) | QFileDevice::ExeOwner | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther);

        QFileInfo file(fullPath);
        if(!file.exists() || file.size() == 0)
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("File not saved (path: %1)").arg(fullPath));
            return;
        }

#if defined(VCMI_WINDOWS)
        // Windows: Silent update or Standard setup of not installed channel
		QString selectedChannel = "stable";
		if(ui->tabWidget->currentIndex() == 1 && ui->testingBuilds->isChecked())
			selectedChannel = normalizeChannel(selectedTestingChannel.isEmpty() ? ui->buildChannel->currentData().toString() : selectedTestingChannel);
	
		QStringList uninstallKeys;
		if(selectedChannel == "beta")
			uninstallKeys = QStringList({ "VCMI (branch beta).x64_is1", "VCMI (branch beta).x86_is1" });
		else if(selectedChannel == "develop")
			uninstallKeys = QStringList({ "VCMI (branch develop).x64_is1", "VCMI (branch develop).x86_is1" });
		else
			uninstallKeys = QStringList({ "VCMI .x64_is1", "VCMI.x64_is1", "VCMI .x86_is1", "VCMI.x86_is1" });
	
		bool silentInstall = false;
		for(const auto &keyName : uninstallKeys)
		{
			const QString regPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + keyName;
			QSettings reg(regPath, QSettings::NativeFormat);
			if(reg.contains("UninstallString"))
			{
				silentInstall = true;
				break;
			}
		}
	
		QStringList exeArgs;
		if(silentInstall)
			exeArgs = QStringList({ "/SILENT", "/NORESTART", "/LAUNCH" });
	
		if(QProcess::startDetached(fullPath, exeArgs))
		{
            if(progress)
				progress->setVisible(false);

            QApplication::quit();
        }
        else
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("Failed to start installer."));
        }

#elif defined(VCMI_MAC)
        // macOS: open using default handler
        if(progress)
			progress->setVisible(false);

        if(!QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath)))
            ui->downloadLink->setText(tr("Package saved to %1 — open it manually.").arg(fullPath));

#elif defined(VCMI_ANDROID)
        const bool targetIsContent = target.startsWith("content://", Qt::CaseInsensitive);
        const QString dstPath = targetIsFile ? target : Helper::createFile(target, fileName, QStringLiteral("application/octet-stream"));
        const bool copyOk = !dstPath.isEmpty() && Helper::performNativeCopy(fullPath, dstPath);

        if(copyOk)
        {
            if(targetIsContent && !targetIsFile)
                ui->downloadLink->setText(tr("Saved to selected folder, install it manually."));
            else if(targetIsContent)
                ui->downloadLink->setText(tr("Saved to selected file, install it manually."));
            else
                ui->downloadLink->setText(tr("Saved to: %1 — install it manually.").arg(target));
        }
        else if(targetIsContent)
        {
            ui->downloadLink->setText(tr("Saved to cache (failed to create destination file)."));
        }
        else
        {
            const QString shownPath = targetIsFile ? dstPath : target;
            ui->downloadLink->setText(tr("Saved to: %1 — install it manually.").arg(shownPath));
        }

        if(progress)
			progress->setVisible(false);

#elif defined(VCMI_IOS)
        // iOS: copy into the picked filesystem folder
        {
            const QString dstPath = QDir(target).filePath(fileName);
            Helper::performNativeCopy(fullPath, dstPath);
            Helper::revealDirectoryInFileBrowser(target);
            ui->downloadLink->setText(tr("Saved to: %1 — install it manually.").arg(target));
        }
		if(progress)
			progress->setVisible(false);

#elif defined(VCMI_UNIX)
		QString currentAppImage = qgetenv("APPIMAGE");
		
		if(currentAppImage.isEmpty())
		{
			if(progress)
				progress->setVisible(false);

			ui->downloadLink->setText(tr("No AppImage found."));
			return;
		}

		if(QFile::remove(currentAppImage))
		{ 
			if(QFile::rename(fullPath, currentAppImage))
			{
				// application can restarted
				QProcess::startDetached(currentAppImage, QStringList());
				QCoreApplication::quit();
			}
			else
			{
				if(progress)
					progress->setVisible(false);

				ui->downloadLink->setText(tr("Failed to rename new AppImage."));
			}
		}
		else
		{
            if(progress)
				progress->setVisible(false);

            ui->downloadLink->setText(tr("Failed to remove old AppImage. Check permissions."));
		}

#else
        // Fallback: just open or inform
        if(progress)
			progress->setVisible(false);

        if(!QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath)))
            ui->downloadLink->setText(tr("Package saved to %1 — open it manually.").arg(fullPath));
#endif
    });
}
