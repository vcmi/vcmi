#include "StdInc.h"
#include "Editor.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/logging/CBasicLogConfigurator.h"

Editor::Editor(QWidget *parent)
	: QMainWindow(parent)
{
	// Setup default logging(enough for now)
	console = new CConsoleHandler;
	CBasicLogConfigurator logConfig(VCMIDirs::get().userCachePath() / "VCMI_Editor_log.txt", console);
	logConfig.configureDefault();

	preinitDLL(console);
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
	std::map<std::string, std::function<void()> > actions; //connect these to actions
	enum MenuName {FILE=0, EDIT, VIEW, TOOLS, PLAYER, HELP, //main level
		TERRAIN=0, RIVER, ROADS, ERASE, OBSTACLES, OBJECTS}; //tools submenus


	//txts are in wrong order
	std::swap(txtEditor[793], txtEditor[797]);
	std::swap(txtEditor[794], txtEditor[797]);
	std::swap(txtEditor[795], txtEditor[797]);
	std::swap(txtEditor[796], txtEditor[797]);

	//setting up actions

	actions["file|1"] = [&]()
	{
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
			"",
			tr("Files (*.h3m)"));

		std::ifstream is;
		is.open (fileName.toStdString().c_str(), std::ios::binary );

		// get length of file
		is.seekg (0, std::ios::end);
		int length = is.tellg();
		is.seekg (0, std::ios::beg);

		char* buffer = new char [length];
		is.read (buffer, length);
		is.close();

		map = CMapService::loadMap((ui8*)buffer, length);
	};

	//setting up menus

	QMenu * menus[6];
	for(int i=0; i<6; ++i)
		menus[i] = menuBar()->addMenu(tr(txtEditor[751+i].c_str()));

	auto addMenu = [&](QMenu ** menuList, int txtBegin, int count, std::string actionBase, MenuName mn, const std::vector<int> & separators)
	{
		for(int i=0; i<count; ++i)
		{
			if(vstd::contains(separators, i))
				menuList[mn]->addSeparator();
			QAction * qa = new QAction(tr(txtEditor[txtBegin+i].c_str()), menus[mn]);
			std::string actionName = actionBase + "|" + boost::lexical_cast<std::string>(i);
			if(vstd::contains(actions, actionName))
			{
				QObject::connect(qa, &QAction::triggered, actions[actionName]);
			}
			menuList[mn]->addAction(qa);
		}
	};

	//terrain submenus
	QMenu* toolMenus[6];
	for(int i=0; i<6; ++i)
		toolMenus[i] = menus[TOOLS]->addMenu(tr(txtEditor[789+i].c_str()));

	using namespace boost::assign;
	std::vector<int> seps;
	seps += 4;

	addMenu(menus, 758, 6, "file", FILE, seps);
	
	seps.clear(); seps += 2, 6, 8;
	addMenu(menus, 860, 10, "edit", EDIT, seps);

	seps.clear(); seps += 2, 3, 7;
	addMenu(menus, 778, 10, "view", VIEW, seps);

	seps.clear(); seps += 0, 2;
	addMenu(menus, 795, 3, "tools", TOOLS, seps);

	seps.clear();
	addMenu(menus, 846, 9, "player", PLAYER, seps);

	seps.clear(); seps += 1;
	addMenu(menus, 856, 2, "help", HELP, seps);

	seps.clear(); seps += 4;
	addMenu(toolMenus, 799, 14, "tools|terrain", TERRAIN, seps);

	seps.clear();;
	addMenu(toolMenus, 814, 5, "tools|rivers", RIVER, seps);

	seps.clear();
	addMenu(toolMenus, 820, 4, "tools|roads", ROADS, seps);

	seps.clear();
	addMenu(toolMenus, 825, 4, "tools|erase", ERASE, seps);

	seps.clear(); seps += 16;
	addMenu(toolMenus, 872, 17, "tools|obstacles", OBSTACLES, seps);

	seps.clear();
	addMenu(toolMenus, 830, 15, "tools|objects", OBJECTS, seps);
}
