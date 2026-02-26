/*
 * modstate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class ModDescription;
VCMI_LIB_NAMESPACE_END

/// Class that represent current state of mod in Launcher
/// Provides Qt-based interface to library class ModDescription
class ModState
{
	const ModDescription & impl;

public:
	explicit ModState(const ModDescription & impl);

	QString getName() const;
	QString getType() const;
	QString getDescription() const;

	QString getID() const;
	QString getParentID() const;
	QString getTopParentID() const;

	QStringList getDependencies() const;
	QStringList getConflicts() const;
	QStringList getScreenshots() const;

	QString getBaseLanguage() const;
	QStringList getSupportedLanguages() const;

	QMap<QString, QStringList> getChangelog() const;

	QString getInstalledVersion() const;
	QString getRepositoryVersion() const;
	QString getVersion() const;
	int getGithubStars() const;

	double getDownloadSizeMegabytes() const;
	size_t getDownloadSizeBytes() const;
	QString getDownloadSizeFormatted() const;
	QString getAuthors() const;
	QString getContact() const;
	QString getLicenseUrl() const;
	QString getLicenseName() const;

	QString getDownloadUrl() const;

	QPair<QString, QString> getCompatibleVersionRange() const;

	bool isSubmod() const;
	bool isCompatibility() const;
	bool isTranslation() const;

	bool isVisible() const;
	bool isHidden() const;
	bool isAvailable() const;
	bool isInstalled() const;
	bool isUpdateAvailable() const;
	bool isCompatible() const;
	bool isKeptDisabled() const;
};
