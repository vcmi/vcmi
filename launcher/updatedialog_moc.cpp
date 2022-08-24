#include "updatedialog_moc.h"
#include "ui_updatedialog_moc.h"

UpdateDialog::UpdateDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::UpdateDialog)
{
	ui->setupUi(this);
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
