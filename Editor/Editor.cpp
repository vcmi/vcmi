#include "StdInc.h"
#include "Editor.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/Filesystem/CResourceLoader.h"

Editor::Editor(QWidget *parent)
	: QMainWindow(parent)
{
	logfile = new std::ofstream((GVCMIDirs.UserPath + "/VCMI_Editor_log.txt").c_str());
	console = new CConsoleHandler;

	preinitDLL(console,logfile);
	loadDLLClasses();

	ui.setupUi(this);

	createMenus();
}

Editor::~Editor()
{

}

void Editor::createMenus()
{
	QMenu * fileMenu = menuBar()->addMenu(tr("File"));
	QMenu * editMenu = menuBar()->addMenu(tr("Edit"));
	QMenu * viewMenu = menuBar()->addMenu(tr("View"));
	QMenu * toolsMenu = menuBar()->addMenu(tr("Tools"));
	QMenu * playerMenu = menuBar()->addMenu(tr("Player"));
	QMenu * helpMenu = menuBar()->addMenu(tr("Help"));
}
