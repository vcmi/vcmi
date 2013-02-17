#ifndef EDITOR_H
#define EDITOR_H

#include <QtWidgets/QMainWindow>
#include "ui_editor.h"

class CConsoleHandler;
class CMap;

class Editor : public QMainWindow
{
	Q_OBJECT

public:
	Editor(QWidget *parent = 0);
	~Editor();
	void createMenus();


	std::unique_ptr<CMap> map;
private:

	std::vector<std::string> txtEditor, txtEditorCmd;

	std::ofstream * logfile;
	CConsoleHandler * console;

	Ui::EditorClass ui;
};

#endif // EDITOR_H
