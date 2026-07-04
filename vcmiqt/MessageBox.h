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
	// QAndroidPlatformMessageDialogHelper holds a JNI ref to a Java helper that
	// becomes null while the Activity is paused; calling QMessageBox in that
	// window triggers Qt's JniAbort. Defer until app is in foreground.
	namespace detail
	{
		inline void deferUntilActive(QPointer<QWidget> safe, std::function<void()> f)
		{
			auto fire = [safe, f = std::move(f)]() {
				if(safe) f();
			};
			if(QGuiApplication::applicationState() == Qt::ApplicationActive)
			{
				fire();
				return;
			}
			auto *holder = new QMetaObject::Connection;
			*holder = QObject::connect(qApp, &QGuiApplication::applicationStateChanged, qApp,
				[fire, holder](Qt::ApplicationState s) {
					if(s != Qt::ApplicationActive) return;
					QObject::disconnect(*holder);
					delete holder;
					fire();
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
