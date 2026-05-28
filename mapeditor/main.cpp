/*
 * main.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include <QApplication>
#include "mainwindow.h"

#ifndef ENABLE_SINGLE_APP_BUILD
int main(int argc, char * argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

	QApplication vcmieditor(argc, argv);
	EditorMainWindow mainWindow;
	return vcmieditor.exec();
}
#endif
