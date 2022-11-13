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
int argcForClient;
char ** argvForClient;
#endif

int main(int argc, char * argv[])
{
	int result;
#ifdef VCMI_IOS
	argcForClient = argc;
	argvForClient = argv;
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
	logGlobal->warn("Starting game with the arguments: %s", args.join(" ").toStdString());

#ifdef Q_OS_IOS
	argcForClient = args.size() + 1; //first argument is omitted
	argvForClient = new char*[argcForClient];
	argvForClient[0] = "vcmiclient";
	for(int i = 1; i < argcForClient; ++i)
	{
		const char * s = args[i - 1].toLocal8Bit().constData();
		argvForClient[i] = new char[strlen(s)];
		strcpy(argvForClient[i], s);
	}
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
