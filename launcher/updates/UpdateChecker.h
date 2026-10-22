/*
 * UpdateChecker.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QObject>
#include <QUrl>

class CDownloadManager;

/// Source the running build was installed from, as reported to the update service
enum class EUpdateDistribution : int8_t
{
	INSTALLER,
	PACKAGE,
	GOOGLE_PLAY,
	FDROID,
	FLATPAK,
	TESTFLIGHT,
	UNKNOWN
};

enum class EUpdateAction : int8_t
{
	INSTALL,
	DOWNLOAD,
	EXTERNAL ///< update happens outside of the launcher, e.g. through a store
};

enum class EUpdateCheckState : int8_t
{
	DISABLED, ///< see UpdateChecker::isEnabled
	NOT_CHECKED,
	CHECKING,
	FAILED,
	NO_UPDATE,
	UPDATE_AVAILABLE
};

struct UpdateInfo
{
	QString version;
	QString date;
	EUpdateAction action = EUpdateAction::EXTERNAL;
	QUrl url; ///< file to download, set for INSTALL and DOWNLOAD
	QStringList notes;
	QString changelog; ///< Markdown
	QUrl changelogUrl;
};

/// Queries the update service for a newer version of VCMI
class UpdateChecker : public QObject
{
	Q_OBJECT

	std::unique_ptr<CDownloadManager> downloadManager;
	EUpdateCheckState state;
	UpdateInfo update;
	QString errorMessage;

	void setState(EUpdateCheckState newState);
	void onDownloadFinished(const QStringList & savedFiles, const QStringList & errors);
	void parseResponse(const QByteArray & data);

public:
	explicit UpdateChecker(QObject * parent);
	~UpdateChecker() override;

	/// false for builds of a channel other than stable, or for a platform unknown to the update service
	static bool isEnabled();

	/// starts a check, unless checks are disabled or a check is already running
	void check();

	EUpdateCheckState getState() const;
	/// valid in UPDATE_AVAILABLE state
	const UpdateInfo & getUpdate() const;
	/// valid in FAILED state
	const QString & getErrorMessage() const;

	static EUpdateDistribution getDistribution();

signals:
	void stateChanged();
};
