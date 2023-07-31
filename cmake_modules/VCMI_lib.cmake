macro(add_main_lib TARGET_NAME LIBRARY_TYPE)
	if(NOT DEFINED MAIN_LIB_DIR)
		set(MAIN_LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
	endif()
	
	set(lib_SRCS
		${MAIN_LIB_DIR}/StdInc.cpp

		${MAIN_LIB_DIR}/battle/AccessibilityInfo.cpp
		${MAIN_LIB_DIR}/battle/BattleAction.cpp
		${MAIN_LIB_DIR}/battle/BattleAttackInfo.cpp
		${MAIN_LIB_DIR}/battle/BattleHex.cpp
		${MAIN_LIB_DIR}/battle/BattleInfo.cpp
		${MAIN_LIB_DIR}/battle/BattleProxy.cpp
		${MAIN_LIB_DIR}/battle/BattleStateInfoForRetreat.cpp
		${MAIN_LIB_DIR}/battle/CBattleInfoCallback.cpp
		${MAIN_LIB_DIR}/battle/CBattleInfoEssentials.cpp
		${MAIN_LIB_DIR}/battle/CCallbackBase.cpp
		${MAIN_LIB_DIR}/battle/CObstacleInstance.cpp
		${MAIN_LIB_DIR}/battle/CPlayerBattleCallback.cpp
		${MAIN_LIB_DIR}/battle/CUnitState.cpp
		${MAIN_LIB_DIR}/battle/DamageCalculator.cpp
		${MAIN_LIB_DIR}/battle/Destination.cpp
		${MAIN_LIB_DIR}/battle/IBattleState.cpp
		${MAIN_LIB_DIR}/battle/ReachabilityInfo.cpp
		${MAIN_LIB_DIR}/battle/SideInBattle.cpp
		${MAIN_LIB_DIR}/battle/SiegeInfo.cpp
		${MAIN_LIB_DIR}/battle/Unit.cpp

		${MAIN_LIB_DIR}/bonuses/Bonus.cpp
		${MAIN_LIB_DIR}/bonuses/BonusEnum.cpp
		${MAIN_LIB_DIR}/bonuses/BonusList.cpp
		${MAIN_LIB_DIR}/bonuses/BonusParams.cpp
		${MAIN_LIB_DIR}/bonuses/BonusSelector.cpp
		${MAIN_LIB_DIR}/bonuses/CBonusProxy.cpp
		${MAIN_LIB_DIR}/bonuses/CBonusSystemNode.cpp
		${MAIN_LIB_DIR}/bonuses/IBonusBearer.cpp
		${MAIN_LIB_DIR}/bonuses/Limiters.cpp
		${MAIN_LIB_DIR}/bonuses/Propagators.cpp
		${MAIN_LIB_DIR}/bonuses/Updaters.cpp

		${MAIN_LIB_DIR}/campaign/CampaignHandler.cpp
		${MAIN_LIB_DIR}/campaign/CampaignState.cpp

		${MAIN_LIB_DIR}/events/ApplyDamage.cpp
		${MAIN_LIB_DIR}/events/GameResumed.cpp
		${MAIN_LIB_DIR}/events/ObjectVisitEnded.cpp
		${MAIN_LIB_DIR}/events/ObjectVisitStarted.cpp
		${MAIN_LIB_DIR}/events/PlayerGotTurn.cpp
		${MAIN_LIB_DIR}/events/TurnStarted.cpp

		${MAIN_LIB_DIR}/filesystem/AdapterLoaders.cpp
		${MAIN_LIB_DIR}/filesystem/CArchiveLoader.cpp
		${MAIN_LIB_DIR}/filesystem/CBinaryReader.cpp
		${MAIN_LIB_DIR}/filesystem/CCompressedStream.cpp
		${MAIN_LIB_DIR}/filesystem/CFileInputStream.cpp
		${MAIN_LIB_DIR}/filesystem/CFilesystemLoader.cpp
		${MAIN_LIB_DIR}/filesystem/CMemoryBuffer.cpp
		${MAIN_LIB_DIR}/filesystem/CMemoryStream.cpp
		${MAIN_LIB_DIR}/filesystem/CZipLoader.cpp
		${MAIN_LIB_DIR}/filesystem/CZipSaver.cpp
		${MAIN_LIB_DIR}/filesystem/FileInfo.cpp
		${MAIN_LIB_DIR}/filesystem/Filesystem.cpp
		${MAIN_LIB_DIR}/filesystem/MinizipExtensions.cpp
		${MAIN_LIB_DIR}/filesystem/ResourceID.cpp

		${MAIN_LIB_DIR}/gameState/CGameState.cpp
		${MAIN_LIB_DIR}/gameState/CGameStateCampaign.cpp
		${MAIN_LIB_DIR}/gameState/InfoAboutArmy.cpp
		${MAIN_LIB_DIR}/gameState/TavernHeroesPool.cpp

		${MAIN_LIB_DIR}/logging/CBasicLogConfigurator.cpp
		${MAIN_LIB_DIR}/logging/CLogger.cpp

		${MAIN_LIB_DIR}/mapObjectConstructors/AObjectTypeHandler.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/CBankInstanceConstructor.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/CObjectClassesHandler.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/CommonConstructors.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/CRewardableConstructor.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/DwellingInstanceConstructor.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/HillFortInstanceConstructor.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/ShipyardInstanceConstructor.cpp
		${MAIN_LIB_DIR}/mapObjectConstructors/ShrineInstanceConstructor.cpp

		${MAIN_LIB_DIR}/mapObjects/CArmedInstance.cpp
		${MAIN_LIB_DIR}/mapObjects/CBank.cpp
		${MAIN_LIB_DIR}/mapObjects/CGCreature.cpp
		${MAIN_LIB_DIR}/mapObjects/CGDwelling.cpp
		${MAIN_LIB_DIR}/mapObjects/CGHeroInstance.cpp
		${MAIN_LIB_DIR}/mapObjects/CGMarket.cpp
		${MAIN_LIB_DIR}/mapObjects/CGObjectInstance.cpp
		${MAIN_LIB_DIR}/mapObjects/CGPandoraBox.cpp
		${MAIN_LIB_DIR}/mapObjects/CGTownBuilding.cpp
		${MAIN_LIB_DIR}/mapObjects/CGTownInstance.cpp
		${MAIN_LIB_DIR}/mapObjects/CObjectHandler.cpp
		${MAIN_LIB_DIR}/mapObjects/CQuest.cpp
		${MAIN_LIB_DIR}/mapObjects/CRewardableObject.cpp
		${MAIN_LIB_DIR}/mapObjects/IMarket.cpp
		${MAIN_LIB_DIR}/mapObjects/IObjectInterface.cpp
		${MAIN_LIB_DIR}/mapObjects/MiscObjects.cpp
		${MAIN_LIB_DIR}/mapObjects/ObjectTemplate.cpp

		${MAIN_LIB_DIR}/mapping/CDrawRoadsOperation.cpp
		${MAIN_LIB_DIR}/mapping/CMap.cpp
		${MAIN_LIB_DIR}/mapping/CMapHeader.cpp
		${MAIN_LIB_DIR}/mapping/CMapEditManager.cpp
		${MAIN_LIB_DIR}/mapping/CMapInfo.cpp
		${MAIN_LIB_DIR}/mapping/CMapOperation.cpp
		${MAIN_LIB_DIR}/mapping/CMapService.cpp
		${MAIN_LIB_DIR}/mapping/MapEditUtils.cpp
		${MAIN_LIB_DIR}/mapping/MapIdentifiersH3M.cpp
		${MAIN_LIB_DIR}/mapping/MapFeaturesH3M.cpp
		${MAIN_LIB_DIR}/mapping/MapFormatH3M.cpp
		${MAIN_LIB_DIR}/mapping/MapReaderH3M.cpp
		${MAIN_LIB_DIR}/mapping/MapFormatJson.cpp
		${MAIN_LIB_DIR}/mapping/ObstacleProxy.cpp

		${MAIN_LIB_DIR}/pathfinder/CGPathNode.cpp
		${MAIN_LIB_DIR}/pathfinder/CPathfinder.cpp
		${MAIN_LIB_DIR}/pathfinder/NodeStorage.cpp
		${MAIN_LIB_DIR}/pathfinder/PathfinderOptions.cpp
		${MAIN_LIB_DIR}/pathfinder/PathfindingRules.cpp
		${MAIN_LIB_DIR}/pathfinder/TurnInfo.cpp

		${MAIN_LIB_DIR}/registerTypes/RegisterTypes.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesClientPacks1.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesClientPacks2.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesMapObjects1.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesMapObjects2.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesMapObjects3.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesLobbyPacks.cpp
		${MAIN_LIB_DIR}/registerTypes/TypesServerPacks.cpp

		${MAIN_LIB_DIR}/rewardable/Configuration.cpp
		${MAIN_LIB_DIR}/rewardable/Info.cpp
		${MAIN_LIB_DIR}/rewardable/Interface.cpp
		${MAIN_LIB_DIR}/rewardable/Limiter.cpp
		${MAIN_LIB_DIR}/rewardable/Reward.cpp

		${MAIN_LIB_DIR}/rmg/RmgArea.cpp
		${MAIN_LIB_DIR}/rmg/RmgObject.cpp
		${MAIN_LIB_DIR}/rmg/RmgPath.cpp
		${MAIN_LIB_DIR}/rmg/CMapGenerator.cpp
		${MAIN_LIB_DIR}/rmg/CMapGenOptions.cpp
		${MAIN_LIB_DIR}/rmg/CRmgTemplate.cpp
		${MAIN_LIB_DIR}/rmg/CRmgTemplateStorage.cpp
		${MAIN_LIB_DIR}/rmg/CZonePlacer.cpp
		${MAIN_LIB_DIR}/rmg/TileInfo.cpp
		${MAIN_LIB_DIR}/rmg/Zone.cpp
		${MAIN_LIB_DIR}/rmg/Functions.cpp
		${MAIN_LIB_DIR}/rmg/RmgMap.cpp
		${MAIN_LIB_DIR}/rmg/modificators/Modificator.cpp
		${MAIN_LIB_DIR}/rmg/modificators/ObjectManager.cpp
		${MAIN_LIB_DIR}/rmg/modificators/ObjectDistributor.cpp
		${MAIN_LIB_DIR}/rmg/modificators/RoadPlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/TreasurePlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/QuestArtifactPlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/ConnectionsPlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/WaterAdopter.cpp
		${MAIN_LIB_DIR}/rmg/modificators/MinePlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/TownPlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/WaterProxy.cpp
		${MAIN_LIB_DIR}/rmg/modificators/WaterRoutes.cpp
		${MAIN_LIB_DIR}/rmg/modificators/RockPlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/RockFiller.cpp
		${MAIN_LIB_DIR}/rmg/modificators/ObstaclePlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/RiverPlacer.cpp
		${MAIN_LIB_DIR}/rmg/modificators/TerrainPainter.cpp
		${MAIN_LIB_DIR}/rmg/threadpool/MapProxy.cpp

		${MAIN_LIB_DIR}/serializer/BinaryDeserializer.cpp
		${MAIN_LIB_DIR}/serializer/BinarySerializer.cpp
		${MAIN_LIB_DIR}/serializer/CLoadIntegrityValidator.cpp
		${MAIN_LIB_DIR}/serializer/CMemorySerializer.cpp
		${MAIN_LIB_DIR}/serializer/Connection.cpp
		${MAIN_LIB_DIR}/serializer/CSerializer.cpp
		${MAIN_LIB_DIR}/serializer/CTypeList.cpp
		${MAIN_LIB_DIR}/serializer/JsonDeserializer.cpp
		${MAIN_LIB_DIR}/serializer/JsonSerializeFormat.cpp
		${MAIN_LIB_DIR}/serializer/JsonSerializer.cpp
		${MAIN_LIB_DIR}/serializer/JsonUpdater.cpp
		${MAIN_LIB_DIR}/serializer/ILICReader.cpp

		${MAIN_LIB_DIR}/spells/AbilityCaster.cpp
		${MAIN_LIB_DIR}/spells/AdventureSpellMechanics.cpp
		${MAIN_LIB_DIR}/spells/BattleSpellMechanics.cpp
		${MAIN_LIB_DIR}/spells/BonusCaster.cpp
		${MAIN_LIB_DIR}/spells/CSpellHandler.cpp
		${MAIN_LIB_DIR}/spells/ExternalCaster.cpp
		${MAIN_LIB_DIR}/spells/ISpellMechanics.cpp
		${MAIN_LIB_DIR}/spells/ObstacleCasterProxy.cpp
		${MAIN_LIB_DIR}/spells/Problem.cpp
		${MAIN_LIB_DIR}/spells/ProxyCaster.cpp
		${MAIN_LIB_DIR}/spells/TargetCondition.cpp
		${MAIN_LIB_DIR}/spells/ViewSpellInt.cpp

		${MAIN_LIB_DIR}/spells/effects/Catapult.cpp
		${MAIN_LIB_DIR}/spells/effects/Clone.cpp
		${MAIN_LIB_DIR}/spells/effects/Damage.cpp
		${MAIN_LIB_DIR}/spells/effects/DemonSummon.cpp
		${MAIN_LIB_DIR}/spells/effects/Dispel.cpp
		${MAIN_LIB_DIR}/spells/effects/Effect.cpp
		${MAIN_LIB_DIR}/spells/effects/Effects.cpp
		${MAIN_LIB_DIR}/spells/effects/Heal.cpp
		${MAIN_LIB_DIR}/spells/effects/LocationEffect.cpp
		${MAIN_LIB_DIR}/spells/effects/Moat.cpp
		${MAIN_LIB_DIR}/spells/effects/Obstacle.cpp
		${MAIN_LIB_DIR}/spells/effects/Registry.cpp
		${MAIN_LIB_DIR}/spells/effects/UnitEffect.cpp
		${MAIN_LIB_DIR}/spells/effects/Summon.cpp
		${MAIN_LIB_DIR}/spells/effects/Teleport.cpp
		${MAIN_LIB_DIR}/spells/effects/Timed.cpp
		${MAIN_LIB_DIR}/spells/effects/RemoveObstacle.cpp
		${MAIN_LIB_DIR}/spells/effects/Sacrifice.cpp

		${MAIN_LIB_DIR}/vstd/StringUtils.cpp

		${MAIN_LIB_DIR}/ArtifactUtils.cpp
		${MAIN_LIB_DIR}/BasicTypes.cpp
		${MAIN_LIB_DIR}/BattleFieldHandler.cpp
		${MAIN_LIB_DIR}/CAndroidVMHelper.cpp
		${MAIN_LIB_DIR}/CArtHandler.cpp
		${MAIN_LIB_DIR}/CArtifactInstance.cpp
		${MAIN_LIB_DIR}/CBonusTypeHandler.cpp
		${MAIN_LIB_DIR}/CBuildingHandler.cpp
		${MAIN_LIB_DIR}/CConfigHandler.cpp
		${MAIN_LIB_DIR}/CConsoleHandler.cpp
		${MAIN_LIB_DIR}/CCreatureHandler.cpp
		${MAIN_LIB_DIR}/CCreatureSet.cpp
		${MAIN_LIB_DIR}/CGameInfoCallback.cpp
		${MAIN_LIB_DIR}/CGameInterface.cpp
		${MAIN_LIB_DIR}/CGeneralTextHandler.cpp
		${MAIN_LIB_DIR}/CHeroHandler.cpp
		${MAIN_LIB_DIR}/CModHandler.cpp
		${MAIN_LIB_DIR}/CModVersion.cpp
		${MAIN_LIB_DIR}/CPlayerState.cpp
		${MAIN_LIB_DIR}/CRandomGenerator.cpp
		${MAIN_LIB_DIR}/CScriptingModule.cpp
		${MAIN_LIB_DIR}/CSkillHandler.cpp
		${MAIN_LIB_DIR}/CStack.cpp
		${MAIN_LIB_DIR}/CThreadHelper.cpp
		${MAIN_LIB_DIR}/CTownHandler.cpp
		${MAIN_LIB_DIR}/GameConstants.cpp
		${MAIN_LIB_DIR}/GameSettings.cpp
		${MAIN_LIB_DIR}/IGameCallback.cpp
		${MAIN_LIB_DIR}/IHandlerBase.cpp
		${MAIN_LIB_DIR}/JsonDetail.cpp
		${MAIN_LIB_DIR}/JsonNode.cpp
		${MAIN_LIB_DIR}/JsonRandom.cpp
		${MAIN_LIB_DIR}/LoadProgress.cpp
		${MAIN_LIB_DIR}/LogicalExpression.cpp
		${MAIN_LIB_DIR}/NetPacksLib.cpp
		${MAIN_LIB_DIR}/MetaString.cpp
		${MAIN_LIB_DIR}/ObstacleHandler.cpp
		${MAIN_LIB_DIR}/StartInfo.cpp
		${MAIN_LIB_DIR}/ResourceSet.cpp
		${MAIN_LIB_DIR}/RiverHandler.cpp
		${MAIN_LIB_DIR}/RoadHandler.cpp
		${MAIN_LIB_DIR}/ScriptHandler.cpp
		${MAIN_LIB_DIR}/TerrainHandler.cpp
		${MAIN_LIB_DIR}/TextOperations.cpp
		${MAIN_LIB_DIR}/VCMIDirs.cpp
		${MAIN_LIB_DIR}/VCMI_Lib.cpp
	)

	# Version.cpp is a generated file
	if(ENABLE_GITVERSION)
		list(APPEND lib_SRCS ${CMAKE_BINARY_DIR}/Version.cpp)
		set_source_files_properties(${CMAKE_BINARY_DIR}/Version.cpp
			PROPERTIES GENERATED TRUE
		)
	endif()

	set(lib_HEADERS
		${MAIN_LIB_DIR}/../include/vstd/CLoggerBase.h
		${MAIN_LIB_DIR}/../Global.h
		${MAIN_LIB_DIR}/StdInc.h

		${MAIN_LIB_DIR}/../include/vstd/ContainerUtils.h
		${MAIN_LIB_DIR}/../include/vstd/RNG.h
		${MAIN_LIB_DIR}/../include/vstd/StringUtils.h

		${MAIN_LIB_DIR}/../include/vcmi/events/AdventureEvents.h
		${MAIN_LIB_DIR}/../include/vcmi/events/ApplyDamage.h
		${MAIN_LIB_DIR}/../include/vcmi/events/BattleEvents.h
		${MAIN_LIB_DIR}/../include/vcmi/events/Event.h
		${MAIN_LIB_DIR}/../include/vcmi/events/EventBus.h
		${MAIN_LIB_DIR}/../include/vcmi/events/GameResumed.h
		${MAIN_LIB_DIR}/../include/vcmi/events/GenericEvents.h
		${MAIN_LIB_DIR}/../include/vcmi/events/ObjectVisitEnded.h
		${MAIN_LIB_DIR}/../include/vcmi/events/ObjectVisitStarted.h
		${MAIN_LIB_DIR}/../include/vcmi/events/PlayerGotTurn.h
		${MAIN_LIB_DIR}/../include/vcmi/events/SubscriptionRegistry.h
		${MAIN_LIB_DIR}/../include/vcmi/events/TurnStarted.h

		${MAIN_LIB_DIR}/../include/vcmi/scripting/Service.h

		${MAIN_LIB_DIR}/../include/vcmi/spells/Caster.h
		${MAIN_LIB_DIR}/../include/vcmi/spells/Magic.h
		${MAIN_LIB_DIR}/../include/vcmi/spells/Service.h
		${MAIN_LIB_DIR}/../include/vcmi/spells/Spell.h

		${MAIN_LIB_DIR}/../include/vcmi/Artifact.h
		${MAIN_LIB_DIR}/../include/vcmi/ArtifactService.h
		${MAIN_LIB_DIR}/../include/vcmi/Creature.h
		${MAIN_LIB_DIR}/../include/vcmi/CreatureService.h
		${MAIN_LIB_DIR}/../include/vcmi/Entity.h
		${MAIN_LIB_DIR}/../include/vcmi/Environment.h
		${MAIN_LIB_DIR}/../include/vcmi/Faction.h
		${MAIN_LIB_DIR}/../include/vcmi/FactionService.h
		${MAIN_LIB_DIR}/../include/vcmi/HeroClass.h
		${MAIN_LIB_DIR}/../include/vcmi/HeroClassService.h
		${MAIN_LIB_DIR}/../include/vcmi/HeroType.h
		${MAIN_LIB_DIR}/../include/vcmi/HeroTypeService.h
		${MAIN_LIB_DIR}/../include/vcmi/Metatype.h
		${MAIN_LIB_DIR}/../include/vcmi/Player.h
		${MAIN_LIB_DIR}/../include/vcmi/ServerCallback.h
		${MAIN_LIB_DIR}/../include/vcmi/Services.h
		${MAIN_LIB_DIR}/../include/vcmi/Skill.h
		${MAIN_LIB_DIR}/../include/vcmi/SkillService.h
		${MAIN_LIB_DIR}/../include/vcmi/Team.h

		${MAIN_LIB_DIR}/battle/AccessibilityInfo.h
		${MAIN_LIB_DIR}/battle/BattleAction.h
		${MAIN_LIB_DIR}/battle/BattleAttackInfo.h
		${MAIN_LIB_DIR}/battle/BattleHex.h
		${MAIN_LIB_DIR}/battle/BattleInfo.h
		${MAIN_LIB_DIR}/battle/BattleStateInfoForRetreat.h
		${MAIN_LIB_DIR}/battle/BattleProxy.h
		${MAIN_LIB_DIR}/battle/CBattleInfoCallback.h
		${MAIN_LIB_DIR}/battle/CBattleInfoEssentials.h
		${MAIN_LIB_DIR}/battle/CCallbackBase.h
		${MAIN_LIB_DIR}/battle/CObstacleInstance.h
		${MAIN_LIB_DIR}/battle/CPlayerBattleCallback.h
		${MAIN_LIB_DIR}/battle/CUnitState.h
		${MAIN_LIB_DIR}/battle/DamageCalculator.h
		${MAIN_LIB_DIR}/battle/Destination.h
		${MAIN_LIB_DIR}/battle/IBattleInfoCallback.h
		${MAIN_LIB_DIR}/battle/IBattleState.h
		${MAIN_LIB_DIR}/battle/IUnitInfo.h
		${MAIN_LIB_DIR}/battle/PossiblePlayerBattleAction.h
		${MAIN_LIB_DIR}/battle/ReachabilityInfo.h
		${MAIN_LIB_DIR}/battle/SideInBattle.h
		${MAIN_LIB_DIR}/battle/SiegeInfo.h
		${MAIN_LIB_DIR}/battle/Unit.h

		${MAIN_LIB_DIR}/bonuses/Bonus.h
		${MAIN_LIB_DIR}/bonuses/BonusEnum.h
		${MAIN_LIB_DIR}/bonuses/BonusList.h
		${MAIN_LIB_DIR}/bonuses/BonusParams.h
		${MAIN_LIB_DIR}/bonuses/BonusSelector.h
		${MAIN_LIB_DIR}/bonuses/CBonusProxy.h
		${MAIN_LIB_DIR}/bonuses/CBonusSystemNode.h
		${MAIN_LIB_DIR}/bonuses/IBonusBearer.h
		${MAIN_LIB_DIR}/bonuses/Limiters.h
		${MAIN_LIB_DIR}/bonuses/Propagators.h
		${MAIN_LIB_DIR}/bonuses/Updaters.h

		${MAIN_LIB_DIR}/campaign/CampaignConstants.h
		${MAIN_LIB_DIR}/campaign/CampaignHandler.h
		${MAIN_LIB_DIR}/campaign/CampaignScenarioPrologEpilog.h
		${MAIN_LIB_DIR}/campaign/CampaignState.h

		${MAIN_LIB_DIR}/events/ApplyDamage.h
		${MAIN_LIB_DIR}/events/GameResumed.h
		${MAIN_LIB_DIR}/events/ObjectVisitEnded.h
		${MAIN_LIB_DIR}/events/ObjectVisitStarted.h
		${MAIN_LIB_DIR}/events/PlayerGotTurn.h
		${MAIN_LIB_DIR}/events/TurnStarted.h

		${MAIN_LIB_DIR}/filesystem/AdapterLoaders.h
		${MAIN_LIB_DIR}/filesystem/CArchiveLoader.h
		${MAIN_LIB_DIR}/filesystem/CBinaryReader.h
		${MAIN_LIB_DIR}/filesystem/CCompressedStream.h
		${MAIN_LIB_DIR}/filesystem/CFileInputStream.h
		${MAIN_LIB_DIR}/filesystem/CFilesystemLoader.h
		${MAIN_LIB_DIR}/filesystem/CInputOutputStream.h
		${MAIN_LIB_DIR}/filesystem/CInputStream.h
		${MAIN_LIB_DIR}/filesystem/CMemoryBuffer.h
		${MAIN_LIB_DIR}/filesystem/CMemoryStream.h
		${MAIN_LIB_DIR}/filesystem/COutputStream.h
		${MAIN_LIB_DIR}/filesystem/CStream.h
		${MAIN_LIB_DIR}/filesystem/CZipLoader.h
		${MAIN_LIB_DIR}/filesystem/CZipSaver.h
		${MAIN_LIB_DIR}/filesystem/FileInfo.h
		${MAIN_LIB_DIR}/filesystem/Filesystem.h
		${MAIN_LIB_DIR}/filesystem/ISimpleResourceLoader.h
		${MAIN_LIB_DIR}/filesystem/MinizipExtensions.h
		${MAIN_LIB_DIR}/filesystem/ResourceID.h

		${MAIN_LIB_DIR}/gameState/CGameState.h
		${MAIN_LIB_DIR}/gameState/CGameStateCampaign.h
		${MAIN_LIB_DIR}/gameState/EVictoryLossCheckResult.h
		${MAIN_LIB_DIR}/gameState/InfoAboutArmy.h
		${MAIN_LIB_DIR}/gameState/SThievesGuildInfo.h
		${MAIN_LIB_DIR}/gameState/TavernHeroesPool.h
		${MAIN_LIB_DIR}/gameState/TavernSlot.h
		${MAIN_LIB_DIR}/gameState/QuestInfo.h

		${MAIN_LIB_DIR}/logging/CBasicLogConfigurator.h
		${MAIN_LIB_DIR}/logging/CLogger.h

		${MAIN_LIB_DIR}/mapObjectConstructors/AObjectTypeHandler.h
		${MAIN_LIB_DIR}/mapObjectConstructors/CBankInstanceConstructor.h
		${MAIN_LIB_DIR}/mapObjectConstructors/CDefaultObjectTypeHandler.h
		${MAIN_LIB_DIR}/mapObjectConstructors/CObjectClassesHandler.h
		${MAIN_LIB_DIR}/mapObjectConstructors/CommonConstructors.h
		${MAIN_LIB_DIR}/mapObjectConstructors/CRewardableConstructor.h
		${MAIN_LIB_DIR}/mapObjectConstructors/DwellingInstanceConstructor.h
		${MAIN_LIB_DIR}/mapObjectConstructors/HillFortInstanceConstructor.h
		${MAIN_LIB_DIR}/mapObjectConstructors/IObjectInfo.h
		${MAIN_LIB_DIR}/mapObjectConstructors/RandomMapInfo.h
		${MAIN_LIB_DIR}/mapObjectConstructors/ShipyardInstanceConstructor.h
		${MAIN_LIB_DIR}/mapObjectConstructors/ShrineInstanceConstructor.h
		${MAIN_LIB_DIR}/mapObjectConstructors/SObjectSounds.h

		${MAIN_LIB_DIR}/mapObjects/CArmedInstance.h
		${MAIN_LIB_DIR}/mapObjects/CBank.h
		${MAIN_LIB_DIR}/mapObjects/CGCreature.h
		${MAIN_LIB_DIR}/mapObjects/CGDwelling.h
		${MAIN_LIB_DIR}/mapObjects/CGHeroInstance.h
		${MAIN_LIB_DIR}/mapObjects/CGMarket.h
		${MAIN_LIB_DIR}/mapObjects/CGObjectInstance.h
		${MAIN_LIB_DIR}/mapObjects/CGPandoraBox.h
		${MAIN_LIB_DIR}/mapObjects/CGTownBuilding.h
		${MAIN_LIB_DIR}/mapObjects/CGTownInstance.h
		${MAIN_LIB_DIR}/mapObjects/CObjectHandler.h
		${MAIN_LIB_DIR}/mapObjects/CQuest.h
		${MAIN_LIB_DIR}/mapObjects/CRewardableObject.h
		${MAIN_LIB_DIR}/mapObjects/IMarket.h
		${MAIN_LIB_DIR}/mapObjects/IObjectInterface.h
		${MAIN_LIB_DIR}/mapObjects/MapObjects.h
		${MAIN_LIB_DIR}/mapObjects/MiscObjects.h
		${MAIN_LIB_DIR}/mapObjects/ObjectTemplate.h

		${MAIN_LIB_DIR}/mapping/CDrawRoadsOperation.h
		${MAIN_LIB_DIR}/mapping/CMapDefines.h
		${MAIN_LIB_DIR}/mapping/CMapEditManager.h
		${MAIN_LIB_DIR}/mapping/CMapHeader.h
		${MAIN_LIB_DIR}/mapping/CMap.h
		${MAIN_LIB_DIR}/mapping/CMapInfo.h
		${MAIN_LIB_DIR}/mapping/CMapOperation.h
		${MAIN_LIB_DIR}/mapping/CMapService.h
		${MAIN_LIB_DIR}/mapping/MapEditUtils.h
		${MAIN_LIB_DIR}/mapping/MapIdentifiersH3M.h
		${MAIN_LIB_DIR}/mapping/MapFeaturesH3M.h
		${MAIN_LIB_DIR}/mapping/MapFormatH3M.h
		${MAIN_LIB_DIR}/mapping/MapFormat.h
		${MAIN_LIB_DIR}/mapping/MapReaderH3M.h
		${MAIN_LIB_DIR}/mapping/MapFormatJson.h
		${MAIN_LIB_DIR}/mapping/ObstacleProxy.h

		${MAIN_LIB_DIR}/pathfinder/INodeStorage.h
		${MAIN_LIB_DIR}/pathfinder/CGPathNode.h
		${MAIN_LIB_DIR}/pathfinder/CPathfinder.h
		${MAIN_LIB_DIR}/pathfinder/NodeStorage.h
		${MAIN_LIB_DIR}/pathfinder/PathfinderOptions.h
		${MAIN_LIB_DIR}/pathfinder/PathfinderUtil.h
		${MAIN_LIB_DIR}/pathfinder/PathfindingRules.h
		${MAIN_LIB_DIR}/pathfinder/TurnInfo.h

		${MAIN_LIB_DIR}/registerTypes/RegisterTypes.h

		${MAIN_LIB_DIR}/rewardable/Configuration.h
		${MAIN_LIB_DIR}/rewardable/Info.h
		${MAIN_LIB_DIR}/rewardable/Interface.h
		${MAIN_LIB_DIR}/rewardable/Limiter.h
		${MAIN_LIB_DIR}/rewardable/Reward.h

		${MAIN_LIB_DIR}/rmg/RmgArea.h
		${MAIN_LIB_DIR}/rmg/RmgObject.h
		${MAIN_LIB_DIR}/rmg/RmgPath.h
		${MAIN_LIB_DIR}/rmg/CMapGenerator.h
		${MAIN_LIB_DIR}/rmg/CMapGenOptions.h
		${MAIN_LIB_DIR}/rmg/CRmgTemplate.h
		${MAIN_LIB_DIR}/rmg/CRmgTemplateStorage.h
		${MAIN_LIB_DIR}/rmg/CZonePlacer.h
		${MAIN_LIB_DIR}/rmg/TileInfo.h
		${MAIN_LIB_DIR}/rmg/Zone.h
		${MAIN_LIB_DIR}/rmg/RmgMap.h
		${MAIN_LIB_DIR}/rmg/float3.h
		${MAIN_LIB_DIR}/rmg/Functions.h
		${MAIN_LIB_DIR}/rmg/modificators/Modificator.h
		${MAIN_LIB_DIR}/rmg/modificators/ObjectManager.h
		${MAIN_LIB_DIR}/rmg/modificators/ObjectDistributor.h
		${MAIN_LIB_DIR}/rmg/modificators/RoadPlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/TreasurePlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/QuestArtifactPlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/ConnectionsPlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/WaterAdopter.h
		${MAIN_LIB_DIR}/rmg/modificators/MinePlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/TownPlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/WaterProxy.h
		${MAIN_LIB_DIR}/rmg/modificators/WaterRoutes.h
		${MAIN_LIB_DIR}/rmg/modificators/RockPlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/RockFiller.h
		${MAIN_LIB_DIR}/rmg/modificators/ObstaclePlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/RiverPlacer.h
		${MAIN_LIB_DIR}/rmg/modificators/TerrainPainter.h
		${MAIN_LIB_DIR}/rmg/threadpool/BlockingQueue.h
		${MAIN_LIB_DIR}/rmg/threadpool/ThreadPool.h
		${MAIN_LIB_DIR}/rmg/threadpool/MapProxy.h

		${MAIN_LIB_DIR}/serializer/BinaryDeserializer.h
		${MAIN_LIB_DIR}/serializer/BinarySerializer.h
		${MAIN_LIB_DIR}/serializer/CLoadIntegrityValidator.h
		${MAIN_LIB_DIR}/serializer/CMemorySerializer.h
		${MAIN_LIB_DIR}/serializer/Connection.h
		${MAIN_LIB_DIR}/serializer/CSerializer.h
		${MAIN_LIB_DIR}/serializer/CTypeList.h
		${MAIN_LIB_DIR}/serializer/JsonDeserializer.h
		${MAIN_LIB_DIR}/serializer/JsonSerializeFormat.h
		${MAIN_LIB_DIR}/serializer/JsonSerializer.h
		${MAIN_LIB_DIR}/serializer/JsonUpdater.h
		${MAIN_LIB_DIR}/serializer/ILICReader.h
		${MAIN_LIB_DIR}/serializer/Cast.h

		${MAIN_LIB_DIR}/spells/AbilityCaster.h
		${MAIN_LIB_DIR}/spells/AdventureSpellMechanics.h
		${MAIN_LIB_DIR}/spells/BattleSpellMechanics.h
		${MAIN_LIB_DIR}/spells/BonusCaster.h
		${MAIN_LIB_DIR}/spells/CSpellHandler.h
		${MAIN_LIB_DIR}/spells/ExternalCaster.h
		${MAIN_LIB_DIR}/spells/ISpellMechanics.h
		${MAIN_LIB_DIR}/spells/ObstacleCasterProxy.h
		${MAIN_LIB_DIR}/spells/Problem.h
		${MAIN_LIB_DIR}/spells/ProxyCaster.h
		${MAIN_LIB_DIR}/spells/TargetCondition.h
		${MAIN_LIB_DIR}/spells/ViewSpellInt.h

		${MAIN_LIB_DIR}/spells/effects/Catapult.h
		${MAIN_LIB_DIR}/spells/effects/Clone.h
		${MAIN_LIB_DIR}/spells/effects/Damage.h
		${MAIN_LIB_DIR}/spells/effects/DemonSummon.h
		${MAIN_LIB_DIR}/spells/effects/Dispel.h
		${MAIN_LIB_DIR}/spells/effects/Effect.h
		${MAIN_LIB_DIR}/spells/effects/Effects.h
		${MAIN_LIB_DIR}/spells/effects/EffectsFwd.h
		${MAIN_LIB_DIR}/spells/effects/Heal.h
		${MAIN_LIB_DIR}/spells/effects/LocationEffect.h
		${MAIN_LIB_DIR}/spells/effects/Obstacle.h
		${MAIN_LIB_DIR}/spells/effects/Registry.h
		${MAIN_LIB_DIR}/spells/effects/UnitEffect.h
		${MAIN_LIB_DIR}/spells/effects/Summon.h
		${MAIN_LIB_DIR}/spells/effects/Teleport.h
		${MAIN_LIB_DIR}/spells/effects/Timed.h
		${MAIN_LIB_DIR}/spells/effects/RemoveObstacle.h
		${MAIN_LIB_DIR}/spells/effects/Sacrifice.h

		${MAIN_LIB_DIR}/AI_Base.h
		${MAIN_LIB_DIR}/ArtifactUtils.h
		${MAIN_LIB_DIR}/BattleFieldHandler.h
		${MAIN_LIB_DIR}/CAndroidVMHelper.h
		${MAIN_LIB_DIR}/CArtHandler.h
		${MAIN_LIB_DIR}/CArtifactInstance.h
		${MAIN_LIB_DIR}/CBonusTypeHandler.h
		${MAIN_LIB_DIR}/CBuildingHandler.h
		${MAIN_LIB_DIR}/CConfigHandler.h
		${MAIN_LIB_DIR}/CConsoleHandler.h
		${MAIN_LIB_DIR}/CCreatureHandler.h
		${MAIN_LIB_DIR}/CCreatureSet.h
		${MAIN_LIB_DIR}/CGameInfoCallback.h
		${MAIN_LIB_DIR}/CGameInterface.h
		${MAIN_LIB_DIR}/CGeneralTextHandler.h
		${MAIN_LIB_DIR}/CHeroHandler.h
		${MAIN_LIB_DIR}/CModHandler.h
		${MAIN_LIB_DIR}/CModVersion.h
		${MAIN_LIB_DIR}/CondSh.h
		${MAIN_LIB_DIR}/ConstTransitivePtr.h
		${MAIN_LIB_DIR}/Color.h
		${MAIN_LIB_DIR}/CPlayerState.h
		${MAIN_LIB_DIR}/CRandomGenerator.h
		${MAIN_LIB_DIR}/CScriptingModule.h
		${MAIN_LIB_DIR}/CSkillHandler.h
		${MAIN_LIB_DIR}/CSoundBase.h
		${MAIN_LIB_DIR}/CStack.h
		${MAIN_LIB_DIR}/CStopWatch.h
		${MAIN_LIB_DIR}/CThreadHelper.h
		${MAIN_LIB_DIR}/CTownHandler.h
		${MAIN_LIB_DIR}/FunctionList.h
		${MAIN_LIB_DIR}/GameConstants.h
		${MAIN_LIB_DIR}/GameSettings.h
		${MAIN_LIB_DIR}/IBonusTypeHandler.h
		${MAIN_LIB_DIR}/IGameCallback.h
		${MAIN_LIB_DIR}/IGameEventsReceiver.h
		${MAIN_LIB_DIR}/IHandlerBase.h
		${MAIN_LIB_DIR}/int3.h
		${MAIN_LIB_DIR}/JsonDetail.h
		${MAIN_LIB_DIR}/JsonNode.h
		${MAIN_LIB_DIR}/JsonRandom.h
		${MAIN_LIB_DIR}/Languages.h
		${MAIN_LIB_DIR}/LoadProgress.h
		${MAIN_LIB_DIR}/LogicalExpression.h
		${MAIN_LIB_DIR}/MetaString.h
		${MAIN_LIB_DIR}/NetPacksBase.h
		${MAIN_LIB_DIR}/NetPacks.h
		${MAIN_LIB_DIR}/NetPacksLobby.h
		${MAIN_LIB_DIR}/NetPackVisitor.h
		${MAIN_LIB_DIR}/ObstacleHandler.h
		${MAIN_LIB_DIR}/Point.h
		${MAIN_LIB_DIR}/Rect.h
		${MAIN_LIB_DIR}/Rect.cpp
		${MAIN_LIB_DIR}/ResourceSet.h
		${MAIN_LIB_DIR}/RiverHandler.h
		${MAIN_LIB_DIR}/RoadHandler.h
		${MAIN_LIB_DIR}/ScriptHandler.h
		${MAIN_LIB_DIR}/ScopeGuard.h
		${MAIN_LIB_DIR}/StartInfo.h
		${MAIN_LIB_DIR}/StringConstants.h
		${MAIN_LIB_DIR}/TerrainHandler.h
		${MAIN_LIB_DIR}/TextOperations.h
		${MAIN_LIB_DIR}/UnlockGuard.h
		${MAIN_LIB_DIR}/VCMIDirs.h
		${MAIN_LIB_DIR}/vcmi_endian.h
		${MAIN_LIB_DIR}/VCMI_Lib.h
	)

	assign_source_group(${lib_SRCS} ${lib_HEADERS})

	add_library(${TARGET_NAME} ${LIBRARY_TYPE} ${lib_SRCS} ${lib_HEADERS})
	set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS "VCMI_DLL=1")
	target_link_libraries(${TARGET_NAME} PUBLIC
		minizip::minizip ZLIB::ZLIB
		${SYSTEM_LIBS} Boost::boost Boost::filesystem Boost::program_options Boost::locale Boost::date_time
	)
	if(APPLE_IOS)
		target_link_libraries(${TARGET_NAME} PUBLIC iOS_utils)
	endif()

	target_include_directories(${TARGET_NAME}
		PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
		PUBLIC ${MAIN_LIB_DIR}/..
		PUBLIC ${MAIN_LIB_DIR}/../include
		PUBLIC ${MAIN_LIB_DIR}
	)

	if(WIN32)
		set_target_properties(${TARGET_NAME}
			PROPERTIES
				OUTPUT_NAME "VCMI_lib"
				PROJECT_LABEL "VCMI_lib"
		)
	endif()

	vcmi_set_output_dir(${TARGET_NAME} "")

	enable_pch(${TARGET_NAME})

	# We want to deploy assets into build directory for easier debugging without install
	if(COPY_CONFIG_ON_BUILD)
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
			COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${MAIN_LIB_DIR}/../config ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${MAIN_LIB_DIR}/../Mods ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
		)
	endif()

	# Update version before vcmi compiling
	if(TARGET update_version)
		add_dependencies(${TARGET_NAME} update_version)
	endif()

	if("${LIBRARY_TYPE}" STREQUAL SHARED)
		install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
	endif()
	if(APPLE_IOS AND NOT USING_CONAN)
		get_target_property(LINKED_LIBS ${TARGET_NAME} LINK_LIBRARIES)
		foreach(LINKED_LIB IN LISTS LINKED_LIBS)
			if(NOT TARGET ${LINKED_LIB})
				if(LINKED_LIB MATCHES "\\${CMAKE_SHARED_LIBRARY_SUFFIX}$")
					install(FILES ${LINKED_LIB} DESTINATION ${LIB_DIR})
				endif()
				continue()
			endif()

			get_target_property(LIB_TYPE ${LINKED_LIB} TYPE)
			if(NOT LIB_TYPE STREQUAL "SHARED_LIBRARY")
				continue()
			endif()

			get_target_property(_aliased ${LINKED_LIB} ALIASED_TARGET)
			if(_aliased)
				set(LINKED_LIB_REAL ${_aliased})
			else()
				set(LINKED_LIB_REAL ${LINKED_LIB})
			endif()

			get_target_property(_imported ${LINKED_LIB_REAL} IMPORTED)
			if(_imported)
				set(INSTALL_TYPE IMPORTED_RUNTIME_ARTIFACTS)
				get_target_property(BOOST_DEPENDENCIES ${LINKED_LIB_REAL} INTERFACE_LINK_LIBRARIES)
				foreach(BOOST_DEPENDENCY IN LISTS BOOST_DEPENDENCIES)
					get_target_property(BOOST_DEPENDENCY_TYPE ${BOOST_DEPENDENCY} TYPE)
					if(BOOST_DEPENDENCY_TYPE STREQUAL "SHARED_LIBRARY")
						install(IMPORTED_RUNTIME_ARTIFACTS ${BOOST_DEPENDENCY} LIBRARY DESTINATION ${LIB_DIR})
					endif()
				endforeach()
			else()
				set(INSTALL_TYPE TARGETS)
			endif()
			install(${INSTALL_TYPE} ${LINKED_LIB_REAL} LIBRARY DESTINATION ${LIB_DIR})
		endforeach()
	endif()
endmacro()
