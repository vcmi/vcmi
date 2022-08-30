#ifndef WINDOWNEWMAP_H
#define WINDOWNEWMAP_H

#include <QDialog>
#include "../lib/rmg/CMapGenOptions.h"

namespace Ui {
class WindowNewMap;
}

class WindowNewMap : public QDialog
{
	Q_OBJECT

public:
	explicit WindowNewMap(QWidget *parent = nullptr);
	~WindowNewMap();

private slots:
	void on_cancelButton_clicked();

	void on_okButtong_clicked();

	void on_sizeCombo_activated(int index);

	void on_twoLevelCheck_stateChanged(int arg1);

	void on_humanCombo_activated(int index);

	void on_cpuCombo_activated(int index);

	void on_randomMapCheck_stateChanged(int arg1);

	void on_templateCombo_activated(int index);

	void on_widthTxt_textChanged(const QString &arg1);

	void on_heightTxt_textChanged(const QString &arg1);

private:

	void generateEmptyMap();
	void generateRandomMap();

	void updateTemplateList();

private:
	Ui::WindowNewMap *ui;

	CMapGenOptions mapGenOptions;
	bool randomMap = false;
};

#endif // WINDOWNEWMAP_H
