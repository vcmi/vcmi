/*
 * androidfilepicker.h, part of VCMI engine
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

#ifdef VCMI_ANDROID

/// Helper for Android-specific file dialogs in the map editor.
/// Provides a choice between internal (Maps folder) and external (SAF) file picking.
class AndroidFilePicker
{
public:
	enum class Mode
	{
		InternalOrExternal, // Ask user whether to use internal Maps folder or external SAF picker
		ExternalOnly        // Only use external SAF picker
	};

	/// Show an open-file dialog appropriate for Android.
	/// For InternalOrExternal mode: asks the user, then either browses the internal
	/// Maps folder or opens the native SAF picker.
	/// Returns the selected file path, or empty string on cancel.
	static QString getOpenFileName(QWidget * parent, const QString & title,
		const QString & internalDir, const QString & filter, Mode mode);

	/// Show a save-file dialog appropriate for Android.
	/// For InternalOrExternal mode: asks the user, then either saves to the internal
	/// Maps folder or saves via the native SAF picker.
	/// Returns the selected file path, or empty string on cancel.
	/// For external saves, the actual writing to the URI happens via writeFileToUri().
	static QString getSaveFileName(QWidget * parent, const QString & title,
		const QString & internalDir, const QString & filter, Mode mode,
		QString & outContentUri);

	/// After saving a file locally, copies it to the content:// URI from the SAF picker.
	/// Only needed when the save target was external.
	static void writeFileToUri(const QString & localPath, const QString & contentUri);

private:
	/// Opens the Android native file picker for reading via JNI.
	static QString nativeOpenFile(const QString & mimeType);

	/// Opens the Android native file picker for saving via JNI.
	static QString nativeSaveFile(const QString & mimeType, const QString & suggestedName);
};

#endif // VCMI_ANDROID
