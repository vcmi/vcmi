/*
 * main.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "main.h"
#include "mainwindow_moc.h"

#include <QApplication>

// Conan workaround https://github.com/conan-io/conan-center-index/issues/13332
#ifdef VCMI_IOS
#if __has_include("QIOSIntegrationPlugin.h")
#include "QIOSIntegrationPlugin.h"
#endif
#endif

int main(int argc, char * argv[])
{
	int result;
#ifdef VCMI_IOS
	{
#endif
	QApplication vcmilauncher(argc, argv);
	MainWindow mainWindow;
	mainWindow.show();
	result = vcmilauncher.exec();
#ifdef VCMI_IOS
	}
	if (result == 0)
		launchGame(argc, argv);
#endif
	return result;
}
