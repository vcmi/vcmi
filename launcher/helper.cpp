/*
 * helper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "helper.h"

#include "mainwindow_moc.h"
#include "settingsView/csettingsview_moc.h"
#include "modManager/cmodlistview_moc.h"

#include "../lib/CConfigHandler.h"

#include <QObject>
#include <QScroller>

#ifdef VCMI_ANDROID
#include <QAndroidJniObject>
#include <QtAndroid>
#endif

#ifdef VCMI_IOS
#include "ios/revealdirectoryinfiles.h"
#include "iOS_utils.h"
#endif

#ifdef VCMI_MOBILE
static QScrollerProperties generateScrollerProperties()
{
	QScrollerProperties result;

	result.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.25);
	result.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.25);
	result.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);

	return result;
}
#endif

#ifdef VCMI_ANDROID
static QString safeEncode(QString uri)
{
	// %-encode unencoded parts of string.
	// This is needed because Qt returns a mixed content url with %-encoded and unencoded parts. On Android >= 13 this causes problems reading these files, when using spaces and unicode characters in folder or filename.
	// Only these should be encoded (other typically %-encoded chars should not be encoded because this leads to errors).
	// Related, but seems not completly fixed (at least in our setup): https://bugreports.qt.io/browse/QTBUG-114435
	if (!uri.startsWith("content://", Qt::CaseInsensitive))
		return uri;
	return QString::fromUtf8(QUrl::toPercentEncoding(uri, "!#$&'()*+,/:;=?@[]<>{}\"`^~%"));
}
#endif

namespace Helper
{
void loadSettings()
{
	settings.init("config/settings.json", "vcmi:settings");
	persistentStorage.init("config/persistentStorage.json", "");
}

void reLoadSettings()
{
	loadSettings();
	for(const auto widget : qApp->allWidgets())
		if(auto settingsView = qobject_cast<CSettingsView *>(widget))
		{
			settingsView->loadSettings();
			break;
		}
	getMainWindow()->updateTranslation();
	getMainWindow()->getModView()->reload();
}

void enableScrollBySwiping(QObject * scrollTarget)
{
#ifdef VCMI_MOBILE
	QScroller::grabGesture(scrollTarget, QScroller::LeftMouseButtonGesture);
	QScroller * scroller = QScroller::scroller(scrollTarget);
	scroller->setScrollerProperties(generateScrollerProperties());
#endif
}

QString getRealPath(QString path)
{
#ifdef VCMI_ANDROID
	if(path.contains("content://", Qt::CaseInsensitive))
	{
		auto str = QAndroidJniObject::fromString(safeEncode(path));
		return QAndroidJniObject::callStaticObjectMethod("eu/vcmi/vcmi/util/FileUtil", "getFilenameFromUri", "(Ljava/lang/String;Landroid/content/Context;)Ljava/lang/String;", str.object<jstring>(), QtAndroid::androidContext().object()).toString();
	}
	return path;
#else
	return path;
#endif
}

bool performNativeCopy(QString src, QString dst)
{
#ifdef VCMI_ANDROID
	auto srcStr = QAndroidJniObject::fromString(safeEncode(src));
	auto dstStr = QAndroidJniObject::fromString(safeEncode(dst));
	QAndroidJniObject::callStaticObjectMethod("eu/vcmi/vcmi/util/FileUtil", "copyFileFromUri", "(Ljava/lang/String;Ljava/lang/String;Landroid/content/Context;)V", srcStr.object<jstring>(), dstStr.object<jstring>(), QtAndroid::androidContext().object());

	if(QFileInfo(dst).exists())
		return true;
	else
		return false;
#else
	return QFile::copy(src, dst);
#endif
}

void revealDirectoryInFileBrowser(QString path)
{
	const auto dirUrl = QUrl::fromLocalFile(QFileInfo{path}.absoluteFilePath());
#ifdef VCMI_IOS
	iOS_utils::revealDirectoryInFiles(dirUrl);
#else
	QDesktopServices::openUrl(dirUrl);
#endif
}

MainWindow * getMainWindow()
{
	foreach(QWidget *w, qApp->allWidgets())
		if(auto mainWin = qobject_cast<MainWindow*>(w))
			return mainWin;
	return nullptr;
}


void keepScreenOn(bool isEnabled)
{
#if defined(VCMI_ANDROID)
	QtAndroid::runOnAndroidThread([isEnabled]
	{
		QtAndroid::androidActivity().callMethod<void>("keepScreenOn", "(Z)V", isEnabled);
	});
#elif defined(VCMI_IOS)
	iOS_utils::keepScreenOn(isEnabled);
#endif
}
}
