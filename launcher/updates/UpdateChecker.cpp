/*
 * UpdateChecker.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "UpdateChecker.h"

#include "../helper.h"
#include "../modManager/cdownloadmanager_moc.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/json/JsonFormatException.h"
#include "../../lib/json/JsonNode.h"

#ifdef VCMI_IOS
#include "iOS_utils.h"
#endif

namespace
{

/// returns empty string if the update service does not know this operating system
QString operatingSystemName()
{
#if defined(VCMI_WINDOWS)
	return "windows";
#elif defined(VCMI_ANDROID)
	return "android";
#elif defined(VCMI_IOS)
	return "ios";
#elif defined(VCMI_MAC)
	return "macos";
#elif defined(Q_OS_LINUX)
	return "linux";
#else
	return {};
#endif
}

/// returns empty string if the update service does not know this architecture
QString architectureName()
{
	static const QMap<QString, QString> names = {
		{"i386", "x86"},
		{"x86_64", "x64"},
		{"arm", "arm32"},
		{"arm64", "arm64"},
	};
	return names.value(QSysInfo::buildCpuArchitecture());
}

QString distributionName(EUpdateDistribution distribution)
{
	switch(distribution)
	{
		case EUpdateDistribution::INSTALLER:
			return "installer";
		case EUpdateDistribution::PACKAGE:
			return "package";
		case EUpdateDistribution::GOOGLE_PLAY:
			return "googleplay";
		case EUpdateDistribution::FDROID:
			return "fdroid";
		case EUpdateDistribution::FLATPAK:
			return "flatpak";
		case EUpdateDistribution::TESTFLIGHT:
			return "testflight";
		case EUpdateDistribution::UNKNOWN:
			return "unknown";
	}
	throw std::runtime_error("Unknown update distribution");
}

#ifdef VCMI_WINDOWS
bool isInstalledByInstaller()
{
	const QString launcherDirectory = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

	// the installer runs in 32-bit mode, so machine-wide installs are registered in the 32-bit registry view
	for(const QString & root : {QString("HKEY_CURRENT_USER"), QString("HKEY_LOCAL_MACHINE")})
	{
		QSettings uninstallKeys(root + "\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", QSettings::Registry32Format);
		for(const QString & application : uninstallKeys.childGroups())
		{
			// suffix of all applications installed by Inno Setup
			if(!application.endsWith("_is1"))
				continue;

			const QString installLocation = uninstallKeys.value(application + "/InstallLocation").toString();
			if(installLocation.isEmpty())
				continue;

			if(QDir(installLocation).canonicalPath().compare(launcherDirectory, Qt::CaseInsensitive) == 0)
				return true;
		}
	}
	return false;
}
#endif

#ifdef VCMI_ANDROID
EUpdateDistribution distributionFromInstaller(const QString & installer)
{
	static const QStringList fdroidClients = {
		"org.fdroid.fdroid",
		"org.fdroid.basic",
		"com.looker.droidify",
		"com.machiav3lli.fdroid",
	};
	static const QStringList packageInstallers = {
		"com.google.android.packageinstaller",
		"com.android.packageinstaller",
	};

	if(installer == "com.android.vending")
		return EUpdateDistribution::GOOGLE_PLAY;
	if(fdroidClients.contains(installer))
		return EUpdateDistribution::FDROID;
	// installed from an APK file, e.g. through a browser or a file manager
	if(installer.isEmpty() || packageInstallers.contains(installer))
		return EUpdateDistribution::PACKAGE;
	return EUpdateDistribution::UNKNOWN;
}
#endif

QString requireString(const JsonNode & response, const std::string & field)
{
	const JsonNode & node = response[field];
	if(!node.isString())
		throw std::runtime_error("field '" + field + "' is missing or is not a string");
	return QString::fromStdString(node.String());
}

}

UpdateChecker::UpdateChecker(QObject * parent)
	: QObject(parent)
	, state(EUpdateCheckState::NOT_CHECKED)
{
	if(QString(VCMI_UPDATE_CHANNEL) != "stable" || operatingSystemName().isEmpty() || architectureName().isEmpty())
		state = EUpdateCheckState::DISABLED;
}

UpdateChecker::~UpdateChecker() = default;

EUpdateDistribution UpdateChecker::getDistribution()
{
#if defined(VCMI_WINDOWS)
	return isInstalledByInstaller() ? EUpdateDistribution::INSTALLER : EUpdateDistribution::PACKAGE;
#elif defined(VCMI_ANDROID)
	return distributionFromInstaller(Helper::getInstallerPackageName());
#elif defined(VCMI_IOS)
	return iOS_utils::isTestFlightBuild() ? EUpdateDistribution::TESTFLIGHT : EUpdateDistribution::UNKNOWN;
#elif defined(VCMI_MAC)
	return EUpdateDistribution::PACKAGE;
#elif defined(VCMI_FLATPAK)
	return EUpdateDistribution::FLATPAK;
#else
	// set by the AppImage runtime
	return qEnvironmentVariableIsSet("APPIMAGE") ? EUpdateDistribution::PACKAGE : EUpdateDistribution::UNKNOWN;
#endif
}

void UpdateChecker::check()
{
	if(state == EUpdateCheckState::DISABLED || state == EUpdateCheckState::CHECKING)
		return;

	QUrlQuery query;
	query.addQueryItem("version", VCMI_VERSION_STRING);
	query.addQueryItem("os", operatingSystemName());
	query.addQueryItem("arch", architectureName());
	query.addQueryItem("dist", distributionName(getDistribution()));
	query.addQueryItem("lang", QString::fromStdString(settings["general"]["language"].String()));

	QUrl url(QString::fromStdString(settings["launcher"]["updateConfigUrl"].String()));
	url.setQuery(query);
	logGlobal->info("Checking for updates: %s", url.toString().toStdString());

	// download manager does not reset its list of downloads and errors
	downloadManager = std::make_unique<CDownloadManager>();
	connect(downloadManager.get(), &CDownloadManager::finished, this, [this](const QStringList & savedFiles, const QStringList &, const QStringList & errors)
	{
		onDownloadFinished(savedFiles, errors);
	});
	downloadManager->downloadFile(url, "autoupdate.json");
	setState(EUpdateCheckState::CHECKING);
}

void UpdateChecker::onDownloadFinished(const QStringList & savedFiles, const QStringList & errors)
{
	if(savedFiles.isEmpty())
	{
		errorMessage = errors.join("\n");
		logGlobal->warn("Failed to check for updates: %s", errorMessage.toStdString());
		setState(EUpdateCheckState::FAILED);
		return;
	}

	QFile file(savedFiles.front());
	if(!file.open(QFile::ReadOnly))
	{
		errorMessage = file.errorString();
		logGlobal->warn("Failed to read update information: %s", errorMessage.toStdString());
		setState(EUpdateCheckState::FAILED);
		return;
	}
	const QByteArray data = file.readAll();
	file.close();
	file.remove();

	try
	{
		parseResponse(data);
	}
	catch(const std::runtime_error & e)
	{
		errorMessage = QString::fromStdString(e.what());
		logGlobal->warn("Invalid update information: %s", e.what());
		setState(EUpdateCheckState::FAILED);
	}
}

void UpdateChecker::parseResponse(const QByteArray & data)
{
	JsonParsingSettings parserSettings;
	parserSettings.mode = JsonParsingSettings::JsonFormatMode::JSON;
	parserSettings.maxDepth = 3;
	parserSettings.strict = true;

	const JsonNode response(data.data(), data.size(), parserSettings, "update information");
	if(!response.isStruct())
		throw std::runtime_error("response is not a JSON object");

	const QString status = requireString(response, "status");
	if(status == "none")
	{
		setState(EUpdateCheckState::NO_UPDATE);
		return;
	}
	if(status != "update")
		throw std::runtime_error("unknown status '" + status.toStdString() + "'");

	UpdateInfo result;
	result.version = requireString(response, "version");
	if(!QRegularExpression("^[0-9]+\\.[0-9]+\\.[0-9]+$").match(result.version).hasMatch())
		throw std::runtime_error("invalid version '" + result.version.toStdString() + "'");
	result.date = requireString(response, "date");
	result.changelog = requireString(response, "changelog");

	if(!response["notes"].isVector())
		throw std::runtime_error("field 'notes' is missing or is not a list");
	for(const auto & note : response["notes"].Vector())
	{
		if(!note.isString())
			throw std::runtime_error("field 'notes' contains a value that is not a string");
		result.notes.push_back(QString::fromStdString(note.String()));
	}

	const QUrl changelogUrl(requireString(response, "changelogUrl"));
	if(changelogUrl.scheme() == "https")
		result.changelogUrl = changelogUrl;
	else
		logGlobal->warn("Ignoring changelog URL '%s'", changelogUrl.toString().toStdString());

	const QUrl url = response["url"].isString() ? QUrl(QString::fromStdString(response["url"].String())) : QUrl();
	const bool urlAllowed = url.scheme() == "https" && url.host() == "download.vcmi.eu";
	if(!url.isEmpty() && !urlAllowed)
		logGlobal->warn("Ignoring download URL '%s'", url.toString().toStdString());

	// actions unknown to this version are handled as a download if there is a file, as an external update otherwise
	const QString action = requireString(response, "action");
	if(action == "external" || !urlAllowed)
	{
		result.action = EUpdateAction::EXTERNAL;
	}
	else
	{
		result.action = action == "install" ? EUpdateAction::INSTALL : EUpdateAction::DOWNLOAD;
		result.url = url;
	}

	update = result;
	setState(EUpdateCheckState::UPDATE_AVAILABLE);
}

void UpdateChecker::setState(EUpdateCheckState newState)
{
	state = newState;
	emit stateChanged();
}

EUpdateCheckState UpdateChecker::getState() const
{
	return state;
}

const UpdateInfo & UpdateChecker::getUpdate() const
{
	return update;
}

const QString & UpdateChecker::getErrorMessage() const
{
	return errorMessage;
}
