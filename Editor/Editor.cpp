#include "StdInc.h"
#include "Editor.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/Filesystem/CResourceLoader.h"
#include "../lib/CGeneralTextHandler.h"

Editor::Editor(QWidget *parent)
	: QMainWindow(parent)
{
	logfile = new std::ofstream((GVCMIDirs.UserPath + "/VCMI_Editor_log.txt").c_str());
	console = new CConsoleHandler;

	preinitDLL(console,logfile);
	loadDLLClasses();

	VLC->generaltexth->readToVector("DATA/EDITOR", txtEditor);
	VLC->generaltexth->readToVector("DATA/EDITRCMD", txtEditorCmd);

	ui.setupUi(this);

	createMenus();
}

Editor::~Editor()
{

}

void Editor::createMenus()
{
	enum MenuName {FILE, EDIT, VIEW, TOOLS, PLAYER, HELP};
	QMenu * menus[6];
	for(int i=0; i<6; ++i)
		menus[i] = menuBar()->addMenu(tr(txtEditor[751+i].c_str()));

	for(int i=0; i<6; ++i)
	{
		if(i == 4)
			menus[FILE]->addSeparator();
		QAction * qa = new QAction(tr(txtEditor[758+i].c_str()), menus[FILE]);
		menus[FILE]->addAction(qa);
	}

	for(int i=0; i<10; ++i)
	{
		if(i == 2 || i == 6 || i == 9)
			menus[EDIT]->addSeparator();
		QAction * qa = new QAction(tr(txtEditor[860+i].c_str()), menus[EDIT]);
		menus[EDIT]->addAction(qa);
	}

	for(int i=0; i<10; ++i)
	{
		if(i == 2 || i == 3 || i == 7)
			menus[VIEW]->addSeparator();
		QAction * qa = new QAction(tr(txtEditor[778+i].c_str()), menus[VIEW]);
		menus[VIEW]->addAction(qa);
	}

	for(int i=0; i<9; ++i)
	{
		if(i == 6 || i == 8)
			menus[TOOLS]->addSeparator();
		QAction * qa = new QAction(tr(txtEditor[789+i].c_str()), menus[TOOLS]);
		menus[TOOLS]->addAction(qa);
	}

	for(int i=0; i<9; ++i)
	{
		QAction * qa = new QAction(tr(txtEditor[846+i].c_str()), menus[PLAYER]);
		menus[PLAYER]->addAction(qa);
	}

	for(int i=0; i<2; ++i)
	{
		if(i == 1)
			menus[HELP]->addSeparator();
		QAction * qa = new QAction(tr(txtEditor[856+i].c_str()), menus[HELP]);
		menus[HELP]->addAction(qa);
	}
}
