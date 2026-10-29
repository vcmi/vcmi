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
#include "../../lib/texts/Languages.h"
#include "../../lib/texts/TextOperations.h"

ModState::ModState(const ModDescription & impl)
	: impl(impl)
{
}

QString ModState::getName() const
{
	return QString::fromStdString(impl.getName());
}

QString ModState::getNameEnglish() const
{
	return QString::fromStdString(impl.getValue("name").String());
}

QString ModState::getType() const
{
	return QString::fromStdString(impl.getValue("modType").String());
}

QString ModState::getDescription() const
{
	QString description = QString::fromStdString(impl.getLocalizedDescription().String());

	if (Qt::mightBeRichText(description))
	{
		QTextDocument document;
		document.setHtml(description);
		description = document.toMarkdown();
	}

	return description;
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
	return QString::fromStdString(impl.getTopParentID());
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
	return stringListStdToQt(impl.getRepositoryValue("screenshots").convertTo<std::vector<std::string>>());
}

QString ModState::getBaseLanguage() const
{
	return QString::fromStdString(impl.getBaseLanguage());
}

int ModState::getTranslationProgress(const QString & language) const
{
	const JsonNode & progress = impl.getRepositoryValue(language.toStdString())["progress"];
	return progress.isNumber() ? progress.Integer() : -1;
}

bool ModState::hasTranslation(const QString & language) const
{
	int progress = getTranslationProgress(language);

	// mods released outside of mod repository workflow don't report progress
	if (progress == -1)
		return !impl.getValue(language.toStdString())["translations"].isNull();

	// workflow may create language entry with zero progress just for translated mod name
	return progress > 0;
}

QMap<QString, QStringList> ModState::getChangelog() const
{
	QMap<QString, QStringList> result;
	const JsonNode & changelog = impl.getLocalizedValue("changelog");

	for (const auto & entry : changelog.Struct())
	{
		QString version = QString::fromStdString(entry.first);
		QStringList changes = stringListStdToQt(entry.second.convertTo<std::vector<std::string>>());

		result[version]	= changes;
	}

	return result;
}

QString ModState::getInstalledVersion() const
{
	return QString::fromStdString(impl.getLocalValue("version").String());
}

QString ModState::getRepositoryVersion() const
{
	return QString::fromStdString(impl.getRepositoryValue("version").String());
}

QString ModState::getVersion() const
{
	return QString::fromStdString(impl.getValue("version").String());
}

QString ModState::getUpdateDate() const
{
	// repository date describes repository version, which may differ from installed one
	if (getRepositoryVersion() != getVersion())
		return {};

	QDateTime date = QDateTime::fromString(QString::fromStdString(impl.getRepositoryValue("updateDate").String()), Qt::ISODate);
	if (!date.isValid())
		return {};

	return QString::fromStdString(TextOperations::getFormattedDateTimeLocal(date.toSecsSinceEpoch()));
}

int ModState::getGithubStars() const
{
	return impl.getRepositoryValue("githubStars").isNull() ? -1 : impl.getRepositoryValue("githubStars").Integer();
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
	return QCoreApplication::translate("File size", "%1 MiB").arg(QString::number(getDownloadSizeMegabytes(), 'f', 1));
}

QStringList ModState::getAuthors() const
{
	const JsonNode & author = impl.getLocalizedValue("author");

	if (author.isVector())
		return stringListStdToQt(author.convertTo<std::vector<std::string>>());

	if (author.String().empty())
		return {};

	return { QString::fromStdString(author.String()) };
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
	const JsonNode & compatibility = impl.getLocalValue("compatibility");

	if (compatibility.isNull())
		return {};

	auto min = QString::fromStdString(compatibility["min"].String());
	auto max = QString::fromStdString(compatibility["max"].String());
	return { min, max};
}

bool ModState::isValidationFailed() const
{
	const JsonNode & validated = impl.getRepositoryValue("validated");

	// repository result describes repository version, which may differ from installed one
	return getRepositoryVersion() == getVersion() && validated.isBool() && !validated.Bool();
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

bool ModState::isDemoSupport() const
{
	return impl.isDemoSupport();
}

bool ModState::isVisible() const
{
	return !isHidden();
}

bool ModState::isHidden() const
{
	if (isTranslation() && !isInstalled())
		return impl.getBaseLanguage() != CGeneralTextHandler::getPreferredLanguage();

	return isCompatibility() || getID() == "vcmi" || getID() == "core";
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
	return impl.isUpdateAvailable();
}

bool ModState::isCompatible() const
{
	return impl.isCompatible();
}

bool ModState::isKeptDisabled() const
{
	return impl.keepDisabled();
}
