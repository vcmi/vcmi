/*
 * modstate.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modstate.h"

#include "../../lib/modding/ModDescription.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/texts/CGeneralTextHandler.h"

ModState::ModState(const ModDescription & impl)
	: impl(impl)
{
}

QString ModState::getName() const
{
	return QString::fromStdString(impl.getName());
}

QString ModState::getType() const
{
	return QString::fromStdString(impl.getValue("modType").String());
}

QString ModState::getDescription() const
{
	return QString::fromStdString(impl.getValue("description").String());
}

QString ModState::getID() const
{
	return QString::fromStdString(impl.getID());
}

QString ModState::getParentID() const
{
	return QString::fromStdString(impl.getParentID());
}

QString ModState::getTopParentID() const
{
	return QString::fromStdString(impl.getParentID());
}

template<typename Container>
QStringList stringListStdToQt(const Container & container)
{
	QStringList result;
	for (const auto & str : container)
		result.push_back(QString::fromStdString(str));
	return result;
}

QStringList ModState::getDependencies() const
{
	return stringListStdToQt(impl.getDependencies());
}

QStringList ModState::getConflicts() const
{
	return stringListStdToQt(impl.getConflicts());
}

QStringList ModState::getScreenshots() const
{
	return {}; // TODO
}

QString ModState::getBaseLanguage() const
{
	return QString::fromStdString(impl.getBaseLanguage());
}

QStringList ModState::getSupportedLanguages() const
{
	return {}; //TODO
}

QMap<QString, QStringList> ModState::getChangelog() const
{
	return {}; //TODO
}

QString ModState::getInstalledVersion() const
{
	return QString::fromStdString(impl.getLocalValue("version").String());
}

QString ModState::getRepositoryVersion() const
{
	return QString::fromStdString(impl.getRepositoryValue("version").String());
}

double ModState::getDownloadSizeMegabytes() const
{
	return impl.getRepositoryValue("downloadSize").Float();
}

size_t ModState::getDownloadSizeBytes() const
{
	return getDownloadSizeMegabytes() * 1024 * 1024;
}

QString ModState::getDownloadSizeFormatted() const
{
	return {}; // TODO
}

QString ModState::getLocalSizeFormatted() const
{
	return {}; // TODO
}

QString ModState::getAuthors() const
{
	return QString::fromStdString(impl.getValue("author").String());
}

QString ModState::getContact() const
{
	return QString::fromStdString(impl.getValue("contact").String());
}

QString ModState::getLicenseUrl() const
{
	return QString::fromStdString(impl.getValue("licenseURL").String());
}

QString ModState::getLicenseName() const
{
	return QString::fromStdString(impl.getValue("licenseName").String());
}

QString ModState::getDownloadUrl() const
{
	return QString::fromStdString(impl.getRepositoryValue("download").String());
}

QPair<QString, QString> ModState::getCompatibleVersionRange() const
{
	return {}; // TODO
}

bool ModState::isSubmod() const
{
	return !getParentID().isEmpty();
}

bool ModState::isCompatibility() const
{
	return impl.isCompatibility();
}

bool ModState::isTranslation() const
{
	return impl.isTranslation();
}

bool ModState::isVisible() const
{
	return !isHidden();
}

bool ModState::isHidden() const
{
	if (isTranslation() && !isInstalled())
		return impl.getBaseLanguage() == CGeneralTextHandler::getPreferredLanguage();

	return isCompatibility() || getID() == "vcmi" || getID() == "core";
}

bool ModState::isDisabled() const
{
	return false; // TODO
}

bool ModState::isEnabled() const
{
	return true; // TODO
}

bool ModState::isAvailable() const
{
	return !isInstalled();
}

bool ModState::isInstalled() const
{
	return impl.isInstalled();
}

bool ModState::isUpdateAvailable() const
{
	return getInstalledVersion() != getRepositoryVersion() && !getRepositoryVersion().isEmpty() && !getInstalledVersion().isEmpty();
}

bool ModState::isCompatible() const
{
	return true; //TODO
}

bool ModState::isKeptDisabled() const
{
	return impl.keepDisabled();
}
