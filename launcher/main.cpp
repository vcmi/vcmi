#include "StdInc.h"
#include "mainwindow_moc.h"
#include <QApplication>
#include "sdldisplayquery.h"

int main(int argc, char *argv[])
{
	auto displayList = getDisplays();
	QApplication a(argc, argv);
	MainWindow w(displayList);
	w.show();
	
	return a.exec();
}
