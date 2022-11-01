/*
 * generatorprogress.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

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
