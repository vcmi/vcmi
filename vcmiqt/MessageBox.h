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

#include <QMessageBox>
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
#endif
}
