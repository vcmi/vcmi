/*
 * scenarioproperties.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "scenarioproperties.h"
#include "ui_scenarioproperties.h"
#include "startingbonus.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/constants/StringConstants.h"

ScenarioProperties::ScenarioProperties(std::shared_ptr<CampaignState> campaignState, CampaignScenarioID scenario):
	ui(new Ui::ScenarioProperties),
	campaignState(campaignState),
	map(campaignState->getMap(scenario, nullptr)),
	scenario(scenario)
{
	ui->setupUi(this);

	setWindowTitle(tr("Scenario Properties"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->lineEditRegionName->setText(getRegionChar(scenario.getNum()));
	ui->plainTextEditRightClickText->setPlainText(QString::fromStdString(campaignState->scenarios.at(scenario).regionText.toString()));
	
	for(int i = 0, index = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		MetaString str;
		str.appendName(PlayerColor(i));
		ui->comboBoxRegionColor->addItem(QString::fromStdString(str.toString()), QVariant(i));
		if(i == campaignState->scenarios.at(scenario).regionColor)
			ui->comboBoxRegionColor->setCurrentIndex(index);
		++index;
	}

	for(int i = 0, index = 0; i < 5; ++i)
	{
		ui->comboBoxDefaultDifficulty->addItem(QString::fromStdString(LIBRARY->generaltexth->arraytxt[142 + i]), QVariant(i));
		if(i == campaignState->scenarios.at(scenario).difficulty)
			ui->comboBoxDefaultDifficulty->setCurrentIndex(index);
		++index;
	}

	for(int i = 0; i < scenario.getNum(); ++i)
	{
		auto tmpScenario = CampaignScenarioID(i);
		auto * item = new QListWidgetItem(getRegionChar(tmpScenario.getNum()) + " - " + QString::fromStdString(campaignState->scenarios.at(tmpScenario).mapName));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(campaignState->scenarios.at(scenario).preconditionRegions.count(tmpScenario) ? Qt::Checked : Qt::Unchecked);
		ui->listWidgetPrerequisites->addItem(item);
	}

	ui->checkBoxPrologueEnabled->setChecked(campaignState->scenarios.at(scenario).prolog.hasPrologEpilog);
	ui->lineEditPrologueVideo->setText(QString::fromStdString(campaignState->scenarios.at(scenario).prolog.prologVideo.first.getName()));
	ui->lineEditPrologueVideoLoop->setText(QString::fromStdString(campaignState->scenarios.at(scenario).prolog.prologVideo.second.getName()));
	ui->lineEditPrologueMusic->setText(QString::fromStdString(campaignState->scenarios.at(scenario).prolog.prologMusic.getName()));
	ui->lineEditPrologueVoice->setText(QString::fromStdString(campaignState->scenarios.at(scenario).prolog.prologVoice.getName()));
	ui->plainTextEditPrologueText->setPlainText(QString::fromStdString(campaignState->scenarios.at(scenario).prolog.prologText.toString()));
	ui->checkBoxEpilogueEnabled->setChecked(campaignState->scenarios.at(scenario).epilog.hasPrologEpilog);
	ui->lineEditEpilogueVideo->setText(QString::fromStdString(campaignState->scenarios.at(scenario).epilog.prologVideo.first.getName()));
	ui->lineEditEpilogueVideoLoop->setText(QString::fromStdString(campaignState->scenarios.at(scenario).epilog.prologVideo.second.getName()));
	ui->lineEditEpilogueMusic->setText(QString::fromStdString(campaignState->scenarios.at(scenario).epilog.prologMusic.getName()));
	ui->lineEditEpilogueVoice->setText(QString::fromStdString(campaignState->scenarios.at(scenario).epilog.prologVoice.getName()));
	ui->plainTextEditEpilogueText->setPlainText(QString::fromStdString(campaignState->scenarios.at(scenario).epilog.prologText.toString()));

	ui->checkBoxCrossoverExperience->setChecked(campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.experience);
	ui->checkBoxCrossoverPrimarySkills->setChecked(campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.primarySkills);
	ui->checkBoxCrossoverSecondarySkills->setChecked(campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.secondarySkills);
	ui->checkBoxCrossoverSpells->setChecked(campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.spells);
	ui->checkBoxCrossoverArtifacts->setChecked(campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.artifacts);

	auto campaignStartOption = campaignState->scenarios.at(scenario).travelOptions.startOptions;
	ui->radioButtonStartingOptionBonus->setChecked(campaignStartOption == CampaignStartOptions::START_BONUS || campaignStartOption == CampaignStartOptions::NONE);
	ui->radioButtonStartingOptionHeroCrossover->setChecked(campaignStartOption == CampaignStartOptions::HERO_CROSSOVER);
	ui->radioButtonStartingOptionStartingHero->setChecked(campaignStartOption == CampaignStartOptions::HERO_OPTIONS);

	for(auto const & objectPtr : LIBRARY->arth->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(campaignState->scenarios.at(scenario).travelOptions.artifactsKeptByHero.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listWidgetCrossoverArtifacts->addItem(item);
	}
	
	for(auto const & objectPtr : LIBRARY->creh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameSingularTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(campaignState->scenarios.at(scenario).travelOptions.monstersKeptByHero.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listWidgetCrossoverCreatures->addItem(item);
	}

	ui->tabWidgetStartOptions->tabBar()->hide();
	ui->tabWidgetStartOptions->setStyleSheet("QTabWidget::pane { border: 0; }");

	ui->tableWidgetStartingCrossover->setColumnCount(2);

	LIBRARY->heroTypes()->forEach([this](const HeroType * hero, bool &)
	{
		heroSelection.emplace(hero->getId().getNum(), hero->getNameTranslated());
	});
	heroSelection.emplace(HeroTypeID::CAMP_STRONGEST, tr("Strongest").toStdString());
	heroSelection.emplace(HeroTypeID::CAMP_GENERATED, tr("Generated").toStdString());
	heroSelection.emplace(HeroTypeID::CAMP_RANDOM, tr("Random").toStdString());

	reloadMapRelatedUi();

	show();
}

ScenarioProperties::~ScenarioProperties()
{
	delete ui;
}

void ScenarioProperties::reloadMapRelatedUi()
{
	map = campaignState->getMap(scenario, nullptr);

	ui->lineEditMapFile->setText(QString::fromStdString(campaignState->scenarios.at(scenario).mapName));
	ui->lineEditScenarioName->setText(map ? QString::fromStdString(map->name.toString()) : tr("No map"));
	if(!map)
		ui->lineEditScenarioName->setStyleSheet("background-color: red;");
	else
		ui->lineEditScenarioName->setStyleSheet(""); 

	ui->pushButtonExport->setEnabled(map != nullptr);
	ui->pushButtonRemove->setEnabled(map != nullptr);

	ui->radioButtonStartingOptionHeroCrossover->setEnabled(scenario.getNum() > 0);
	bool allowSelectingStartHero = false;
	if(map != nullptr)
		for(auto & player : map->players)
			if(player.generateHeroAtMainTown)
				allowSelectingStartHero = true;
	ui->radioButtonStartingOptionStartingHero->setEnabled(allowSelectingStartHero);

	ui->tabPrologueEpilogue->setEnabled(map != nullptr);
	ui->tabCrossover->setEnabled(map != nullptr);
	ui->tabStarting->setEnabled(map != nullptr);

	ui->comboBoxDefaultDifficulty->setEnabled(map != nullptr);
	ui->listWidgetPrerequisites->setEnabled(map != nullptr);
	ui->plainTextEditRightClickText->setEnabled(map != nullptr);

	ui->tableWidgetStartingCrossover->clearContents();
	ui->tableWidgetStartingCrossover->setRowCount(0);
	ui->listWidgetStartingBonusOption->clear();

	if(map != nullptr)
	{
		std::vector<PlayerColor> selectableColors;
		for(int i = 0; i < map->players.size(); i++)
			if(map->players[i].canHumanPlay)
				selectableColors.push_back(PlayerColor(i));

		ui->comboBoxStartingBonusPlayerPosition->clear();
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		{
			if(!vstd::contains(selectableColors, PlayerColor(i)))
				continue;

			MetaString str;
			str.appendRawString((tr("Player") + " ").toStdString());
			str.appendNumber(i + 1);
			str.appendRawString(" (");
			str.appendName(PlayerColor(i));
			str.appendRawString(")");
			ui->comboBoxStartingBonusPlayerPosition->addItem(QString::fromStdString(str.toString()), QVariant(i));
		}
		int index = ui->comboBoxStartingBonusPlayerPosition->findData(campaignState->scenarios.at(scenario).travelOptions.playerColor.getNum());
		if(index != -1)
			ui->comboBoxStartingBonusPlayerPosition->setCurrentIndex(index);
	}

	if(campaignState->scenarios.at(scenario).travelOptions.startOptions == CampaignStartOptions::HERO_CROSSOVER || campaignState->scenarios.at(scenario).travelOptions.startOptions == CampaignStartOptions::HERO_OPTIONS)
	{
		if(campaignState->scenarios.at(scenario).travelOptions.startOptions == CampaignStartOptions::HERO_OPTIONS)
			on_radioButtonStartingOptionStartingHero_toggled();
		else
			on_radioButtonStartingOptionHeroCrossover_toggled();

		for(int i = 0; i < campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose.size(); i++)
		{
			auto bonus = campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose[i];

			ui->tableWidgetStartingCrossover->insertRow(i);
			QComboBox* comboBoxOption = new QComboBox();
			if(campaignState->scenarios.at(scenario).travelOptions.startOptions == CampaignStartOptions::HERO_OPTIONS)
			{
				for(auto & hero : heroSelection)
					comboBoxOption->addItem(QString::fromStdString(hero.second), hero.first);
			}
			else
			{
				for(int j = 0; j < scenario.getNum(); ++j)
				{
					auto tmpScenario = CampaignScenarioID(j);
					auto text = getRegionChar(tmpScenario.getNum()) + " - " + QString::fromStdString(campaignState->scenarios.at(tmpScenario).mapName);
					comboBoxOption->addItem(text, j);
				}
			}

			QComboBox* comboBoxPlayer = new QComboBox();
			for(int i = 0; i < ui->comboBoxStartingBonusPlayerPosition->count(); ++i) // copy from player dropdown
				comboBoxPlayer->addItem(ui->comboBoxStartingBonusPlayerPosition->itemText(i), ui->comboBoxStartingBonusPlayerPosition->itemData(i));

			const auto & bonusValue = bonus.getValue<CampaignBonusHeroesFromScenario>();

			// set selected
			int index = comboBoxPlayer->findData(bonusValue.startingPlayer.getNum());
			if(index != -1)
				comboBoxPlayer->setCurrentIndex(index);
			index = comboBoxOption->findData(bonusValue.scenario.getNum());
			if(index != -1)
				comboBoxOption->setCurrentIndex(index);

			ui->tableWidgetStartingCrossover->setCellWidget(i, 0, comboBoxOption);
			ui->tableWidgetStartingCrossover->setCellWidget(i, 1, comboBoxPlayer);
		}

		ui->tableWidgetStartingCrossover->resizeColumnsToContents();
	}
	else if(campaignState->scenarios.at(scenario).travelOptions.startOptions == CampaignStartOptions::START_BONUS)
	{
		ui->listWidgetStartingBonusOption->clear();
		for(auto & bonus : campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose)
		{
			QListWidgetItem * item = new QListWidgetItem(StartingBonus::getBonusListTitle(bonus, map));
			item->setData(Qt::UserRole, QVariant::fromValue(bonus));
			ui->listWidgetStartingBonusOption->addItem(item);
		}
	}
	reloadEnableState();
}

void ScenarioProperties::reloadEnableState()
{
	ui->pushButtonStartingAdd->setEnabled(ui->tableWidgetStartingCrossover->rowCount() < 3 && ui->listWidgetStartingBonusOption->count() < 3);
	ui->pushButtonStartingRemove->setEnabled(ui->tableWidgetStartingCrossover->rowCount() > 0 || ui->listWidgetStartingBonusOption->count() > 0);
}

bool ScenarioProperties::showScenarioProperties(std::shared_ptr<CampaignState> campaignState, CampaignScenarioID scenario)
{
	if(!campaignState || scenario == CampaignScenarioID::NONE)
		return false;

	auto * dialog = new ScenarioProperties(campaignState, scenario);

	dialog->setAttribute(Qt::WA_DeleteOnClose);

	return dialog->exec() == QDialog::Accepted;
}


void ScenarioProperties::on_buttonBox_clicked(QAbstractButton * button)
{
	if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
	{
		campaignState->scenarios.at(scenario).regionText = MetaString::createFromRawString(ui->plainTextEditRightClickText->toPlainText().toStdString());

		campaignState->scenarios.at(scenario).regionColor = ui->comboBoxRegionColor->currentData().toInt();
		campaignState->scenarios.at(scenario).difficulty = ui->comboBoxDefaultDifficulty->currentData().toInt();

		campaignState->scenarios.at(scenario).prolog.hasPrologEpilog = ui->checkBoxPrologueEnabled->isChecked();
		campaignState->scenarios.at(scenario).prolog.prologVideo.first = VideoPath::builtin(ui->lineEditPrologueVideo->text().toStdString());
		campaignState->scenarios.at(scenario).prolog.prologVideo.second = VideoPath::builtin(ui->lineEditPrologueVideoLoop->text().toStdString());
		campaignState->scenarios.at(scenario).prolog.prologMusic = AudioPath::builtin(ui->lineEditPrologueMusic->text().toStdString());
		campaignState->scenarios.at(scenario).prolog.prologVoice = AudioPath::builtin(ui->lineEditPrologueVoice->text().toStdString());
		campaignState->scenarios.at(scenario).prolog.prologText = MetaString::createFromRawString(ui->plainTextEditPrologueText->toPlainText().toStdString());
		campaignState->scenarios.at(scenario).epilog.hasPrologEpilog = ui->checkBoxEpilogueEnabled->isChecked();
		campaignState->scenarios.at(scenario).epilog.prologVideo.first = VideoPath::builtin(ui->lineEditEpilogueVideo->text().toStdString());
		campaignState->scenarios.at(scenario).epilog.prologVideo.second = VideoPath::builtin(ui->lineEditEpilogueVideoLoop->text().toStdString());
		campaignState->scenarios.at(scenario).epilog.prologMusic = AudioPath::builtin(ui->lineEditEpilogueMusic->text().toStdString());
		campaignState->scenarios.at(scenario).epilog.prologVoice = AudioPath::builtin(ui->lineEditEpilogueVoice->text().toStdString());
		campaignState->scenarios.at(scenario).epilog.prologText = MetaString::createFromRawString(ui->plainTextEditEpilogueText->toPlainText().toStdString());

		campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.experience = ui->checkBoxCrossoverExperience->isChecked();
		campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.primarySkills = ui->checkBoxCrossoverPrimarySkills->isChecked();
		campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.secondarySkills = ui->checkBoxCrossoverSecondarySkills->isChecked();
		campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.spells = ui->checkBoxCrossoverSpells->isChecked();
		campaignState->scenarios.at(scenario).travelOptions.whatHeroKeeps.artifacts = ui->checkBoxCrossoverArtifacts->isChecked();

		CampaignStartOptions startOption = CampaignStartOptions::NONE;
		if(ui->radioButtonStartingOptionBonus->isChecked())
			startOption = CampaignStartOptions::START_BONUS;
		else if(ui->radioButtonStartingOptionHeroCrossover->isChecked())
			startOption = CampaignStartOptions::HERO_CROSSOVER;
		else if(ui->radioButtonStartingOptionStartingHero->isChecked())
			startOption = CampaignStartOptions::HERO_OPTIONS;
		campaignState->scenarios.at(scenario).travelOptions.startOptions = startOption;

		campaignState->scenarios.at(scenario).travelOptions.artifactsKeptByHero.clear();
		for(int i = 0; i < ui->listWidgetCrossoverArtifacts->count(); ++i)
		{
			auto * item = ui->listWidgetCrossoverArtifacts->item(i);
			if(item->checkState() == Qt::Checked)
				campaignState->scenarios.at(scenario).travelOptions.artifactsKeptByHero.emplace(i);
		}

		campaignState->scenarios.at(scenario).travelOptions.monstersKeptByHero.clear();
		for(int i = 0; i < ui->listWidgetCrossoverCreatures->count(); ++i)
		{
			auto * item = ui->listWidgetCrossoverCreatures->item(i);
			if(item->checkState() == Qt::Checked)
				campaignState->scenarios.at(scenario).travelOptions.monstersKeptByHero.emplace(i);
		}

		campaignState->scenarios.at(scenario).preconditionRegions.clear();
		for(int i = 0; i < ui->listWidgetPrerequisites->count(); ++i)
		{
			auto * item = ui->listWidgetPrerequisites->item(i);
			if(item->checkState() == Qt::Checked)
				campaignState->scenarios.at(scenario).preconditionRegions.emplace(i);
		}

		campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose.clear();
		if(ui->radioButtonStartingOptionBonus->isChecked())
		{
			campaignState->scenarios.at(scenario).travelOptions.playerColor = PlayerColor(ui->comboBoxStartingBonusPlayerPosition->currentData().toInt());
			campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose.clear();
			for (int i = 0; i < ui->listWidgetStartingBonusOption->count(); ++i) {
				QListWidgetItem *item = ui->listWidgetStartingBonusOption->item(i);
				campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose.push_back(item->data(Qt::UserRole).value<CampaignBonus>());
			}
		}
		else if(ui->radioButtonStartingOptionHeroCrossover->isChecked() || ui->radioButtonStartingOptionStartingHero->isChecked())
		{
			for (int i = 0; i < ui->tableWidgetStartingCrossover->rowCount(); ++i)
			{
				QComboBox* comboBoxOption = qobject_cast<QComboBox*>(ui->tableWidgetStartingCrossover->cellWidget(i, 0));
				QComboBox* comboBoxPlayer = qobject_cast<QComboBox*>(ui->tableWidgetStartingCrossover->cellWidget(i, 1));
				CampaignBonus bonus;

				if (ui->radioButtonStartingOptionHeroCrossover->isChecked())
				{
					bonus = CampaignBonusHeroesFromScenario{
						PlayerColor(comboBoxPlayer->currentData().toInt()),
						CampaignScenarioID(comboBoxOption->currentData().toInt())
					};
				}
				else
				{
					bonus = CampaignBonusStartingHero{
						PlayerColor(comboBoxPlayer->currentData().toInt()),
						HeroTypeID(comboBoxOption->currentData().toInt())
					};
				}
				campaignState->scenarios.at(scenario).travelOptions.bonusesToChoose.push_back(bonus);
			}
		}

		accept();
	}
	close();
}

void ScenarioProperties::on_pushButtonCreatureTypeAll_clicked()
{
	for(int i = 0; i < ui->listWidgetCrossoverCreatures->count(); ++i)
		ui->listWidgetCrossoverCreatures->item(i)->setCheckState(Qt::CheckState::Checked);
}

void ScenarioProperties::on_pushButtonCreatureTypeNone_clicked()
{
	for(int i = 0; i < ui->listWidgetCrossoverCreatures->count(); ++i)
		ui->listWidgetCrossoverCreatures->item(i)->setCheckState(Qt::CheckState::Unchecked);
}

void ScenarioProperties::on_pushButtonImport_clicked()
{
	auto filename = QFileDialog::getOpenFileName(this, tr("Open map"), "", tr("All supported maps (*.vmap *.h3m);;VCMI maps(*.vmap);;HoMM3 maps(*.h3m)"));
	if(filename.isEmpty())
		return;

	QFile file(filename);
	
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::critical(nullptr, tr("Error"), tr("Could not open the file."));
		return;
	}
	
	QByteArray fileData = file.readAll();
	std::vector<uint8_t> byteArray(fileData.begin(), fileData.end());
	campaignState->mapPieces[scenario] = byteArray;

	QFileInfo fileInfo(filename);
	QString baseName = fileInfo.fileName();
	campaignState->scenarios.at(scenario).mapName = baseName.toStdString();

	reloadMapRelatedUi();
}

void ScenarioProperties::on_pushButtonExport_clicked()
{
	auto mapName = QString::fromStdString(campaignState->scenarios.at(scenario).mapName);
	bool isVmap = mapName.toLower().endsWith(".vmap");
	QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save map"), mapName, isVmap ? tr("VCMI maps (*.vmap);") : tr("HoMM3 maps (*.h3m);"));
	if (fileName.isEmpty())
		return;

	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::critical(nullptr, tr("Error"), tr("Could not open the file."));
		return;
	}

	auto byteArray = campaignState->mapPieces[scenario];
	QByteArray fileData(reinterpret_cast<const char*>(byteArray.data()), byteArray.size());

	file.write(fileData);
}

void ScenarioProperties::on_pushButtonRemove_clicked()
{
	campaignState->mapPieces.erase(scenario);
	campaignState->scenarios.at(scenario) = CampaignScenario();
	reloadMapRelatedUi();
}

void ScenarioProperties::on_radioButtonStartingOptionBonus_toggled()
{
	ui->tabWidgetStartOptions->setCurrentIndex(0);
	ui->listWidgetStartingBonusOption->clear();
	ui->pushButtonStartingEdit->setEnabled(true);
}

void ScenarioProperties::on_radioButtonStartingOptionHeroCrossover_toggled()
{
	ui->tabWidgetStartOptions->setCurrentIndex(1);
	ui->tableWidgetStartingCrossover->clearContents();
	ui->tableWidgetStartingCrossover->setRowCount(0);
	ui->tableWidgetStartingCrossover->setHorizontalHeaderLabels({tr("Source scenario"), tr("Player position")});
	ui->tableWidgetStartingCrossover->resizeColumnsToContents();
	ui->pushButtonStartingEdit->setEnabled(false);
}

void ScenarioProperties::on_radioButtonStartingOptionStartingHero_toggled()
{
	ui->tabWidgetStartOptions->setCurrentIndex(1);
	ui->tableWidgetStartingCrossover->clearContents();
	ui->tableWidgetStartingCrossover->setRowCount(0);
	ui->tableWidgetStartingCrossover->setHorizontalHeaderLabels({tr("Hero"), tr("Player position")});
	ui->tableWidgetStartingCrossover->resizeColumnsToContents();
	ui->pushButtonStartingEdit->setEnabled(false);
}

void ScenarioProperties::on_pushButtonStartingAdd_clicked()
{
	if(ui->radioButtonStartingOptionHeroCrossover->isChecked() || ui->radioButtonStartingOptionStartingHero->isChecked())
	{
		int row = ui->tableWidgetStartingCrossover->rowCount();
		ui->tableWidgetStartingCrossover->insertRow(row);

		QComboBox* comboBoxOption = new QComboBox();
		if(ui->radioButtonStartingOptionStartingHero->isChecked())
		{
			for(auto & hero : heroSelection)
				comboBoxOption->addItem(QString::fromStdString(hero.second), hero.first);
		}
		else
		{
			for(int i = 0; i < scenario.getNum(); ++i)
			{
				auto tmpScenario = CampaignScenarioID(i);
				auto text = getRegionChar(tmpScenario.getNum()) + " - " + QString::fromStdString(campaignState->scenarios.at(tmpScenario).mapName);
				comboBoxOption->addItem(text, i);
			}
		}
		QComboBox* comboBoxPlayer = new QComboBox();
		for(int i = 0; i < ui->comboBoxStartingBonusPlayerPosition->count(); ++i) // copy from player dropdown
			comboBoxPlayer->addItem(ui->comboBoxStartingBonusPlayerPosition->itemText(i), ui->comboBoxStartingBonusPlayerPosition->itemData(i));
		
		ui->tableWidgetStartingCrossover->setCellWidget(row, 0, comboBoxOption);
		ui->tableWidgetStartingCrossover->setCellWidget(row, 1, comboBoxPlayer);

		ui->tableWidgetStartingCrossover->resizeColumnsToContents();
	}
	else
	{
		CampaignBonus bonus = CampaignBonusSpell{ HeroTypeID(), SpellID() };

		if(StartingBonus::showStartingBonus(PlayerColor(ui->comboBoxStartingBonusPlayerPosition->currentData().toInt()), map, bonus))
		{
			QListWidgetItem * item = new QListWidgetItem(StartingBonus::getBonusListTitle(bonus, map));
			item->setData(Qt::UserRole, QVariant::fromValue(bonus));
			ui->listWidgetStartingBonusOption->addItem(item);
		}
	}
	reloadEnableState();
}

void ScenarioProperties::on_pushButtonStartingEdit_clicked()
{
	if(ui->radioButtonStartingOptionBonus->isChecked())
	{
		QList<QListWidgetItem *> selectedItems = ui->listWidgetStartingBonusOption->selectedItems();
		QListWidgetItem * item = selectedItems.takeFirst();

		CampaignBonus bonus = item->data(Qt::UserRole).value<CampaignBonus>();
		if(StartingBonus::showStartingBonus(PlayerColor(ui->comboBoxStartingBonusPlayerPosition->currentData().toInt()), map, bonus))
		{
			item->setText(StartingBonus::getBonusListTitle(bonus, map));
			item->setData(Qt::UserRole, QVariant::fromValue(bonus));
		}
	}
}

void ScenarioProperties::on_pushButtonStartingRemove_clicked()
{
	if(ui->radioButtonStartingOptionHeroCrossover->isChecked() || ui->radioButtonStartingOptionStartingHero->isChecked())
	{
		int rows = ui->tableWidgetStartingCrossover->rowCount() - 1;
		ui->tableWidgetStartingCrossover->removeRow(rows);
		ui->tableWidgetStartingCrossover->setRowCount(rows);

		ui->tableWidgetStartingCrossover->resizeColumnsToContents();
	}
	else if(ui->radioButtonStartingOptionBonus->isChecked())
	{
		QList<QListWidgetItem *> selectedItems = ui->listWidgetStartingBonusOption->selectedItems();
		for (QListWidgetItem *item : selectedItems)
			delete ui->listWidgetStartingBonusOption->takeItem(ui->listWidgetStartingBonusOption->row(item));
	}
	reloadEnableState();
}

QString ScenarioProperties::getRegionChar(int no)
{
	return QString(static_cast<char>('A' + no));
}
