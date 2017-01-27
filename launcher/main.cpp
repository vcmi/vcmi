#include <QApplication>
#include "StdInc.h"
#include "mainwindow_moc.h"
#include "sdldisplayquery.h"

int main(int argc, char *argv[])
{
	QApplication vcmilauncher(argc, argv);
	auto displayList = getDisplays();
	MainWindow mainWindow(displayList);
	mainWindow.show();
	return vcmilauncher.exec();
}
