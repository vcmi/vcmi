/*
 * prepare_android.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "prepare_p.h"
#include "../lib/CAndroidVMHelper.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QtAndroid>

namespace
{
// https://gist.github.com/ssendeavour/7324701
bool copyRecursively(const QString & srcFilePath, const QString & tgtFilePath)
{
	QFileInfo srcFileInfo{srcFilePath};
	if(srcFileInfo.isDir()) {
		QDir targetDir{tgtFilePath};
		targetDir.cdUp();
		if(!targetDir.mkpath(QFileInfo{tgtFilePath}.fileName()))
			return false;
		targetDir.setPath(tgtFilePath);

		QDir sourceDir{srcFilePath};
		const auto fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
		for(const auto & fileName : fileNames) {
			const auto newSrcFilePath = sourceDir.filePath(fileName);
			const auto newTgtFilePath = targetDir.filePath(fileName);
			if(!copyRecursively(newSrcFilePath, newTgtFilePath))
				return false;
		}
	} else {
		if(!QFile::copy(srcFilePath, tgtFilePath))
			return false;
	}
	return true;
}
}

namespace launcher
{
void prepareAndroid()
{
	QAndroidJniEnvironment jniEnv;
	CAndroidVMHelper::initClassloader(static_cast<JNIEnv *>(jniEnv));

	const bool justLaunched = QtAndroid::androidActivity().getField<jboolean>("justLaunched") == JNI_TRUE;
	if(!justLaunched)
		return;

	// copy core data to internal directory
	const auto vcmiDir = QAndroidJniObject::callStaticObjectMethod<jstring>("eu/vcmi/vcmi/NativeMethods", "internalDataRoot").toString();
	for(auto vcmiFilesResource : {QLatin1String{"config"}, QLatin1String{"Mods"}})
	{
		QDir destDir = QString{"%1/%2"}.arg(vcmiDir, vcmiFilesResource);
		destDir.removeRecursively();
		copyRecursively(QString{":/%1"}.arg(vcmiFilesResource), destDir.absolutePath());
	}
}
}
