/*
 * templateeditor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "templateeditor.h"
#include "ui_templateeditor.h"
#include "graphicelements/CardItem.h"
#include "graphicelements/LineItem.h"
#include "terrainselector.h"
#include "factionselector.h"
#include "mineselector.h"
#include "treasureselector.h"
#include "GeometryAlgorithm.h"

#include "../helper.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../../lib/texts/MetaString.h"

TemplateEditor::TemplateEditor():
	ui(new Ui::TemplateEditor)
{
	ui->setupUi(this);
	
	setWindowIcon(QIcon{":/icons/menu-game.png"});
	ui->actionOpen->setIcon(QIcon{":/icons/document-open.png"});
	ui->actionSave->setIcon(QIcon{":/icons/document-save.png"});
	ui->actionNew->setIcon(QIcon{":/icons/document-new.png"});
	ui->actionAddZone->setIcon(QIcon{":/icons/zone-add.svg"});
	ui->actionRemoveZone->setIcon(QIcon{":/icons/zone-remove.svg"});
	ui->actionAutoPosition->setIcon(QIcon{":/icons/zones-layout.svg"});
	ui->actionZoom_in->setIcon(QIcon{":/icons/zoom_plus.png"});
	ui->actionZoom_out->setIcon(QIcon{":/icons/zoom_minus.png"});
	ui->actionZoom_auto->setIcon(QIcon{":/icons/zoom_base.png"});
	ui->actionZoom_reset->setIcon(QIcon{":/icons/zoom_zero.png"});

	templateScene.reset(new TemplateScene());
	ui->templateView->setScene(templateScene.get());

	ui->toolBox->setVisible(false);

	ui->spinBoxZoneVisPosX->setSingleStep(CardItem::GRID_SIZE);
	ui->spinBoxZoneVisPosY->setSingleStep(CardItem::GRID_SIZE);

	ui->templateView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->templateView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->templateView->setSceneRect(-3000, -3000, 6000, 6000);
	ui->templateView->centerOn(QPointF(0, 0));
	ui->templateView->setDragMode(QGraphicsView::DragMode::ScrollHandDrag);

	loadContent();

	setTitle();
	
	setWindowModality(Qt::ApplicationModal);

	show();
}

TemplateEditor::~TemplateEditor()
{
	delete ui;
}

void TemplateEditor::initContent()
{
	ui->comboBoxTemplateSelection->clear();

	ui->toolBox->setVisible(false);
	ui->pageZone->setEnabled(false);

	if(templates.empty())
		return;

	ui->toolBox->setVisible(true);

	selectedTemplate = templates.begin()->first;
	for(auto & tpl : templates)
		ui->comboBoxTemplateSelection->addItem(QString::fromStdString(tpl.first));
}

void TemplateEditor::autoPositionZones()
{
	auto & zones = templates[selectedTemplate]->getZones();

	std::vector<GeometryAlgorithm::Node> nodes;
	std::default_random_engine rng(0);
	std::uniform_real_distribution<double> distX(0.0, 500);
	std::uniform_real_distribution<double> distY(0.0, 500);
	for(auto & item : zones)
	{
		GeometryAlgorithm::Node node;
		node.x = distX(rng);
		node.y = distY(rng);
		node.id = item.first;
		nodes.push_back(node);
	}
	std::vector<GeometryAlgorithm::Edge> edges;
	for(auto & item : templates[selectedTemplate]->getConnectedZoneIds())
		edges.push_back({
			vstd::find_pos_if(nodes, [item](auto & elem){ return elem.id == item.getZoneA(); }),
			vstd::find_pos_if(nodes, [item](auto & elem){ return elem.id == item.getZoneB(); })
		});
		
	GeometryAlgorithm::forceDirectedLayout(nodes, edges, 1000, 500, 500);

	for(auto & item : nodes)
		zones.at(item.id)->setVisiblePosition(Point(CardItem::GRID_SIZE * round(item.x / CardItem::GRID_SIZE), CardItem::GRID_SIZE * round(item.y / CardItem::GRID_SIZE)));
}

void TemplateEditor::loadContent(bool autoPosition)
{
	cards.clear();
	lines.clear();

	selectedZone = -1;

	templateScene->clear();

	if(templates.empty())
		return;

	auto & zones = templates[selectedTemplate]->getZones();
	if(autoPosition || std::all_of(zones.begin(), zones.end(), [](auto & item){ return item.second->getVisiblePosition().x == 0 && item.second->getVisiblePosition().y == 0; }))
		autoPositionZones();

	for(auto & zone : zones)
	{
		auto svgItem = new CardItem();
		svgItem->setSelectCallback([this, svgItem](bool selected){
			ui->pageZone->setDisabled(!selected);
			ui->toolBox->setCurrentIndex(selected ? 1 : 0);
			ui->actionRemoveZone->setEnabled(selected);
			if(selected)
			{
				selectedZone = svgItem->getId();
				loadZoneMenuContent();
			}
			else
				selectedZone = -1;
		});
		svgItem->setPosChangeCallback([this, svgItem](QPointF pos){
			updateConnectionLines();
			for(auto & zone : templates[selectedTemplate]->getZones())
				if(zone.first == svgItem->getId())
				{
					zone.second->setVisiblePosition(Point(svgItem->pos().toPoint().rx() + (svgItem->boundingRect().width() * svgItem->scale() / 2), svgItem->pos().toPoint().ry() + (svgItem->boundingRect().height() * svgItem->scale() / 2)));
					zone.second->setVisibleSize(svgItem->scale());
				}
			loadZoneMenuContent(true);
		});
		svgItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);

		templateScene->addItem(svgItem);
		cards[zone.first] = svgItem;

		updateZoneCards();
	}
	
	updateConnectionLines(true);
	updateZonePositions();

	ui->templateView->show();

	ui->toolBox->setCurrentIndex(0);
	connect(ui->toolBox, &QToolBox::currentChanged, this, [this](int index) mutable {
		if(!ui->pageZone->isEnabled() && index == 1)
			ui->toolBox->setCurrentIndex(0);
	});

	ui->lineEditName->setText(QString::fromStdString(templates[selectedTemplate]->getName()));
	ui->lineEditDescription->setText(QString::fromStdString(templates[selectedTemplate]->getDescription()));
	ui->spinBoxMinSizeX->setValue(templates[selectedTemplate]->getMapSizes().first.x);
	ui->spinBoxMinSizeY->setValue(templates[selectedTemplate]->getMapSizes().first.y);
	ui->spinBoxMinSizeZ->setValue(templates[selectedTemplate]->getMapSizes().first.z);
	ui->spinBoxMaxSizeX->setValue(templates[selectedTemplate]->getMapSizes().second.x);
	ui->spinBoxMaxSizeY->setValue(templates[selectedTemplate]->getMapSizes().second.y);
	ui->spinBoxMaxSizeZ->setValue(templates[selectedTemplate]->getMapSizes().second.z);
	ui->checkBoxAllowedWaterContentNone->setChecked(templates[selectedTemplate]->allowedWaterContent.count(EWaterContent::EWaterContent::NONE));
	ui->checkBoxAllowedWaterContentNormal->setChecked(templates[selectedTemplate]->allowedWaterContent.count(EWaterContent::EWaterContent::NORMAL));
	ui->checkBoxAllowedWaterContentIslands->setChecked(templates[selectedTemplate]->allowedWaterContent.count(EWaterContent::EWaterContent::ISLANDS));

	auto setPlayersTable = [this](QTableWidget * widget, std::vector<std::pair<int, int>> & range){
		widget->clear();
		widget->clearContents();
		widget->setRowCount(0);
		widget->setColumnCount(3);
		widget->setHorizontalHeaderLabels({ tr("Min"), tr("Max"), tr("Action") });
		widget->horizontalHeader()->setStretchLastSection(true);
		for(int i = 0; i < range.size(); i++)
		{
			widget->insertRow(i);
			QSpinBox* minSpin = new QSpinBox();
			QSpinBox* maxSpin = new QSpinBox();
			QPushButton* delButton = new QPushButton();
			minSpin->setRange(0, 999);
			maxSpin->setRange(0, 999);
			minSpin->setValue(range.at(i).first);
			maxSpin->setValue(range.at(i).second);
			connect(minSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [i, &range](int val){
				range.at(i).first = val;
			});
			connect(maxSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [i, &range](int val){
				range.at(i).second = val;
			});
			delButton->setText(tr("Delete"));
			connect(delButton, &QPushButton::clicked, this, [this, i, &range](){
				range.erase(range.begin() + i);
				loadContent();
			});
			widget->setCellWidget(i, 0, minSpin);
			widget->setCellWidget(i, 1, maxSpin);
			widget->setCellWidget(i, 2, delButton);
		}
		widget->insertRow(widget->rowCount());
		QPushButton* addButton = new QPushButton();
		addButton->setText(tr("Add"));
		connect(addButton, &QPushButton::clicked, this, [this, &range](){
			range.push_back(std::make_pair(0, 0));
			loadContent();
		});
		widget->setCellWidget(widget->rowCount() - 1, 2, addButton);
	};
	setPlayersTable(ui->tableWidgetPlayersPlayer, templates[selectedTemplate]->players.range);
	setPlayersTable(ui->tableWidgetPlayersHuman, templates[selectedTemplate]->humanPlayers.range);

	loadZoneConnectionMenuContent();
}

void TemplateEditor::updateZonePositions()
{
	for(auto & card : cards)
	{
		auto & zone = templates[selectedTemplate]->getZones().at(card.first);
		card.second->setPos(zone->getVisiblePosition().x - (card.second->boundingRect().width() * card.second->scale() / 2), zone->getVisiblePosition().y - (card.second->boundingRect().height() * card.second->scale() / 2));
		card.second->setScale(zone->getVisibleSize());
	}

	updateConnectionLines();
}

QString TemplateEditor::getZoneToolTip(std::shared_ptr<rmg::ZoneOptions> zone)
{
	QString tmp{};
	tmp.append(tr("ID: %1").arg(QString::number(zone->getId())));
	tmp.append("\n");
	tmp.append(tr("Max treasure: %1").arg(QString::number(zone->getMaxTreasureValue())));
	//TODO: extend other interesting things (terrains, towns, ...)
	return tmp;
}

void TemplateEditor::updateZoneCards(TRmgTemplateZoneId id)
{
	for(auto & card : cards)
	{
		if(card.first != id && id > -1)
			continue;

		auto & zone = templates[selectedTemplate]->getZones().at(card.first);
		auto type = zone->getType();

		if(type == ETemplateZoneType::PLAYER_START || type == ETemplateZoneType::CPU_START)
			card.second->setPlayerColor(PlayerColor(*zone->getOwner()));
		else if(type == ETemplateZoneType::TREASURE)
			card.second->setMultiFillColor(QColor(165, 125, 55), QColor(250, 229, 157));
		else if(type == ETemplateZoneType::WATER)
			card.second->setMultiFillColor(QColor(55, 68, 165), QColor(157, 250, 247));
		else
			card.second->setFillColor(QColor(200, 200, 200));
		card.second->setJunction(type == ETemplateZoneType::JUNCTION);

		card.second->setId(card.first);
		for(auto & res : {GameResID::WOOD, GameResID::ORE, GameResID::MERCURY, GameResID::SULFUR, GameResID::CRYSTAL, GameResID::GEMS, GameResID::GOLD})
			card.second->setResAmount(res, zone->getMinesInfo().count(res) ? zone->getMinesInfo().at(res) : 0);
		card.second->setChestValue(zone->getMaxTreasureValue());
		card.second->setMonsterStrength(zone->monsterStrength);
		card.second->setToolTip(getZoneToolTip(zone));
		card.second->setScale(zone->getVisibleSize());
		card.second->updateContent();
	}
}

void TemplateEditor::loadZoneMenuContent(bool onlyPosition)
{
	if(selectedZone < 0 || selectedTemplate.empty())
		return;
	
	auto setValue = [](auto& target, const auto& newValue){ target->setValue(newValue); };
	auto & zone = templates[selectedTemplate]->getZones().at(selectedZone);
	setValue(ui->spinBoxZoneVisPosX, zone->getVisiblePosition().x);
	setValue(ui->spinBoxZoneVisPosY, zone->getVisiblePosition().y);
	setValue(ui->doubleSpinBoxZoneVisSize, zone->getVisibleSize());
	
	if(onlyPosition)
		return;

	setValue(ui->spinBoxZoneSize, zone->getSize());
	setValue(ui->spinBoxTownCountPlayer, zone->playerTowns.townCount);
	setValue(ui->spinBoxCastleCountPlayer, zone->playerTowns.castleCount);
	setValue(ui->spinBoxTownDensityPlayer, zone->playerTowns.townDensity);
	setValue(ui->spinBoxCastleDensityPlayer, zone->playerTowns.castleDensity);
	setValue(ui->spinBoxTownCountNeutral, zone->neutralTowns.townCount);
	setValue(ui->spinBoxCastleCountNeutral, zone->neutralTowns.castleCount);
	setValue(ui->spinBoxTownDensityNeutral, zone->neutralTowns.townDensity);
	setValue(ui->spinBoxCastleDensityNeutral, zone->neutralTowns.castleDensity);
	ui->checkBoxMatchTerrainToTown->setChecked(zone->matchTerrainToTown);
	ui->checkBoxTownsAreSameType->setChecked(zone->townsAreSameType);
	ui->checkBoxZoneLinkTowns->setChecked(zone->townsLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->checkBoxZoneLinkMines->setChecked(zone->minesLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->checkBoxZoneLinkTerrain->setChecked(zone->terrainTypeLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->checkBoxZoneLinkTreasure->setChecked(zone->treasureLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->checkBoxZoneLinkCustomObjects->setChecked(zone->customObjectsLikeZone != rmg::ZoneOptions::NO_ZONE);
	setValue(ui->spinBoxZoneLinkTowns, zone->townsLikeZone != rmg::ZoneOptions::NO_ZONE ? zone->townsLikeZone : 0);
	setValue(ui->spinBoxZoneLinkMines, zone->minesLikeZone != rmg::ZoneOptions::NO_ZONE ? zone->minesLikeZone : 0);
	setValue(ui->spinBoxZoneLinkTerrain, zone->terrainTypeLikeZone != rmg::ZoneOptions::NO_ZONE ? zone->terrainTypeLikeZone : 0);
	setValue(ui->spinBoxZoneLinkTreasure, zone->treasureLikeZone != rmg::ZoneOptions::NO_ZONE ? zone->treasureLikeZone : 0);
	setValue(ui->spinBoxZoneLinkCustomObjects, zone->customObjectsLikeZone != rmg::ZoneOptions::NO_ZONE ? zone->customObjectsLikeZone : 0);
	ui->spinBoxZoneLinkTowns->setEnabled(zone->townsLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->spinBoxZoneLinkMines->setEnabled(zone->minesLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->spinBoxZoneLinkTerrain->setEnabled(zone->terrainTypeLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->spinBoxZoneLinkTreasure->setEnabled(zone->treasureLikeZone != rmg::ZoneOptions::NO_ZONE);
	ui->spinBoxZoneLinkCustomObjects->setEnabled(zone->customObjectsLikeZone != rmg::ZoneOptions::NO_ZONE);
	
	setValue(ui->spinBoxZoneId, zone->id);
	ui->spinBoxZoneId->setEnabled(false);

	ui->comboBoxZoneType->clear();
	ui->comboBoxZoneType->addItem(tr("Player start"), QVariant(static_cast<int>(ETemplateZoneType::PLAYER_START)));
	ui->comboBoxZoneType->addItem(tr("CPU start"), QVariant(static_cast<int>(ETemplateZoneType::CPU_START)));
	ui->comboBoxZoneType->addItem(tr("Treasure"), QVariant(static_cast<int>(ETemplateZoneType::TREASURE)));
	ui->comboBoxZoneType->addItem(tr("Junction"), QVariant(static_cast<int>(ETemplateZoneType::JUNCTION)));
	ui->comboBoxZoneType->addItem(tr("Water"), QVariant(static_cast<int>(ETemplateZoneType::WATER)));
	ui->comboBoxZoneType->addItem(tr("Sealed"), QVariant(static_cast<int>(ETemplateZoneType::SEALED)));
	for (int i = 0; i < ui->comboBoxZoneType->count(); ++i)
    	if (ui->comboBoxZoneType->itemData(i).toInt() == static_cast<int>(zone->getType()))
        	ui->comboBoxZoneType->setCurrentIndex(i);

	ui->comboBoxZoneOwner->clear();
	auto type = static_cast<ETemplateZoneType>(ui->comboBoxZoneType->currentData().toInt());
	if((type == ETemplateZoneType::PLAYER_START || type == ETemplateZoneType::CPU_START))
	{
		ui->comboBoxZoneOwner->setEnabled(true);
		for(auto color = PlayerColor(0); color < PlayerColor::PLAYER_LIMIT; ++color)
		{
			MetaString str;
			str.appendName(color);
			ui->comboBoxZoneOwner->addItem(QString::fromStdString(str.toString()), QVariant(static_cast<int>(color)));
		}
		for (int i = 0; i < ui->comboBoxZoneOwner->count(); ++i)
			if (ui->comboBoxZoneOwner->itemData(i).toInt() == static_cast<int>(*zone->getOwner()))
				ui->comboBoxZoneOwner->setCurrentIndex(i);
	}
	else
	{
		ui->comboBoxZoneOwner->addItem(tr("None"));
		ui->comboBoxZoneOwner->setEnabled(false);
	}

	ui->comboBoxMonsterStrength->clear();
	ui->comboBoxMonsterStrength->addItem(tr("None"), QVariant(static_cast<int>(EMonsterStrength::EMonsterStrength::ZONE_NONE)));
	ui->comboBoxMonsterStrength->addItem(tr("Random"), QVariant(static_cast<int>(EMonsterStrength::EMonsterStrength::RANDOM)));
	ui->comboBoxMonsterStrength->addItem(tr("Weak"), QVariant(static_cast<int>(EMonsterStrength::EMonsterStrength::ZONE_WEAK)));
	ui->comboBoxMonsterStrength->addItem(tr("Normal"), QVariant(static_cast<int>(EMonsterStrength::EMonsterStrength::ZONE_NORMAL)));
	ui->comboBoxMonsterStrength->addItem(tr("Strong"), QVariant(static_cast<int>(EMonsterStrength::EMonsterStrength::ZONE_STRONG)));
	for (int i = 0; i < ui->comboBoxMonsterStrength->count(); ++i)
    	if (ui->comboBoxMonsterStrength->itemData(i).toInt() == static_cast<int>(zone->monsterStrength))
        	ui->comboBoxMonsterStrength->setCurrentIndex(i);
}

void TemplateEditor::loadZoneConnectionMenuContent()
{
	auto widget = ui->tableWidgetConnections;
	auto & connections = templates[selectedTemplate]->connections;

	widget->clear();
	widget->clearContents();
	widget->setRowCount(0);
	widget->setColumnCount(6);
	widget->setHorizontalHeaderLabels({ tr("Zone A"), tr("Zone B"), tr("Guard"), tr("Type"), tr("Road"), "" });
	for (int i = 0; i < connections.size(); i++)
	{
		int row = widget->rowCount();
		widget->insertRow(row);
		QSpinBox* zoneA = new QSpinBox();
		QSpinBox* zoneB = new QSpinBox();
		QSpinBox* guardStrength = new QSpinBox();
		QComboBox* connectionType = new QComboBox();
		QComboBox* hasRoad = new QComboBox();
		QPushButton* delButton = new QPushButton();
		zoneA->setRange(0, 999);
		zoneB->setRange(0, 999);
		guardStrength->setRange(0, 99999);
		connectionType->addItem(tr("Guarded"), QVariant(static_cast<int>(rmg::EConnectionType::GUARDED)));
		connectionType->addItem(tr("Fictive"), QVariant(static_cast<int>(rmg::EConnectionType::FICTIVE)));
		connectionType->addItem(tr("Repulsive"), QVariant(static_cast<int>(rmg::EConnectionType::REPULSIVE)));
		connectionType->addItem(tr("Wide"), QVariant(static_cast<int>(rmg::EConnectionType::WIDE)));
		connectionType->addItem(tr("Force portal"), QVariant(static_cast<int>(rmg::EConnectionType::FORCE_PORTAL)));
		hasRoad->addItem(tr("Random"), QVariant(static_cast<int>(rmg::ERoadOption::ROAD_RANDOM)));
		hasRoad->addItem(tr("Yes"), QVariant(static_cast<int>(rmg::ERoadOption::ROAD_TRUE)));
		hasRoad->addItem(tr("No"), QVariant(static_cast<int>(rmg::ERoadOption::ROAD_FALSE)));
		zoneA->setValue(connections.at(i).getZoneA());
		zoneB->setValue(connections.at(i).getZoneB());
		guardStrength->setValue(connections.at(i).getGuardStrength());
		for (int j = 0; j < connectionType->count(); ++j)
			if (connectionType->itemData(j).toInt() == static_cast<int>(connections[i].getConnectionType()))
				connectionType->setCurrentIndex(j);
		for (int j = 0; j < hasRoad->count(); ++j)
			if (hasRoad->itemData(j).toInt() == static_cast<int>(connections[i].hasRoad))
				hasRoad->setCurrentIndex(j);
		delButton->setText(tr("Del"));
		delButton->setToolTip(tr("Delete"));
		connect(zoneA, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this, i, &connections](int val){
			connections.at(i).zoneA = val;
			updateConnectionLines(true);
			changed();
		});
		connect(zoneB, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this, i, &connections](int val){
			connections.at(i).zoneB = val;
			updateConnectionLines(true);
			changed();
		});
		connect(guardStrength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this, i, &connections](int val){
			connections.at(i).guardStrength = val;
			updateConnectionLines();
			changed();
		});
		connect(connectionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i, &connections, connectionType](int val){
			connections.at(i).connectionType = static_cast<rmg::EConnectionType>(connectionType->itemData(val).toInt());
			updateConnectionLines();
			changed();
		});
		connect(hasRoad, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i, &connections, hasRoad](int val){
			connections.at(i).hasRoad = static_cast<rmg::ERoadOption>(hasRoad->itemData(val).toInt());
			updateConnectionLines();
			changed();
		});
		connect(delButton, &QPushButton::pressed, this, [this, i, &connections](){
			connections.erase(connections.begin() + i);
			updateConnectionLines(true);
			loadZoneConnectionMenuContent();
			changed();
		});
		widget->setCellWidget(i, 0, zoneA);
		widget->setCellWidget(i, 1, zoneB);
		widget->setCellWidget(i, 2, guardStrength);
		widget->setCellWidget(i, 3, connectionType);
		widget->setCellWidget(i, 4, hasRoad);
		widget->setCellWidget(i, 5, delButton);
	};
	widget->resizeColumnsToContents();
}

void TemplateEditor::saveZoneMenuContent()
{
	if(selectedZone < 0 || selectedTemplate.empty())
		return;

	auto zone = templates[selectedTemplate]->getZones().at(selectedZone);
	auto type = static_cast<ETemplateZoneType>(ui->comboBoxZoneType->currentData().toInt());

	zone->setVisiblePosition(Point(ui->spinBoxZoneVisPosX->value(), ui->spinBoxZoneVisPosY->value()));
	zone->setVisibleSize(ui->doubleSpinBoxZoneVisSize->value());
	zone->setType(type);
	zone->setSize(ui->spinBoxZoneSize->value());
	zone->playerTowns.townCount = ui->spinBoxTownCountPlayer->value();
	zone->playerTowns.castleCount = ui->spinBoxCastleCountPlayer->value();
	zone->playerTowns.townDensity = ui->spinBoxTownDensityPlayer->value();
	zone->playerTowns.castleDensity = ui->spinBoxCastleDensityPlayer->value();
	zone->neutralTowns.townCount = ui->spinBoxTownCountNeutral->value();
	zone->neutralTowns.castleCount = ui->spinBoxCastleCountNeutral->value();
	zone->neutralTowns.townDensity = ui->spinBoxTownDensityNeutral->value();
	zone->neutralTowns.castleDensity = ui->spinBoxCastleDensityNeutral->value();
	zone->matchTerrainToTown = ui->checkBoxMatchTerrainToTown->isChecked();
	zone->townsAreSameType = ui->checkBoxTownsAreSameType->isChecked();
	zone->monsterStrength = static_cast<EMonsterStrength::EMonsterStrength>(ui->comboBoxMonsterStrength->currentData().toInt());
	zone->id = ui->spinBoxZoneId->value();
	zone->townsLikeZone = ui->checkBoxZoneLinkTowns->isChecked() ? ui->spinBoxZoneLinkTowns->value() : rmg::ZoneOptions::NO_ZONE;
	zone->minesLikeZone = ui->checkBoxZoneLinkMines->isChecked() ? ui->spinBoxZoneLinkMines->value() : rmg::ZoneOptions::NO_ZONE;
	zone->terrainTypeLikeZone = ui->checkBoxZoneLinkTerrain->isChecked() ? ui->spinBoxZoneLinkTerrain->value() : rmg::ZoneOptions::NO_ZONE;
	zone->treasureLikeZone = ui->checkBoxZoneLinkTreasure->isChecked() ? ui->spinBoxZoneLinkTreasure->value() : rmg::ZoneOptions::NO_ZONE;
	zone->customObjectsLikeZone = ui->checkBoxZoneLinkCustomObjects->isChecked() ? ui->spinBoxZoneLinkCustomObjects->value() : rmg::ZoneOptions::NO_ZONE;

	if((type == ETemplateZoneType::PLAYER_START || type == ETemplateZoneType::CPU_START))
		zone->owner = std::optional<int>(static_cast<PlayerColor>(ui->comboBoxZoneOwner->currentData().toInt()));
	else
		zone->owner = std::nullopt;

	updateZonePositions();
	updateZoneCards(selectedZone);

	changed();
}

void TemplateEditor::updateConnectionLines(bool recreate)
{
	if(recreate)
	{
		for(auto & line : lines)
			templateScene->removeItem(line);
		lines.clear();

		for(int i = 0; i < templates[selectedTemplate]->getConnectedZoneIds().size(); i++)
		{
			auto & connection = templates[selectedTemplate]->getConnectedZoneIds()[i];
			auto line = new LineItem();
			line->setId(i);
			line->setLineToolTip(tr("Zone A: %1\nZone B: %2\nGuard: %3").arg(QString::number(connection.zoneA)).arg(QString::number(connection.zoneB)).arg(QString::number(connection.guardStrength)));
			line->setClickCallback([this, line](){
				ui->toolBox->setCurrentIndex(2);
				ui->tableWidgetConnections->selectRow(line->getId());
			});
			lines.push_back(line);
			templateScene->addItem(line);
		}
	}

	const auto& connections = templates[selectedTemplate]->getConnectedZoneIds();
	for (size_t i = 0; i < connections.size(); ++i)
	{
		auto& connection = connections[i];

		if(!cards.count(connection.getZoneA()) || !cards.count(connection.getZoneB()) || i >= lines.size())
			continue;

		auto getCenter = [](auto & i){
			return i->pos() + QPointF(i->boundingRect().width() / 2, i->boundingRect().height() / 2) * i->scale();
		};
		auto posA = getCenter(cards[connection.getZoneA()]);
		auto posB = getCenter(cards[connection.getZoneB()]);

		lines[i]->setLine(QLineF(posA.x(), posA.y(), posB.x(), posB.y()));
		lines[i]->setText(QString::number(connection.getGuardStrength()));

		std::map<rmg::EConnectionType, QPen> pens = {
			{rmg::EConnectionType::GUARDED, QPen(Qt::black, 5, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin)},
			{rmg::EConnectionType::FICTIVE, QPen(Qt::black, 5, Qt::DashLine, Qt::SquareCap, Qt::BevelJoin)},
			{rmg::EConnectionType::REPULSIVE, QPen(Qt::black, 5, Qt::DashDotLine, Qt::SquareCap, Qt::BevelJoin)},
			{rmg::EConnectionType::WIDE, QPen(Qt::black, 5, Qt::DotLine, Qt::SquareCap, Qt::BevelJoin)},
			{rmg::EConnectionType::FORCE_PORTAL, QPen(Qt::black, 5, Qt::DashDotDotLine, Qt::SquareCap, Qt::BevelJoin)}
		};
		lines[i]->setPen(pens[connection.connectionType]);
	}
}

void TemplateEditor::saveContent()
{
	templates[selectedTemplate]->name = ui->lineEditName->text().toStdString();
	templates[selectedTemplate]->description = ui->lineEditDescription->text().toStdString();
	templates[selectedTemplate]->minSize = int3(ui->spinBoxMinSizeX->value(), ui->spinBoxMinSizeY->value(), ui->spinBoxMinSizeZ->value());
	templates[selectedTemplate]->maxSize = int3(ui->spinBoxMaxSizeX->value(), ui->spinBoxMaxSizeY->value(), ui->spinBoxMaxSizeZ->value());

	templates[selectedTemplate]->allowedWaterContent.clear();
	if(ui->checkBoxAllowedWaterContentNone->isChecked())
		templates[selectedTemplate]->allowedWaterContent.insert(EWaterContent::EWaterContent::NONE);
	if(ui->checkBoxAllowedWaterContentNormal->isChecked())
		templates[selectedTemplate]->allowedWaterContent.insert(EWaterContent::EWaterContent::NORMAL);
	if(ui->checkBoxAllowedWaterContentIslands->isChecked())
		templates[selectedTemplate]->allowedWaterContent.insert(EWaterContent::EWaterContent::ISLANDS);

	changed();
}

bool TemplateEditor::getAnswerAboutUnsavedChanges()
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

void TemplateEditor::setTitle()
{
	QFileInfo fileInfo(filename);
	QString title = QString("%1%2 - %3 (%4)").arg(fileInfo.fileName(), unsaved ? "*" : "", tr("VCMI Template Editor"), GameConstants::VCMI_VERSION.c_str());
	setWindowTitle(title);
}

void TemplateEditor::changed()
{
	unsaved = true;
	setTitle();
}

void TemplateEditor::saveTemplate()
{
	saveContent();

	Helper::saveTemplate(templates, filename);
	unsaved = false;
}

void TemplateEditor::showTemplateEditor()
{
	auto * dialog = new TemplateEditor();

	dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void TemplateEditor::on_actionOpen_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;
	
	auto filenameSelect = QFileDialog::getOpenFileName(this, tr("Open template"),
		QString::fromStdString(VCMIDirs::get().userDataPath().make_preferred().string()),
		tr("VCMI templates(*.json)"));
	if(filenameSelect.isEmpty())
		return;

	templates = Helper::openTemplateInternal(filenameSelect);

	initContent();
	loadContent();
	ui->templateView->autoFit();
}

void TemplateEditor::on_actionSave_as_triggered()
{
	auto filenameSelect = QFileDialog::getSaveFileName(this, tr("Save template"), "", tr("VCMI templates (*.json)"));

	if(filenameSelect.isNull())
		return;

	QFileInfo fileInfo(filenameSelect);

	if(fileInfo.suffix().toLower() != "json")
		filenameSelect += ".json";

	filename = filenameSelect;
	saveTemplate();
	setTitle();
}

void TemplateEditor::on_actionNew_triggered()
{
	if(!getAnswerAboutUnsavedChanges())
		return;

	templates = std::map<std::string, std::shared_ptr<CRmgTemplate>>();
	templates["TemplateEditor"] = std::make_shared<CRmgTemplate>();
	
	changed();
	initContent();
	loadContent();
	ui->templateView->autoFit();
}

void TemplateEditor::on_actionSave_triggered()
{
	if(filename.isNull())
		on_actionSave_as_triggered();
	else 
		saveTemplate();
	setTitle();
}

void TemplateEditor::on_actionAutoPosition_triggered()
{
	if(!templates.size())
		return;

	saveContent();
	loadContent(true);
	ui->templateView->autoFit();
}

void TemplateEditor::on_actionZoom_in_triggered()
{
	ui->templateView->changeZoomLevel(true);
}

void TemplateEditor::on_actionZoom_out_triggered()
{
	ui->templateView->changeZoomLevel(false);
}

void TemplateEditor::on_actionZoom_auto_triggered()
{
	ui->templateView->autoFit();
}

void TemplateEditor::on_actionZoom_reset_triggered()
{
	ui->templateView->setZoomLevel(0);
}

void TemplateEditor::on_actionAddZone_triggered()
{
	if(!templates.size())
		return;

	for(int i = 1; i < 999; i++)
	{
		if(!templates[selectedTemplate]->zones.count(i))
		{
			templates[selectedTemplate]->zones[i] = std::make_shared<rmg::ZoneOptions>();
			break;
		}
	}
	loadContent();
}

void TemplateEditor::on_actionRemoveZone_triggered()
{
	templates[selectedTemplate]->zones.erase(selectedZone);
	selectedZone = -1;
	loadContent();
}

void TemplateEditor::on_comboBoxTemplateSelection_activated(int index)
{
	auto value = ui->comboBoxTemplateSelection->currentText().toStdString();
	if(value.empty())
		return;

	saveContent();
	selectedTemplate = value;
	loadContent();
}

void TemplateEditor::closeEvent(QCloseEvent *event)
{
	if(getAnswerAboutUnsavedChanges())
		QWidget::closeEvent(event);
	else
		event->ignore();
}

void TemplateEditor::on_pushButtonAddSubTemplate_clicked()
{
	bool ok;
	QString text = QInputDialog::getText(this, tr("Enter Name"), tr("Name:"), QLineEdit::Normal, "", &ok);

	if (!ok || text.isEmpty())
		return;

	if(templates.count(text.toStdString()))
	{
		QMessageBox::critical(this, tr("Already existing!"), tr("A template with this name is already existing."));
		return;
	}

	selectedTemplate = text.toStdString();
	templates[selectedTemplate] = std::make_shared<CRmgTemplate>();
	ui->comboBoxTemplateSelection->addItem(text);
	ui->comboBoxTemplateSelection->setCurrentIndex(ui->comboBoxTemplateSelection->count() - 1);

	loadContent();
}

void TemplateEditor::on_pushButtonRemoveSubTemplate_clicked()
{
	if(templates.size() < 2)
	{
		QMessageBox::critical(this, tr("To few templates!"), tr("At least one template should remain after removing."));
		return;
	}

	templates.erase(selectedTemplate);
	ui->comboBoxTemplateSelection->removeItem(ui->comboBoxTemplateSelection->currentIndex());
	selectedTemplate = ui->comboBoxTemplateSelection->currentText().toStdString();

	loadContent();
}

void TemplateEditor::on_pushButtonRenameSubTemplate_clicked()
{
	if(ui->comboBoxTemplateSelection->currentText().isEmpty() || !templates.size())
		return;

	bool ok;
	QString text = QInputDialog::getText(this, tr("Enter Name"), tr("Name:"), QLineEdit::Normal, ui->comboBoxTemplateSelection->currentText(), &ok);

	if (!ok || text.isEmpty())
		return;

	ui->comboBoxTemplateSelection->setItemText(ui->comboBoxTemplateSelection->currentIndex(), text);

	templates[text.toStdString()] = templates[selectedTemplate];
	templates.erase(selectedTemplate);
	selectedTemplate = text.toStdString();
}

void TemplateEditor::on_spinBoxZoneVisPosX_valueChanged()
{
	if(ui->spinBoxZoneVisPosX->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneVisPosY_valueChanged()
{
	if(ui->spinBoxZoneVisPosY->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_doubleSpinBoxZoneVisSize_valueChanged()
{
	if(ui->doubleSpinBoxZoneVisSize->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_comboBoxZoneType_currentTextChanged(const QString &text)
{
	if(!ui->comboBoxZoneType->hasFocus())
		return;
	ui->comboBoxZoneType->clearFocus(); 

	saveZoneMenuContent();
	loadZoneMenuContent();
}

void TemplateEditor::on_comboBoxZoneOwner_currentTextChanged(const QString &text)
{
	if(ui->comboBoxZoneOwner->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneSize_valueChanged()
{
	if(ui->spinBoxZoneSize->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxTownCountPlayer_valueChanged()
{
	if(ui->spinBoxTownCountPlayer->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxCastleCountPlayer_valueChanged()
{
	if(ui->spinBoxCastleCountPlayer->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxTownDensityPlayer_valueChanged()
{
	if(ui->spinBoxTownDensityPlayer->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxCastleDensityPlayer_valueChanged()
{
	if(ui->spinBoxCastleDensityPlayer->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxTownCountNeutral_valueChanged()
{
	if(ui->spinBoxTownCountNeutral->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxCastleCountNeutral_valueChanged()
{
	if(ui->spinBoxCastleCountNeutral->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxTownDensityNeutral_valueChanged()
{
	if(ui->spinBoxTownDensityNeutral->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxCastleDensityNeutral_valueChanged()
{
	if(ui->spinBoxCastleDensityNeutral->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_checkBoxMatchTerrainToTown_stateChanged(int state)
{
	if(ui->checkBoxMatchTerrainToTown->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_checkBoxTownsAreSameType_stateChanged(int state)
{
	if(ui->checkBoxTownsAreSameType->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_comboBoxMonsterStrength_currentTextChanged(const QString &text)
{
	if(ui->comboBoxMonsterStrength->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneId_valueChanged()
{
	if(ui->spinBoxZoneId->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneLinkTowns_valueChanged()
{
	if(ui->spinBoxZoneLinkTowns->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneLinkMines_valueChanged()
{
	if(ui->spinBoxZoneLinkMines->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneLinkTerrain_valueChanged()
{
	if(ui->spinBoxZoneLinkTerrain->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneLinkTreasure_valueChanged()
{
	if(ui->spinBoxZoneLinkTreasure->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_spinBoxZoneLinkCustomObjects_valueChanged()
{
	if(ui->spinBoxZoneLinkCustomObjects->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_checkBoxZoneLinkTowns_stateChanged(int state)
{
	if(!ui->checkBoxZoneLinkTowns->hasFocus())
		return;
	ui->checkBoxZoneLinkTowns->clearFocus();

	saveZoneMenuContent();
	loadZoneMenuContent();
}

void TemplateEditor::on_checkBoxZoneLinkMines_stateChanged(int state)
{
	if(!ui->checkBoxZoneLinkMines->hasFocus())
		return;
	ui->checkBoxZoneLinkMines->clearFocus();

	saveZoneMenuContent();
	loadZoneMenuContent();
}

void TemplateEditor::on_checkBoxZoneLinkTerrain_stateChanged(int state)
{
	if(!ui->checkBoxZoneLinkTerrain->hasFocus())
		return;
	ui->checkBoxZoneLinkTerrain->clearFocus();

	saveZoneMenuContent();
	loadZoneMenuContent();
}

void TemplateEditor::on_checkBoxZoneLinkTreasure_stateChanged(int state)
{
	if(!ui->checkBoxZoneLinkTreasure->hasFocus())
		return;
	ui->checkBoxZoneLinkTreasure->clearFocus();

	saveZoneMenuContent();
	loadZoneMenuContent();
}

void TemplateEditor::on_checkBoxZoneLinkCustomObjects_stateChanged(int state)
{
	if(!ui->checkBoxZoneLinkCustomObjects->hasFocus())
		return;
	ui->checkBoxZoneLinkCustomObjects->clearFocus();

	saveZoneMenuContent();
	loadZoneMenuContent();
}

void TemplateEditor::on_checkBoxAllowedWaterContentNone_stateChanged(int state)
{
	if(ui->checkBoxAllowedWaterContentNone->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_checkBoxAllowedWaterContentNormal_stateChanged(int state)
{
	if(ui->checkBoxAllowedWaterContentNormal->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_checkBoxAllowedWaterContentIslands_stateChanged(int state)
{
	if(ui->checkBoxAllowedWaterContentIslands->hasFocus())
		saveZoneMenuContent();
}

void TemplateEditor::on_pushButtonConnectionAdd_clicked()
{
	auto & connections = templates[selectedTemplate]->connections;
	connections.push_back(rmg::ZoneConnection());
	loadZoneConnectionMenuContent();
}

void TemplateEditor::on_pushButtonOpenTerrainTypes_clicked()
{
	TerrainSelector::showTerrainSelector(templates[selectedTemplate]->getZones().at(selectedZone)->terrainTypes);
}

void TemplateEditor::on_pushButtonOpenBannedTerrainTypes_clicked()
{
	TerrainSelector::showTerrainSelector(templates[selectedTemplate]->getZones().at(selectedZone)->bannedTerrains);
}

void TemplateEditor::on_pushButtonAllowedTowns_clicked()
{
	FactionSelector::showFactionSelector(templates[selectedTemplate]->getZones().at(selectedZone)->townTypes);
}

void TemplateEditor::on_pushButtonBannedTowns_clicked()
{
	FactionSelector::showFactionSelector(templates[selectedTemplate]->getZones().at(selectedZone)->bannedTownTypes);
}

void TemplateEditor::on_pushButtonTownHints_clicked()
{
	//TODO: Implement dialog
	QMessageBox::critical(this, tr("Error"), tr("Not implemented yet!"));
}

void TemplateEditor::on_pushButtonAllowedMonsters_clicked()
{
	FactionSelector::showFactionSelector(templates[selectedTemplate]->getZones().at(selectedZone)->monsterTypes);
}

void TemplateEditor::on_pushButtonBannedMonsters_clicked()
{
	FactionSelector::showFactionSelector(templates[selectedTemplate]->getZones().at(selectedZone)->bannedMonsters);
}

void TemplateEditor::on_pushButtonTreasure_clicked()
{
	TreasureSelector::showTreasureSelector(templates[selectedTemplate]->getZones().at(selectedZone)->treasureInfo);
}

void TemplateEditor::on_pushButtonMines_clicked()
{
	MineSelector::showMineSelector(templates[selectedTemplate]->getZones().at(selectedZone)->mines);
	updateZoneCards(selectedZone);
}

void TemplateEditor::on_pushButtonCustomObjects_clicked()
{
	//TODO: Implement dialog
	QMessageBox::critical(this, tr("Error"), tr("Not implemented yet!"));
}
