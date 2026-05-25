/*
 * editorfiledialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "editorfiledialog.h"

#include <QFileDialog>
#include <QWidget>

#ifdef VCMI_ANDROID
#include "androidfilepicker.h"
#endif

QString EditorFileDialog::getOpenFileName(QWidget * parent, const QString & title,
	const QString & dir, const QString & filter,
	bool externalOnly)
{
#ifdef VCMI_ANDROID
	return AndroidFilePicker::getOpenFileName(parent, title, dir, filter,
		externalOnly ? AndroidFilePicker::Mode::ExternalOnly
		             : AndroidFilePicker::Mode::InternalOrExternal);
#else
	Q_UNUSED(externalOnly)
	return QFileDialog::getOpenFileName(parent, title, dir, filter);
#endif
}

QString EditorFileDialog::getSaveFileName(QWidget * parent, const QString & title,
	const QString & dir, const QString & filter,
	QString & outContentUri,
	bool externalOnly)
{
	outContentUri.clear();
#ifdef VCMI_ANDROID
	return AndroidFilePicker::getSaveFileName(parent, title, dir, filter,
		externalOnly ? AndroidFilePicker::Mode::ExternalOnly
		             : AndroidFilePicker::Mode::InternalOrExternal,
		outContentUri);
#else
	Q_UNUSED(externalOnly)
	return QFileDialog::getSaveFileName(parent, title, dir, filter);
#endif
}

void EditorFileDialog::writeFileToUri(const QString & localPath, const QString & contentUri)
{
	if(contentUri.isEmpty())
		return;
#ifdef VCMI_ANDROID
	AndroidFilePicker::writeFileToUri(localPath, contentUri);
#endif
}
