/*
 * campaigneditor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "campaigneditor.h"
#include "ui_campaigneditor.h"

#include "campaignproperties.h"
#include "scenarioproperties.h"

#include "../BitmapHandler.h"
#include "../helper.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/campaign/CampaignRegionsHandler.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/ModIncompatibility.h"
#include "../../lib/texts/CGeneralTextHandler.h"

CampaignEditor::CampaignEditor():
	ui(new Ui::CampaignEditor),
	selectedScenario(CampaignScenarioID::NONE)
{
	ui->setupUi(this);
	
	setWindowIcon(QIcon{":/icons/menu-game.png"});
	ui->actionOpen->setIcon(QIcon{":/icons/document-open.png"});
	ui->actionSave->setIcon(QIcon{":/icons/document-save.png"});
	ui->actionNew->setIcon(QIcon{":/icons/document-new.png"});
	ui->actionScenarioProperties->setIcon(QIcon{":/icons/menu-settings.png"});
	ui->actionCampaignProperties->setIcon(QIcon{":/icons/menu-mods.png"});
	ui->actionShowFullBackground->setIcon(QIcon{":/icons/tool-area.png"});

	ui->actionShowFullBackground->setCheckable(true);
	connect(ui->actionShowFullBackground, &QAction::triggered, [this](){ redraw(); });

	campaignScene.reset(new CampaignScene());
	ui->campaignView->setScene(campaignScene.get());

	redraw();

	setTitle();
	
	setWindowModality(Qt::ApplicationModal);

	show();
}

CampaignEditor::~CampaignEditor()
{
	delete ui;
}

void CampaignEditor::redraw()
{
	ui->actionSave->setEnabled(campaignState != nullptr);
	ui->actionSave_as->setEnabled(campaignState != nullptr);
	ui->actionScenarioProperties->setEnabled(campaignState != nullptr && campaignState->scenarios.count(selectedScenario));
	ui->actionCampaignProperties->setEnabled(campaignState != nullptr);

	if(!campaignState)
		return;

	campaignScene->clear();

	auto background = BitmapHandler::loadBitmap(campaignState->getRegions().getBackgroundName().getName());
	if(!ui->actionShowFullBackground->isChecked())
		background = background.copy(0, 0, 456, 600); 
	campaignScene->addItem(new QGraphicsPixmapItem(QPixmap::fromImage(background)));
	for (auto & s : campaignState->scenarios)
	{
		auto scenario = s.first;
		auto color = campaignState->scenarios.at(scenario).regionColor;
		auto image = BitmapHandler::loadBitmap(campaignState->getRegions().getAvailableName(scenario, color).getName());
		if(selectedScenario == scenario)
			image = BitmapHandler::loadBitmap(campaignState->getRegions().getSelectedName(scenario, color).getName());
		else if(campaignState->scenarios.at(scenario).mapName == "")
			image = BitmapHandler::loadBitmap(campaignState->getRegions().getConqueredName(scenario, color).getName());
		auto pixmap = new ClickablePixmapItem(QPixmap::fromImage(image), [this, scenario]()
		{
			bool redrawRequired = selectedScenario != scenario;
			selectedScenario = scenario;

			if(redrawRequired)
				redraw();
		}, [this, scenario]()
		{
			if(ScenarioProperties::showScenarioProperties(campaignState, scenario))
				changed();
			redraw();
		}, [this, scenario](QGraphicsSceneContextMenuEvent * event)
		{
			QMenu contextMenu(this);
			QAction *actionScenarioProperties = contextMenu.addAction(tr("Scenario editor"));
			actionScenarioProperties->setIcon(ui->actionScenarioProperties->icon());
			connect(actionScenarioProperties, &QAction::triggered, this, [this, scenario]() {
				if(ScenarioProperties::showScenarioProperties(campaignState, scenario))
					changed();
				redraw();
			});
			contextMenu.exec(event->screenPos());
		});
		auto pos = campaignState->getRegions().getPosition(scenario);
		pixmap->setPos(pos.x, pos.y);
		pixmap->setToolTip(QString::fromStdString(campaignState->scenarios.at(scenario).mapName));
		campaignScene->addItem(pixmap);
	}

	campaignScene->setSceneRect(background.rect());
	ui->campaignView->show();
}

bool CampaignEditor::getAnswerAboutUnsavedChanges()
{
	if(unsaved)
	{
		auto sure = QMessageBox::question(this, tr("Confirmation"), tr("Unsaved changes will be lost, are you sure?"));
		if(sure == QMessageBox::No)
		{
			return false;
		}
	}
	return true;
}

void CampaignEditor::setTitle()
{
	QFileInfo fileInfo(filename);
	QString title = QString("%1%2 - %3 (%4)").arg(fileInfo.fileName(), unsaved ? "*" : "", tr("VCMI Campaign Editor"), GameConstants::VCMI_VERSION.c_str());
	setWindowTitle(title);
}

void CampaignEditor::changed()
{
	unsaved = true;
	setTitle();
}

bool CampaignEditor::validate()
{
	if(campaignState->mapPieces.size() != campaignState->campaignRegions.regions.size())
		logGlobal->trace("Not all regions have a map");

	return true;
}

void CampaignEditor::saveCampaign()
{
	if(!validate())
		return;

	Helper::saveCampaign(campaignState, filename);
	unsaved = false;
}

void CampaignEditor::showCampaignEditor(QWidget *parent)
{
	auto * dialog = new CampaignEditor();

	dialog->move(parent->geometry().center() - dialog->rect().center());

	dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void CampaignEditor::on_actionOpen_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;
	
	auto filenameSelect = QFileDialog::getOpenFileName(this, tr("Open map"),
		QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string()),
		tr("All supported campaigns (*.vcmp *.h3c);;VCMI campaigns(*.vcmp);;HoMM3 campaigns(*.h3c)"));
	if(filenameSelect.isEmpty())
		return;
	
	campaignState = Helper::openCampaignInternal(filenameSelect);
	selectedScenario = *campaignState->allScenarios().begin();

	for(auto const & scenario : campaignState->allScenarios())
	{
		if(!CampaignEditor::tryToOpenMap(this, campaignState, scenario))
		{
			campaignState.reset();
			selectedScenario = CampaignScenarioID::NONE;
			return;
		}
	}

	while(campaignState->scenarios.size() < campaignState->campaignRegions.regions.size())
		campaignState->scenarios.emplace(CampaignScenarioID(std::prev(campaignState->scenarios.end())->first + 1), CampaignScenario()); // show as regions without scenario defined yet

	redraw();
}

void CampaignEditor::on_actionOpenSet_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;

	auto campaignSets = JsonUtils::assembleFromFiles("config/campaignSets.json");
	QMap<QString, QList<ResourcePath>> sets;
	for(auto const & set : campaignSets.Struct())
	{
		auto name = QString::fromStdString(set.second["text"].isNull() ? set.first : LIBRARY->generaltexth->translate(set.second["text"].String()));
		for(auto const & item : set.second["items"].Vector())
		{
			auto res = ResourcePath(item["file"].String(), EResType::CAMPAIGN);
			if(CResourceHandler::get()->existsResource(res))
				sets[name].append(res);
		}
	}

	QStringList setNames = sets.keys();
	bool ok = false;
	QString selectedSet = QInputDialog::getItem(this, tr("Open Campaign set"), tr("Select Campaign set"), setNames, 0, false, &ok);

	if(!ok)
		return;
	
	QMap<QString, ResourcePath> campaigns;
	for(auto const & campaign : sets.value(selectedSet))
	{
		auto c = CampaignHandler::getHeader(campaign.getName());
		campaigns.insert(QString::fromStdString(c->getNameTranslated()), campaign);
	}

	QString selectedCampaign = QInputDialog::getItem(this, tr("Open Campaign"), tr("Select Campaign"), campaigns.keys(), 0, false, &ok);

	if(!ok)
		return;
	
	campaignState = CampaignHandler::getCampaign(campaigns.find(selectedCampaign).value().getName());
	selectedScenario = *campaignState->allScenarios().begin();

	redraw();
}

void CampaignEditor::on_actionSave_as_triggered()
{
	if(!campaignState)
		return;

	auto filenameSelect = QFileDialog::getSaveFileName(this, tr("Save campaign"), "", tr("VCMI campaigns (*.vcmp)"));

	if(filenameSelect.isNull())
		return;

	QFileInfo fileInfo(filenameSelect);

	if(fileInfo.suffix().toLower() != "vcmp")
		filenameSelect += ".vcmp";

	filename = filenameSelect;
	saveCampaign();
	setTitle();
}

void CampaignEditor::on_actionNew_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;
	
	campaignState = std::make_unique<CampaignState>();
	campaignState->campaignRegions = *LIBRARY->campaignRegions->getByIndex(0);
	for (int i = 0; i < campaignState->campaignRegions.regions.size(); i++)
	{
		CampaignScenario s;
		s.travelOptions.startOptions = CampaignStartOptions::START_BONUS;
		campaignState->scenarios.emplace(CampaignScenarioID(i), s);
	}
	campaignState->modName = "mapEditor";
	campaignState->creationDateTime = std::time(nullptr);
	
	changed();
	redraw();
}

void CampaignEditor::on_actionSave_triggered()
{
	if(!campaignState)
		return;

	if(filename.isNull())
		on_actionSave_as_triggered();
	else 
		saveCampaign();
	setTitle();
}

void CampaignEditor::on_actionCampaignProperties_triggered()
{
	if(!campaignState)
		return;

	if(CampaignProperties::showCampaignProperties(campaignState))
		changed();
	redraw();
}

void CampaignEditor::on_actionScenarioProperties_triggered()
{
	if(!campaignState || selectedScenario == CampaignScenarioID::NONE)
		return;

	if(ScenarioProperties::showScenarioProperties(campaignState, selectedScenario))
		changed();
	redraw();
}

void CampaignEditor::closeEvent(QCloseEvent *event)
{
	if(getAnswerAboutUnsavedChanges())
		QWidget::closeEvent(event);
	else
		event->ignore();
}

std::unique_ptr<CMap> CampaignEditor::tryToOpenMap(QWidget* parent, std::shared_ptr<CampaignState> state, CampaignScenarioID scenario)
{
	try
	{
		auto map = state->getMap(scenario, nullptr);
		return map;
	}
	catch(const ModIncompatibility & e)
	{
		assert(e.whatExcessive().empty());
		auto qstrError = QString::fromStdString(e.getFullErrorMsg()).remove('{').remove('}');
		QMessageBox::warning(parent, tr("Mods are required"), qstrError);
		return nullptr;
	}
	catch(const IdentifierResolutionException & e)
	{
		MetaString errorMsg;
		errorMsg.appendTextID("vcmi.server.errors.campOrMapFile.unknownEntity");
		errorMsg.replaceRawString(e.identifierName);
		QMessageBox::critical(parent, tr("Failed to open map"), QString::fromStdString(errorMsg.toString()));
		return nullptr;
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(parent, tr("Failed to open map"), tr(e.what()));
		return nullptr;
	}
}
