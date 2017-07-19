/*
 * main.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include <QApplication>
#include "StdInc.h"
#include "mainwindow_moc.h"
#include "sdldisplayquery.h"

int main(int argc, char * argv[])
{
	QApplication vcmilauncher(argc, argv);
	auto displayList = getDisplays();
	MainWindow mainWindow(displayList);
	mainWindow.show();
	return vcmilauncher.exec();
}
