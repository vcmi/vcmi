#ifndef UPDATEDIALOG_MOC_H
#define UPDATEDIALOG_MOC_H

#include <QDialog>

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
	Q_OBJECT

public:
	explicit UpdateDialog(QWidget *parent = nullptr);
	~UpdateDialog();
	
	static void showUpdateDialog();

private:
	Ui::UpdateDialog *ui;
};

#endif // UPDATEDIALOG_MOC_H
