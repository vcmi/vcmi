#include "DoomAI.h"
//#include "..\..\lib\mapping\CMapDefines.h"
#include "..\..\lib\mapObjects\CGTownInstance.h"
#include "..\..\lib\VCMI_Lib.h"
/*
typedef enum NodeContentEnum {
	NONE = 0,
	VISITABLE_OBJECTS = 1,
	BORDERS = 2,
	OBSERVABLE_POINT = 4
} NodeContent;

class Node;

class NodeJoint {
public:
	std::shared_ptr<Node> node1;
	std::shared_ptr<Node> node2;
	ui32 distance;

	NodeJoint(std::shared_ptr<Node> n1, std::shared_ptr<Node> n2, ui32 dist) {
		node1 = n1;
		node2 = n2;
		distance = dist;
	}

	void link();

	void unlink();

	Node & other(Node &node);

	~NodeJoint() {
		node1 = NULL;
		node2 = NULL;
	}
};

class Node {
public:
	int3 mapPos;
	NodeContent usefullContent;
	std::shared_ptr<std::set<std::shared_ptr<NodeJoint>>> joints;

	Node(int3 pos, NodeContent nodeContent) {
		mapPos = pos;
		usefullContent = nodeContent;
		joints = std::make_shared<std::set<std::shared_ptr<NodeJoint>>>();
	}

	bool isUsefull() {
		return usefullContent != 0;
	}

	void addJoint(Node &to, ui32 distance) {
		std::shared_ptr<NodeJoint> joint;

		for each (joint in (*joints))
		{
			if (to.mapPos == joint->other(*this).mapPos) {
				joint->distance = distance;
			}
		}

		joint = std::make_shared<NodeJoint>(
			std::allocate_shared<NodeJoint>(this), 
			std::allocate_shared<NodeJoint>(to), 
			distance);

		joint->link();
	}

	~Node() {
		joints = NULL;
	}
};

void NodeJoint::link() {
	node1->joints->insert(std::allocate_shared<NodeJoint>(this));
	node2->joints->insert(std::allocate_shared<NodeJoint>(this));
}

void NodeJoint::unlink() {
	node1->joints->erase(std::allocate_shared<NodeJoint>(this));
	node2->joints->erase(std::allocate_shared<NodeJoint>(this));
}

Node &  NodeJoint::other(Node &node) {
	return *(node.mapPos == node1->mapPos ? node1 : node2);
}

struct TileInfo {
public:
	std::shared_ptr<Node> node = NULL;
	bool visible = false;
	ETerrainType::EETerrainType terrainType = ETerrainType::EETerrainType::WRONG;
	ERoadType::ERoadType roadType = ERoadType::ERoadType::NO_ROAD;

	ui32 pathDistance;
	int3 pathParent;
	int run;

	bool isWalkable() {
		return this->terrainType == ETerrainType::EETerrainType::BORDER 
			|| ETerrainType::EETerrainType::WATER 
			|| ETerrainType::EETerrainType::ROCK;
	}
};

class NodeGraph {
public:
	std::shared_ptr<std::set<std::shared_ptr<Node>>> nodes;
	std::shared_ptr<boost::multi_array<TileInfo, 3>> map;

	NodeGraph(int3 mapSize) {
		map = std::make_shared<boost::multi_array<TileInfo, 3>>(boost::extents[mapSize.x][mapSize.y][mapSize.z]);
		nodes = std::make_shared<std::set<std::shared_ptr<Node>>>();
	}

	void addTile(int3 pos, const struct TerrainTile* tile) {
		TileInfo &tileInfo = (*this->map)(pos);

		if (tileInfo.visible) {
			return;
		}

		tileInfo.visible = true;
		tileInfo.terrainType = tile->terType;

		NodeContent nodeContent = NodeContent::NONE;
		const std::vector<CGObjectInstance*> visitableObjects = tile->visitableObjects;
		const std::vector<CGObjectInstance*> blockingObjects = tile->blockingObjects;

		if (tileInfo.isWalkable()) {
			if (visitableObjects.size() > 0 || blockingObjects.size() > 0) {
				nodeContent = NodeContent::VISITABLE_OBJECTS;
			}

			foreach_neighbour(pos, [&](const int3 &neighborPos) {
				auto neighborInfo = (*this->map)(neighborPos);

				if (!neighborInfo.visible) {
					nodeContent = NodeContent::OBSERVABLE_POINT;
				}
			});
		}

		foreach_neighbour(pos, [&](const int3 &neighborPos) {
			auto neighborInfo = (*this->map)(neighborPos);

			if (neighborInfo.node != NULL && neighborInfo.node->usefullContent == NodeContent::OBSERVABLE_POINT) {
				bool allNeighborsAreVisible = true;

				foreach_neighbour(neighborPos, [&](const int3 &neighborPos) {
					auto neighborInfo = (*this->map)(neighborPos);

					allNeighborsAreVisible &= neighborInfo.visible;
				});

				if (!allNeighborsAreVisible) {
					nodes->erase(neighborInfo.node);
					for each (auto nodeJoint in *(neighborInfo.node->joints))
					{
						nodeJoint->unlink();
					}

					neighborInfo.node = NULL;
				}
			}
		});

		if (nodeContent != NodeContent::NONE && tileInfo.node == NULL) {
			tileInfo.node = std::make_shared<Node>(pos, nodeContent);
			nodes->insert(tileInfo.node);
			scanPath(*tileInfo.node);
		}
	}

	~NodeGraph() {
		nodes->clear();
		nodes = NULL;
		map = NULL;
	}

private:
	int run = 0;
	void scanPath(Node &start) {
		run++;
		(*map)(start.mapPos).run = run;
		std::shared_ptr<std::queue<int3>> positions = std::make_shared<std::queue<int3>>();
		positions->push(start.mapPos);

		while(!positions->empty())
		{
			int3 pos = positions->front();
			TileInfo &parent = (*map)(pos);
			ui32 distance = parent.pathDistance;

			foreach_neighbour(pos, [&](int3 neighborPos) {
				TileInfo &neighbor = (*map)(neighborPos);

				if (neighbor.visible || neighbor.isWalkable()) {
					ui32 tileCost = getTileCost(neighbor);

					if (pos.x != neighborPos.x && pos.y != neighborPos.y) {
						tileCost *= 1.4;
					}

					ui32 distance = parent.pathDistance + tileCost;

					if (neighbor.run != run || neighbor.pathDistance > distance) {
						neighbor.run = run;
						neighbor.pathDistance = distance;
						neighbor.pathParent = pos;
						positions->push(neighborPos);

						if (neighbor.node != NULL) {
							start.addJoint(*neighbor.node, distance);
						}
					}
				}
			});

			positions->pop();
		}
	}

	ui32 getTileCost(TileInfo &tile) {
		unsigned ret = GameConstants::BASE_MOVEMENT_COST;

		//if there is road both on dest and src tiles - use road movement cost
		if (tile.roadType != ERoadType::NO_ROAD)
		{
			switch (tile.roadType)
			{
			case ERoadType::DIRT_ROAD:
				return 75;
			case ERoadType::GRAVEL_ROAD:
				return 65;
			case ERoadType::COBBLESTONE_ROAD:
				return 50;
			default:
				break;
			}
		}

		switch (tile.terrainType)
		{
		case ETerrainType::ROUGH:
			return 125;
		case ETerrainType::SNOW:
		case ETerrainType::SAND:
			return 150;
		case ETerrainType::SWAMP:
			return 175;
		default:
			break;
		}

		return GameConstants::BASE_MOVEMENT_COST;
	}
};
*/
//
//
//class BuildTask;
//
//class BuildState {
//public:
//	TResources dailyIncome;
//	TResources availableResources;
//	int totalScore;
//	int day;
//	int dayOfWeek;
//
//	virtual bool hasBuilt(BuildingID buildingID) { return false; }
//
//	virtual int getAvailableCreaturesCount(BuildingInfo* buildingInfo) { return 0; }
//
//	virtual std::string toString() { return "initial"; }
//};
//
//class BuildTask : public BuildState {
//public:
//	BuildingID toBuild;
//	std::shared_ptr<BuildState> previousState;
//
//	BuildTask(std::shared_ptr<BuildState> previous, BuildingInfo* buildingInfo) {
//		toBuild = buildingInfo->id;
//		dailyIncome = previous->dailyIncome + buildingInfo->dailyIncome;
//		availableResources = previous->availableResources - buildingInfo->buildCost + previous->dailyIncome;
//		previousState = previous;
//		day = previous->day + 1;
//		dayOfWeek = previous->dayOfWeek % 7 == boost::date_time::Sunday ? boost::date_time::Monday : previous->dayOfWeek + 1;
//	}
//
//	~BuildTask() {
//		previousState.reset();
//	}
//
//	virtual std::string toString() override {
//		return previousState->toString() + " > " + std::to_string(toBuild.num);
//	}
//
//	virtual bool hasBuilt(BuildingID buildingID) override {
//		return toBuild == buildingID || previousState->hasBuilt(buildingID);
//	}
//
//	virtual int getAvailableCreaturesCount(BuildingInfo* buildingInfo) override {
//		if (!hasBuilt(buildingInfo->id)) {
//			return 0;
//		}
//
//		int result = previousState->getAvailableCreaturesCount(buildingInfo);
//
//		if (dayOfWeek == boost::date_time::Monday) {
//			int grows = buildingInfo->creatureGrows;
//
//			if (hasBuilt(BuildingID::CASTLE)) {
//				grows *= 2;
//			}
//			else if (hasBuilt(BuildingID::CITADEL)) {
//				grows = grows * 3 / 2;
//			}
//
//			result += grows;
//		}
//
//		return result;
//	}
//};
//
//class BuildPlan : public BuildState {
//public:
//	std::map<BuildingID, BuildingInfo*> existingBuildings = std::map<BuildingID, BuildingInfo*>();
//	std::vector<BuildingInfo*> toBuild = std::vector<BuildingInfo*>();
//	int scanDepth;
//	int scanDepthWeeks;
//
//	bool canBuild(BuildState &current, BuildingInfo* buildingInfo) {
//		if (!current.availableResources.canAfford(buildingInfo->buildCost)) {
//			return false;
//		}
//
//		return buildingInfo->requirements.test([&](const BuildingID &id) -> bool {
//			return current.hasBuilt(id);
//		});
//	}
//	
//	std::shared_ptr<BuildTask> createBuildTask(std::shared_ptr<BuildState> current, BuildingInfo* info) {
//		LOG_TRACE_PARAMS(logAi, "%s > %i", current->toString() % info->id.num);
//
//		return std::make_shared<BuildTask>(current, info);
//	}
//	
//	virtual bool hasBuilt(BuildingID buildingID) override {
//		return vstd::contains(existingBuildings, buildingID);
//	}
//
//	void calculateScore(std::shared_ptr<BuildTask> state) {
//		LOG_TRACE_PARAMS(logAi, "for %s", state->toString());
//
//		std::vector<BuildingInfo*> buildings = std::vector<BuildingInfo*>();
//
//		for (int level = 0; level < GameConstants::CREATURES_PER_TOWN; level++) {
//			BuildingID buildingID = BuildingID(BuildingID::DWELL_FIRST + level);
//						
//			BuildingInfo* buildingInfo;
//
//			if (vstd::contains(existingBuildings, buildingID)) {
//				buildingInfo = existingBuildings.at(buildingID);
//			}
//			else {
//				buildingInfo = toBuild.at(buildingID);
//			}
//
//			buildings.push_back(buildingInfo);
//		}
//
//		std::sort(buildings.begin(), buildings.end(), [&](BuildingInfo* b1, BuildingInfo* b2) -> bool {
//			return b1->costPerScore() < b2->costPerScore();
//		});
//
//		for (BuildingInfo* buildingInfo : buildings) {
//			int available = state->getAvailableCreaturesCount(buildingInfo);
//			TResources cost = buildingInfo->creatureCost;
//
//			while (cost < state->availableResources && available > 0) {
//				state->availableResources -= cost;
//				available--;
//				state->totalScore += buildingInfo->creatureScore;
//			}
//		}
//
//		logAi->info("Score for %s is %i", state->toString(), state->totalScore);
//	}
//};
//
//class DoomAIState {
//public:
//	int3 mapSize;
//	//std::shared_ptr<NodeGraph> graph;
//	std::shared_ptr<CCallback> gameInfo;
//
//	DoomAIState() {
//	}
//
//	void init(std::shared_ptr<CCallback> cb) {
//		LOG_TRACE(logAi);
//
//		gameInfo = cb;
//		mapSize = gameInfo->getMapSize();
//		//graph = std::make_shared<NodeGraph>(mapSize);
//
//		//scanVisibleTiles();
//		//scanTowns();
//	}
//
//	~DoomAIState() {
//		gameInfo = NULL;
//		//graph = NULL;
//	}
//	/*
//	void scanVisibleTiles() {
//		for (int x = 0; x < mapSize.x; x++)
//			for (int y = 0; y < mapSize.y; y++)
//				for (int z = 0; z < mapSize.z; z++) {
//					int3 pos = int3(x, y, z);
//					const struct TerrainTile* tile = gameInfo->getTile(pos, false);
//					if (tile) {
//						LOG_TRACE_PARAMS(logAi, "Doom: Tile revealed %s", pos.toString());
//						const std::vector<const CGObjectInstance*> objects = gameInfo->getVisitableObjs(pos, false);
//						graph->addTile(pos, tile);
//						//LOG_TRACE_PARAMS(logAi, "Doom: Tile has objects %i", objects.size());
//					}
//				}
//	}*/
//	
//	std::vector<BuildingID> scanTown(const CGTownInstance * town, bool forceEconomics) {
//		
//
//
//
//		std::shared_ptr<BuildPlan> p = std::make_shared<BuildPlan>();
//
//		p->dailyIncome = dailyIncome;
//		p->dayOfWeek = gameInfo->getDate(Date::EDateType::DAY_OF_WEEK);
//		p->scanDepthWeeks = 2;
//		p->scanDepth = p->scanDepthWeeks * 7 - p->dayOfWeek + 1;
//		p->availableResources = gameInfo->getResourceAmount();
//
//		auto creatures = townInfo->creatures;
//		auto buildings = townInfo->getAllBuildings();
//
//
//		
//
//
//		TResources weeklyIncome = town->dailyIncome() * 7;
//		std::vector<BuildingID> result = std::vector<BuildingID>();
//
//		armyCost *= 3;
//
//		if (!town->hasBuilt(BuildingID::TOWN_HALL)) {
//			result.push_back(BuildingID::TOWN_HALL);
//		}
//
//		if (town->hasBuilt(BuildingID::CITY_HALL) || (!forceEconomics && weeklyIncome.canAfford(armyCost))) {
//			if (p->existingBuildings.size() >= 2 && !town->hasBuilt(BuildingID::CITADEL) && gameInfo->getDate(Date::DAY_OF_WEEK) > boost::date_time::Friday) {
//				if (gameInfo->canBuildStructure(town, BuildingID::CITADEL) == EBuildingState::ALLOWED) {
//					result.push_back(BuildingID::CITADEL);
//					logAi->trace("Building preferences Citadel");
//				}
//			}
//
//			if (p->existingBuildings.size() >= 2 && !town->hasBuilt(BuildingID::CASTLE) && gameInfo->getDate(Date::DAY_OF_WEEK) > boost::date_time::Friday) {
//				if (gameInfo->canBuildStructure(town, BuildingID::CASTLE) == EBuildingState::ALLOWED) {
//					result.push_back(BuildingID::CASTLE);
//					logAi->trace("Building preferences CASTLE");
//				}
//			}
//
//			for (auto bi : p->toBuild) {
//				logAi->trace("Building preferences %s", bi->toString());
//				result.push_back(bi->id);
//			}
//		}
//
//
//		requiredForDwellings -= p->availableResources;
//		requiredForDwellings.positive();
//
//		logAi->trace("I need %s additional resources to develop the town.", requiredForDwellings.toString());
//		
//		return result;
//	}
//};

DoomAI::DoomAI()
{
}


DoomAI::~DoomAI()
{
}

void DoomAI::init(std::shared_ptr<CCallback> gameInfo) {
	VCAI::init(gameInfo);

	//auto state = std::make_shared<DoomAIState>(gameInfo);

	//state->init(gameInfo); 
}
