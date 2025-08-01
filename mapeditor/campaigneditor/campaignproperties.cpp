/*
 * campaignproperties.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "campaignproperties.h"
#include "ui_campaignproperties.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/campaign/CampaignRegionsHandler.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/json/JsonNode.h"

CampaignProperties::CampaignProperties(std::shared_ptr<CampaignState> campaignState):
	ui(new Ui::CampaignProperties),
	campaignState(campaignState),
	regions(campaignState->campaignRegions)
{
	ui->setupUi(this);

	setWindowTitle(tr("Campaign Properties"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->lineEditName->setText(QString::fromStdString(campaignState->name.toString()));
	ui->textEditDescription->setText(QString::fromStdString(campaignState->description.toString()));
	ui->lineEditAuthor->setText(QString::fromStdString(campaignState->author.toString()));
	ui->lineEditAuthorContact->setText(QString::fromStdString(campaignState->authorContact.toString()));
	ui->dateTimeEditCreationDateTime->setDateTime(QDateTime::fromSecsSinceEpoch(campaignState->creationDateTime));
	ui->lineEditCampaignVersion->setText(QString::fromStdString(campaignState->campaignVersion.toString()));
	ui->lineEditMusic->setText(QString::fromStdString(campaignState->music.getName()));
	ui->checkBoxScenarioDifficulty->setChecked(campaignState->difficultyChosenByPlayer);
	
	const JsonNode legacyRegionConfig(JsonPath::builtin("config/campaign_regions.json"));
	int legacyRegionNumber = legacyRegionConfig["campaign_regions"].Vector().size();

	for (int i = 0; i < legacyRegionNumber; i++)
		ui->comboBoxRegionPreset->insertItem(i, QString::fromStdString(LIBRARY->generaltexth->translate("core.camptext.names", i)));
	ui->comboBoxRegionPreset->insertItem(legacyRegionNumber, tr("Custom"));
	ui->comboBoxRegionPreset->setCurrentIndex(20);

	loadRegion();

	show();
}

CampaignProperties::~CampaignProperties()
{
	delete ui;
}

bool CampaignProperties::showCampaignProperties(std::shared_ptr<CampaignState> campaignState)
{
	if(!campaignState)
		return false;

	auto * dialog = new CampaignProperties(campaignState);

	dialog->setAttribute(Qt::WA_DeleteOnClose);

	return dialog->exec() == QDialog::Accepted;
}

void CampaignProperties::on_buttonBox_clicked(QAbstractButton * button)
{
	if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
	{
		if(!saveRegion())
			return;
		campaignState->name = MetaString::createFromRawString(ui->lineEditName->text().toStdString());
		campaignState->description = MetaString::createFromRawString(ui->textEditDescription->toPlainText().toStdString());
		campaignState->author = MetaString::createFromRawString(ui->lineEditAuthor->text().toStdString());
		campaignState->authorContact = MetaString::createFromRawString(ui->lineEditAuthorContact->text().toStdString());
		campaignState->creationDateTime = ui->dateTimeEditCreationDateTime->dateTime().toSecsSinceEpoch();
		campaignState->campaignVersion = MetaString::createFromRawString(ui->lineEditCampaignVersion->text().toStdString());
		campaignState->music = AudioPath::builtin(ui->lineEditMusic->text().toStdString());
		campaignState->difficultyChosenByPlayer = ui->checkBoxScenarioDifficulty->isChecked();
		accept();
	}
	close();
}

void CampaignProperties::on_comboBoxRegionPreset_currentIndexChanged(int index)
{
	if(ui->comboBoxRegionPreset->count() == 21 && ui->comboBoxRegionPreset->currentIndex() != 20)
		regions = *LIBRARY->campaignRegions->getByIndex(index);
	
	loadRegion();
}

void CampaignProperties::on_pushButtonRegionAdd_clicked()
{
	int row = ui->tableWidgetRegions->rowCount();
	ui->tableWidgetRegions->insertRow(row);
	ui->tableWidgetRegions->setItem(row, 0, new QTableWidgetItem("INFIX"));
	ui->tableWidgetRegions->setItem(row, 1, new QTableWidgetItem(QString::number(0)));
	ui->tableWidgetRegions->setItem(row, 2, new QTableWidgetItem(QString::number(0)));
	ui->tableWidgetRegions->setItem(row, 3, new QTableWidgetItem(QString::number(-1)));
	ui->tableWidgetRegions->setItem(row, 4, new QTableWidgetItem(QString::number(-1)));
}

void CampaignProperties::on_pushButtonRegionRemove_clicked()
{
	int rows = ui->tableWidgetRegions->rowCount() - 1;
	ui->tableWidgetRegions->removeRow(rows);
	ui->tableWidgetRegions->setRowCount(rows);
}

void CampaignProperties::loadRegion()
{
	ui->lineEditBackground->setText(QString::fromStdString(regions.campBackground.empty() ? regions.campPrefix + "_BG" : regions.campBackground));
	ui->lineEditSuffix1->setText(QString::fromStdString(regions.campSuffix.size() ? regions.campSuffix[0] : "En"));
	ui->lineEditSuffix2->setText(QString::fromStdString(regions.campSuffix.size() ? regions.campSuffix[1] : "Se"));
	ui->lineEditSuffix3->setText(QString::fromStdString(regions.campSuffix.size() ? regions.campSuffix[2] : "Co"));
	ui->lineEditPrefix->setText(QString::fromStdString(regions.campPrefix));
	ui->spinBoxColorSuffixLength->setValue(regions.colorSuffixLength);

	ui->tableWidgetRegions->clearContents();
	ui->tableWidgetRegions->setRowCount(0);
	ui->tableWidgetRegions->setColumnCount(5);
	ui->tableWidgetRegions->setHorizontalHeaderLabels({tr("Infix"), tr("X"), tr("Y"), tr("Label Pos X"), tr("Label Pos Y")});
	for (int i = 0; i < regions.regions.size(); ++i)
	{
		ui->tableWidgetRegions->insertRow(ui->tableWidgetRegions->rowCount());
		ui->tableWidgetRegions->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(regions.regions[i].infix)));
		ui->tableWidgetRegions->setItem(i, 1, new QTableWidgetItem(QString::number(regions.regions[i].pos.x)));
		ui->tableWidgetRegions->setItem(i, 2, new QTableWidgetItem(QString::number(regions.regions[i].pos.y)));
		ui->tableWidgetRegions->setItem(i, 3, new QTableWidgetItem(QString::number(regions.regions[i].labelPos.has_value() ? (*regions.regions[i].labelPos).x : -1)));
		ui->tableWidgetRegions->setItem(i, 4, new QTableWidgetItem(QString::number(regions.regions[i].labelPos.has_value() ? (*regions.regions[i].labelPos).y : -1)));
	}
	ui->tableWidgetRegions->resizeColumnsToContents();
}

bool CampaignProperties::saveRegion()
{
	regions.campBackground = ui->lineEditBackground->text().toStdString();
	if(regions.campSuffix.size() == 3)
	{
		regions.campSuffix[0] = ui->lineEditSuffix1->text().toStdString();
		regions.campSuffix[1] = ui->lineEditSuffix2->text().toStdString();
		regions.campSuffix[2] = ui->lineEditSuffix3->text().toStdString();
	}
	else
	{
		regions.campSuffix.push_back(ui->lineEditSuffix1->text().toStdString());
		regions.campSuffix.push_back(ui->lineEditSuffix2->text().toStdString());
		regions.campSuffix.push_back(ui->lineEditSuffix3->text().toStdString());
	}
	regions.campPrefix = ui->lineEditPrefix->text().toStdString();
	regions.colorSuffixLength = ui->spinBoxColorSuffixLength->value();

	regions.regions.clear();
	for (int i = 0; i < ui->tableWidgetRegions->rowCount(); ++i)
	{
		CampaignRegions::RegionDescription rd;

		rd.infix = ui->tableWidgetRegions->item(i, 0)->text().toStdString();
		rd.pos.x = ui->tableWidgetRegions->item(i, 1)->text().toInt();
		rd.pos.y = ui->tableWidgetRegions->item(i, 2)->text().toInt();
		auto labelX = ui->tableWidgetRegions->item(i, 3)->text().toInt();
		auto labelY = ui->tableWidgetRegions->item(i, 4)->text().toInt();
		if(labelX == -1 || labelY == -1)
			rd.labelPos = std::nullopt;
		else
			Point(labelX, labelY);

		regions.regions.push_back(rd);
	}

	if(campaignState->campaignRegions.regions.size() > regions.regions.size())
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, tr("Fewer Scenarios"), tr("New Region setup supports fewer scenarios than before. Some will removed. Continue?"), QMessageBox::Yes|QMessageBox::No);
		if (reply != QMessageBox::Yes)
			return false;
	}

	campaignState->campaignRegions = regions;

	while(campaignState->scenarios.size() < campaignState->campaignRegions.regions.size())
		campaignState->scenarios.emplace(CampaignScenarioID(std::prev(campaignState->scenarios.end())->first + 1), CampaignScenario());
	while(campaignState->scenarios.size() > campaignState->campaignRegions.regions.size())
	{
		auto elem = std::prev(campaignState->scenarios.end());
		campaignState->mapPieces.erase(elem->first);
		campaignState->scenarios.erase(elem);
	}
		
	
	return true;
}
