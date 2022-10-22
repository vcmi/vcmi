/*
 * generatorprogress.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "generatorprogress.h"
#include "ui_generatorprogress.h"
#include <thread>
#include <chrono>

GeneratorProgress::GeneratorProgress(Load::Progress & source, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GeneratorProgress),
	source(source)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	setWindowFlags(Qt::Window);

	show();
}

GeneratorProgress::~GeneratorProgress()
{
	delete ui;
}


void GeneratorProgress::update()
{
	while(!source.finished())
	{
		int status = float(source.get()) / 2.55f;
		ui->progressBar->setValue(status);
		qApp->processEvents();
	}

	//delete source;
	close();
}
