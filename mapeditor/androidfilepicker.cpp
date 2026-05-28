/*
 * androidfilepicker.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "androidfilepicker.h"

#ifdef VCMI_ANDROID

#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QRegExp>
#include <QWidget>

#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QtAndroid>

QString AndroidFilePicker::getOpenFileName(QWidget * parent, const QString & title,
	const QString & internalDir, const QString & filter, Mode mode)
{
	if(mode == Mode::ExternalOnly)
	{
		return nativeOpenFile("*/*");
	}

	// Ask user: internal or external
	QMessageBox box(parent);
	box.setWindowTitle(title);
	box.setText(QObject::tr("Where do you want to open the file from?"));
	auto * btnInternal = box.addButton(QObject::tr("Internal"), QMessageBox::AcceptRole);
	auto * btnExternal = box.addButton(QObject::tr("External"), QMessageBox::ActionRole);
	box.addButton(QMessageBox::Cancel);
	box.exec();

	if(box.clickedButton() == btnInternal)
	{
		return QFileDialog::getOpenFileName(parent, title, internalDir, filter,
			nullptr, QFileDialog::DontUseNativeDialog);
	}
	else if(box.clickedButton() == btnExternal)
	{
		return nativeOpenFile("*/*");
	}

	return {};
}

QString AndroidFilePicker::getSaveFileName(QWidget * parent, const QString & title,
	const QString & internalDir, const QString & filter, Mode mode,
	QString & outContentUri)
{
	outContentUri.clear();

	if(mode == Mode::ExternalOnly)
	{
		// Derive MIME type, suggested filename and temp-file extension from the
		// filter string (e.g. "PNG (*.png)" → ext="png", mime="image/png").
		QString ext;
		const int starDot = filter.indexOf("*.");
		if(starDot >= 0)
		{
			int end = filter.indexOf(QRegExp("[^a-zA-Z0-9]"), starDot + 2);
			ext = filter.mid(starDot + 2, end < 0 ? -1 : end - (starDot + 2)).toLower();
		}

		QString mimeType     = "application/octet-stream";
		QString suggestedName = ext.isEmpty() ? "file" : "export." + ext;
		if     (ext == "png")            mimeType = "image/png";
		else if(ext == "jpg" || ext == "jpeg") mimeType = "image/jpeg";
		else if(ext == "bmp")            mimeType = "image/bmp";
		else if(ext == "vmap")           suggestedName = "map.vmap";
		else if(ext == "vcmp")           suggestedName = "campaign.vcmp";
		else if(ext == "json")           { mimeType = "application/json"; suggestedName = "template.json"; }

		QString uri = nativeSaveFile(mimeType, suggestedName);
		if(uri.isEmpty())
			return {};
		outContentUri = uri;

		// Build a temp path with the correct extension so that callers like
		// QImage::save() can detect the format from the file name.
		QString tempDir = QDir::tempPath();
		QDir().mkpath(tempDir);
		return tempDir + "/vcmi_tmp." + (ext.isEmpty() ? "bin" : ext);
	}

	// Ask user: internal or external
	QMessageBox box(parent);
	box.setWindowTitle(title);
	box.setText(QObject::tr("Where do you want to save the file?"));
	auto * btnInternal = box.addButton(QObject::tr("Internal"), QMessageBox::AcceptRole);
	auto * btnExternal = box.addButton(QObject::tr("External"), QMessageBox::ActionRole);
	box.addButton(QMessageBox::Cancel);
	box.exec();

	if(box.clickedButton() == btnInternal)
	{
		return QFileDialog::getSaveFileName(parent, title, internalDir, filter,
			nullptr, QFileDialog::DontUseNativeDialog);
	}
	else if(box.clickedButton() == btnExternal)
	{
		QString suggestedName = "map.vmap";
		// Extract suggested name from filter
		if(filter.contains("*.vmap"))
			suggestedName = "map.vmap";
		else if(filter.contains("*.vcmp"))
			suggestedName = "campaign.vcmp";
		else if(filter.contains("*.json"))
			suggestedName = "template.json";

		QString mimeType = "application/octet-stream";

		QString uri = nativeSaveFile(mimeType, suggestedName);
		if(uri.isEmpty())
			return {};

		outContentUri = uri;
		QDir().mkpath(QDir::tempPath());
		QString tempPath = QDir::tempPath() + "/vcmi_export_tmp";
		return tempPath;
	}

	return {};
}

void AndroidFilePicker::writeFileToUri(const QString & localPath, const QString & contentUri)
{
	QAndroidJniObject activity = QtAndroid::androidActivity();
	if(!activity.isValid())
		return;

	QAndroidJniObject jLocalPath = QAndroidJniObject::fromString(localPath);
	QAndroidJniObject jUri = QAndroidJniObject::fromString(contentUri);

	activity.callMethod<void>("writeFileToUri",
		"(Ljava/lang/String;Ljava/lang/String;)V",
		jLocalPath.object<jstring>(), jUri.object<jstring>());

	QAndroidJniEnvironment env;
	if(env->ExceptionCheck())
		env->ExceptionClear();
}

QString AndroidFilePicker::nativeOpenFile(const QString & mimeType)
{
	QAndroidJniObject activity = QtAndroid::androidActivity();
	if(!activity.isValid())
		return {};

	QAndroidJniObject jMimeType = QAndroidJniObject::fromString(mimeType);
	QAndroidJniObject result = activity.callObjectMethod("pickFileForOpen",
		"(Ljava/lang/String;)Ljava/lang/String;",
		jMimeType.object<jstring>());

	if(!result.isValid())
		return {};

	QString path = result.toString();
	return path.isEmpty() ? QString() : path;
}

QString AndroidFilePicker::nativeSaveFile(const QString & mimeType, const QString & suggestedName)
{
	QAndroidJniObject activity = QtAndroid::androidActivity();
	if(!activity.isValid())
		return {};

	QAndroidJniObject jMimeType = QAndroidJniObject::fromString(mimeType);
	QAndroidJniObject jName = QAndroidJniObject::fromString(suggestedName);
	QAndroidJniObject result = activity.callObjectMethod("pickFileForSave",
		"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
		jMimeType.object<jstring>(), jName.object<jstring>());

	if(!result.isValid())
		return {};

	QString uri = result.toString();
	return uri.isEmpty() ? QString() : uri;
}

#endif // VCMI_ANDROID
