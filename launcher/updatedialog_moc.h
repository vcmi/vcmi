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

private slots:
    void on_checkOnStartup_stateChanged(int state);

private:
	Ui::UpdateDialog *ui;
};

#endif // UPDATEDIALOG_MOC_H
