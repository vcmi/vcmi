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

namespace MessageBox
{
#ifdef VCMI_IOS
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651

	template<typename Functor>
	void showDialog(const Functor & f)
	{
		QTimer::singleShot(0, this, f);
	}

	void information(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QTimer::singleShot(0, this, [=](){
			QMessageBox::information(parent, title, text, buttons, defaultButton);
		});
	}

#else

	template<typename Functor>
	void showDialog(const Functor & f)
	{
		f();
	}

	void information(QWidget *parent, const QString &title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton)
	{
		QMessageBox::information(parent, title, text, buttons, defaultButton);
	}
#endif
}
