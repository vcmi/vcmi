#ifndef EDITOR_H
#define EDITOR_H

#include <QtWidgets/QMainWindow>
#include "ui_editor.h"

class CConsoleHandler;

class Editor : public QMainWindow
{
	Q_OBJECT

public:
	Editor(QWidget *parent = 0);
	~Editor();
	void createMenus();
private:

	std::ofstream * logfile;
	CConsoleHandler * console;

	Ui::EditorClass ui;
};

#endif // EDITOR_H
