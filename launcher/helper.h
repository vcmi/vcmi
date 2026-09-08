/*
 * helper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QString>

class QObject;
class MainWindow;

namespace Helper
{
	void loadSettings();
	void reLoadSettings();
	void enableScrollBySwiping(QObject * scrollTarget);
	QString getRealPath(QString path);
	bool performNativeCopy(QString src, QString dst);
	QString createFile(QString target, QString fileName, QString mime);
	void revealDirectoryInFileBrowser(QString path);
	MainWindow * getMainWindow();
	void keepScreenOn(bool isEnabled);
	/// on Android, gamepad input is handled by the activity - tell it whether starting the game is possible right now
	void allowGamepadStart(bool isEnabled);
	bool canUseFolderPicker();
	void nativeFolderPicker(QWidget *parent, std::function<void(QString)>&& cb);
	QStringList findFilesForCopy(const QString &treeUri);
	void sendFileToApp(QString path);
	/// looks up a Steam game's install directory by AppID (Windows only - returns empty string elsewhere or if not found)
	QString findSteamGameInstallDir(const QString & appId, const QString & fallbackInstallDir = QString());
#ifdef VCMI_ANDROID
	bool isInstalledFromGooglePlay();
#endif
}
