#include "../lib/mapping/CMap.h"
#include "../lib/rmg/CRmgTemplateStorage.h"
#include "../lib/rmg/CRmgTemplate.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/VCMI_Lib.h"

#include "windownewmap.h"
#include "ui_windownewmap.h"
#include "mainwindow.h"
#include "generatorprogress.h"

WindowNewMap::WindowNewMap(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::WindowNewMap)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	setWindowModality(Qt::ApplicationModal);

	show();
}

WindowNewMap::~WindowNewMap()
{
	delete ui;
}

void WindowNewMap::on_cancelButton_clicked()
{
	close();
}

void generateRandomMap(CMapGenerator & gen, MainWindow * window, bool empty)
{
	window->setMapRaw(gen.generate(empty));
}

void WindowNewMap::on_okButtong_clicked()
{
	EWaterContent::EWaterContent water = EWaterContent::RANDOM;
	EMonsterStrength::EMonsterStrength monster = EMonsterStrength::RANDOM;
	if(ui->waterOpt1->isChecked())
		water = EWaterContent::RANDOM;
	if(ui->waterOpt2->isChecked())
		water = EWaterContent::NONE;
	if(ui->waterOpt3->isChecked())
		water = EWaterContent::NORMAL;
	if(ui->waterOpt4->isChecked())
		water = EWaterContent::ISLANDS;
	if(ui->monsterOpt1->isChecked())
		monster = EMonsterStrength::RANDOM;
	if(ui->monsterOpt2->isChecked())
		monster = EMonsterStrength::GLOBAL_WEAK;
	if(ui->monsterOpt3->isChecked())
		monster = EMonsterStrength::GLOBAL_NORMAL;
	if(ui->monsterOpt4->isChecked())
		monster = EMonsterStrength::GLOBAL_STRONG;

	mapGenOptions.setWaterContent(water);
	mapGenOptions.setMonsterStrength(monster);
	CMapGenerator generator(mapGenOptions);

	auto progressBarWnd = new GeneratorProgress(generator, this);
	progressBarWnd->show();

	{
		std::thread generate(&::generateRandomMap, std::ref(generator), static_cast<MainWindow*>(parent()), !ui->randomMapCheck->isChecked());
		progressBarWnd->update();
		generate.join();
	}

	static_cast<MainWindow*>(parent())->setMap(true);
	close();
}

void WindowNewMap::generateEmptyMap()
{

}

void WindowNewMap::generateRandomMap()
{

}

void WindowNewMap::on_sizeCombo_activated(int index)
{
	std::map<int, std::pair<int, int>> sizes
	{
		{0, {36, 36}},
		{1, {72, 72}},
		{2, {108, 108}},
		{3, {144, 144}},
	};

	ui->widthTxt->setText(QString::number(sizes[index].first));
	ui->heightTxt->setText(QString::number(sizes[index].second));
}


void WindowNewMap::on_twoLevelCheck_stateChanged(int arg1)
{
	bool twoLevel = ui->twoLevelCheck->isChecked();
	mapGenOptions.setHasTwoLevels(twoLevel);
	updateTemplateList();
}


void WindowNewMap::on_humanCombo_activated(int index)
{
	const int playerLimit = 8;
	std::map<int, int> players
	{
		{0, CMapGenOptions::RANDOM_SIZE},
		{1, 1},
		{2, 2},
		{3, 3},
		{4, 4},
		{5, 5},
		{6, 6},
		{7, 7},
		{8, 8}
	};

	int humans = players[index];
	if(humans > playerLimit)
	{
		humans = playerLimit;
		ui->humanCombo->setCurrentIndex(humans);
		return;
	}

	mapGenOptions.setPlayerCount(humans);

	int teams = mapGenOptions.getTeamCount();
	if(teams > humans - 1)
	{
		teams = humans - 1;
		//TBD
	}

	int cpu = mapGenOptions.getCompOnlyPlayerCount();
	if(cpu > playerLimit - humans)
	{
		cpu = playerLimit - humans;
		ui->cpuCombo->setCurrentIndex(cpu + 1);
	}

	int cpuTeams = mapGenOptions.getCompOnlyTeamCount(); //comp only players - 1
	if(cpuTeams > cpu - 1)
	{
		cpuTeams = cpu - 1;
		//TBD
	}

	//void setMapTemplate(const CRmgTemplate * value);
	updateTemplateList();
}


void WindowNewMap::on_cpuCombo_activated(int index)
{
	const int playerLimit = 8;
	std::map<int, int> players
	{
		{0, CMapGenOptions::RANDOM_SIZE},
		{1, 0},
		{2, 1},
		{3, 2},
		{4, 3},
		{5, 4},
		{6, 5},
		{7, 6},
		{8, 7}
	};

	int humans = mapGenOptions.getPlayerCount();
	int cpu = players[index];
	if(cpu > playerLimit - humans)
	{
		cpu = playerLimit - humans;
		ui->cpuCombo->setCurrentIndex(cpu + 1);
		return;
	}

	mapGenOptions.setCompOnlyPlayerCount(cpu);
	updateTemplateList();
}


void WindowNewMap::on_randomMapCheck_stateChanged(int arg1)
{
	randomMap = ui->randomMapCheck->isChecked();
	ui->templateCombo->setEnabled(randomMap);
	updateTemplateList();
}


void WindowNewMap::on_templateCombo_activated(int index)
{
	if(index == 0)
	{
		mapGenOptions.setMapTemplate(nullptr);
		return;
	}

	auto * templ = VLC->tplh->getTemplate(ui->templateCombo->currentText().toStdString());
	mapGenOptions.setMapTemplate(templ);
}


void WindowNewMap::on_widthTxt_textChanged(const QString &arg1)
{
	mapGenOptions.setWidth(arg1.toInt());
	updateTemplateList();
}


void WindowNewMap::on_heightTxt_textChanged(const QString &arg1)
{
	mapGenOptions.setHeight(arg1.toInt());
	updateTemplateList();
}

void WindowNewMap::updateTemplateList()
{
	ui->templateCombo->clear();
	ui->templateCombo->setCurrentIndex(-1);

	if(!randomMap)
		return;

	mapGenOptions.setMapTemplate(nullptr);
	auto templates = mapGenOptions.getPossibleTemplates();
	if(templates.empty())
		return;

	ui->templateCombo->addItem("[default]");

	for(auto * templ : templates)
	{
		ui->templateCombo->addItem(QString::fromStdString(templ->getName()));
	}

	ui->templateCombo->setCurrentIndex(0);
}
