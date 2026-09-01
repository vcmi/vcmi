/*
 * MessageBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "vcmiqt.h"

#include <QGuiApplication>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>

namespace MessageBoxCustom
{
#ifdef VCMI_IOS
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651

	template<typename Functor>
	inline void showDialog(QWidget *parent, const Functor & f)
	{
		QTimer::singleShot(0, parent, f);
	}

	inline void information(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QTimer::singleShot(0, parent, [=](){
			QMessageBox::information(parent, title, text, buttons, defaultButton);
		});
	}

	inline void critical(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QTimer::singleShot(0, parent, [=](){
			QMessageBox::critical(parent, title, text, buttons, defaultButton);
		});
	}

#elif defined(VCMI_ANDROID)
	// Showing a native QMessageBox synchronously from a nested event loop (e.g. GOG extraction)
	// reaches QAndroidPlatformMessageDialogHelper while its JNI helper is null and triggers Qt's
	// JniAbort. Defer to a clean event-loop tick instead - this also naturally waits until the app
	// is in foreground, since a paused Android app does not process the posted event.
	namespace detail
	{
		inline void deferUntilActive(QPointer<QWidget> safe, std::function<void()> f)
		{
			QTimer::singleShot(0, qApp, [safe, f = std::move(f)]() {
				if(safe) f();
			});
		}
	}

	template<typename Functor>
	inline void showDialog(QWidget *parent, const Functor & f)
	{
		detail::deferUntilActive(parent, f);
	}

	inline void information(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QPointer<QWidget> safe(parent);
		detail::deferUntilActive(safe, [safe, title, text, buttons, defaultButton]{
			QMessageBox::information(safe, title, text, buttons, defaultButton);
		});
	}

	inline void critical(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QPointer<QWidget> safe(parent);
		detail::deferUntilActive(safe, [safe, title, text, buttons, defaultButton]{
			QMessageBox::critical(safe, title, text, buttons, defaultButton);
		});
	}

#else

	template<typename Functor>
	inline void showDialog(QWidget *parent, const Functor & f)
	{
		f();
	}

	inline void information(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QMessageBox::information(parent, title, text, buttons, defaultButton);
	}

	inline void critical(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QMessageBox::critical(parent, title, text, buttons, defaultButton);
	}
#endif
}
