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
#include "main.h"
#include "mainwindow_moc.h"

#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include "../lib/VCMIDirs.h"

// Conan workaround https://github.com/conan-io/conan-center-index/issues/13332
#ifdef VCMI_IOS
#if __has_include("QIOSIntegrationPlugin.h")
#include "QIOSIntegrationPlugin.h"
#endif
#endif

int __argc;
char ** __argv;

int main(int argc, char * argv[])
{
	int result;
#ifdef VCMI_IOS
	__argc = argc;
	__argv = argv;
	{
#endif
	QApplication vcmilauncher(argc, argv);
	MainWindow mainWindow;
	mainWindow.show();
	result = vcmilauncher.exec();
#ifdef VCMI_IOS
	}
	if (result == 0)
		launchGame(__argc, __argv);
#endif
	return result;
}

void startGame(const QStringList & args)
{
	__argc = args.size() + 1; //first argument is omitted
	__argv = new char*[__argc];
	__argv[0] = new char[strlen("vcmi")];
	strcpy(__argv[0], "vcmi");
	for(int i = 1; i < __argc; ++i)
	{
		const char * s = args[i - 1].toLocal8Bit().constData();
		__argv[i] = new char[strlen(s)];
		strcpy(__argv[i], s);
	}
#ifdef Q_OS_IOS
	logGlobal->warn("Starting game with the arguments: %s", args.join(" ").toStdString());
	qApp->quit();
#else
	startExecutable(pathToQString(VCMIDirs::get().clientPath()), args);
#endif
}

#ifndef Q_OS_IOS
void startExecutable(QString name, const QStringList & args)
{
	QProcess process;
	
	// Start the executable
	if(process.startDetached(name, args))
	{
		qApp->quit();
	}
	else
	{
		QMessageBox::critical(qApp->activeWindow(),
							  "Error starting executable",
							  "Failed to start " + name + "\n"
							  "Reason: " + process.errorString(),
							  QMessageBox::Ok,
							  QMessageBox::Ok);
	}
}
#endif
