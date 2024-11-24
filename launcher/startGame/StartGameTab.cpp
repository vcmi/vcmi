#include "StartGameTab.h"
#include "ui_StartGameTab.h"

#include "../mainwindow_moc.h"
#include "../main.h"

StartGameTab::StartGameTab(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::StartGameTab)
{
	ui->setupUi(this);
}

StartGameTab::~StartGameTab()
{
	delete ui;
}

MainWindow * StartGameTab::getMainWindow()
{
	foreach(QWidget *w, qApp->allWidgets())
		if(QMainWindow* mainWin = qobject_cast<QMainWindow*>(w))
			return dynamic_cast<MainWindow *>(mainWin);
	return nullptr;
}

void StartGameTab::on_buttonPlay_clicked()
{
	getMainWindow()->hide();
	startGame({});
}

