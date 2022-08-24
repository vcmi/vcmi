#include "updatedialog_moc.h"
#include "ui_updatedialog_moc.h"

#include "../lib/CConfigHandler.h"

UpdateDialog::UpdateDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::UpdateDialog)
{
	ui->setupUi(this);
	
	connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
	
	if(settings["launcher"]["updateOnStartup"].Bool() == true)
		ui->checkOnStartup->setCheckState(Qt::CheckState::Checked);
}

UpdateDialog::~UpdateDialog()
{
	delete ui;
}

void UpdateDialog::showUpdateDialog()
{
	UpdateDialog * dialog = new UpdateDialog;
	
	dialog->setWindowModality(Qt::ApplicationModal);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void UpdateDialog::on_checkOnStartup_stateChanged(int state)
{
	Settings node = settings.write["launcher"]["updateOnStartup"];
	node->Bool() = (state == 2 ? true : false);
}

