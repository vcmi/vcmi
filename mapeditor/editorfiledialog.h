/*
 * editorfiledialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QString>

class QWidget;

/// Thin wrappers around file dialogs that transparently handle
/// the Android SAF (Storage Access Framework) file picker.
/// On non-Android platforms these simply delegate to QFileDialog.
namespace EditorFileDialog
{
	/// Show an open-file dialog. Returns selected path or empty string on cancel.
	/// When externalOnly is true (Android), skips the "internal Maps folder" option
	/// and always opens the native SAF picker (useful for templates / images).
	QString getOpenFileName(QWidget * parent, const QString & title,
		const QString & dir, const QString & filter,
		bool externalOnly = false);

	/// Show a save-file dialog. Returns selected path or empty string on cancel.
	/// On Android, outContentUri receives the content:// URI if the user chose
	/// external storage; call writeFileToUri() after saving locally.
	/// When externalOnly is true (Android), always opens the native SAF picker.
	QString getSaveFileName(QWidget * parent, const QString & title,
		const QString & dir, const QString & filter,
		QString & outContentUri,
		bool externalOnly = false);

	/// After saving a file locally on Android, copies it to the content:// URI.
	/// No-op when contentUri is empty or on non-Android platforms.
	void writeFileToUri(const QString & localPath, const QString & contentUri);
}
