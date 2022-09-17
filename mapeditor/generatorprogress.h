#ifndef GENERATORPROGRESS_H
#define GENERATORPROGRESS_H

#include <QDialog>
#include "../lib/LoadProgress.h"

namespace Ui {
class GeneratorProgress;
}

class GeneratorProgress : public QDialog
{
	Q_OBJECT

public:
	explicit GeneratorProgress(Load::Progress & source, QWidget *parent = nullptr);
	~GeneratorProgress();

	void update();

private:
	Ui::GeneratorProgress *ui;
	Load::Progress & source;
};

#endif // GENERATORPROGRESS_H
