# AI

There are two types of AI: adventure and battle.

**Adventure AIs** are responsible for moving heroes across the map and developing towns  
**Battle AIs** are responsible for fighting, i.e. moving stacks on the battlefield  

We have 3 battle AIs so far:
* BattleAI - strongest
* StupidAI - for neutrals, should be simple so that experienced players can abuse it
* Empty AI - should do nothing at all. If needed another battle AI can be introduced.  

Each battle AI consist of a few classes, but the main class, kind of entry point usually has the same name as the package itself. In BattleAI it is the BattleAI class. It implements some battle specific interface, do not remember. Main method there is activeStack(battle::Unit* stack). It is invoked by the system when it's time to move your stack. The thing you use to interact with the game and receive the gamestate is usually referenced in the code as cb. CPlayerSpecificCallback it should be. It has a lot of methods and can do anything. For instance it has battleGetUnitsIf(), which returns all units on the battlefield matching some lambda condition.
Each side in a battle is represented by an CArmedInstance object. CHeroInstance and CGDwelling, CGMonster and more are subclasses of CArmedInstance. CArmedInstance contains a set of stacks. When the battle starts, these stacks are converted to battle stacks. Usually Battle AIs reference them using the interface battle::Unit *.
Units have bonuses. Nearly everything aspect of a unit is configured in the form of bonuses. Attack, defense, health, retaliation, shooter or not, initial count of shots and so on.
When you call unit->getAttack() it summarizes all these bonuses and returns the resulting value.  

One important class is HypotheticBattle. It is used to evaluate the effects of an action without changing the actual gamestate. It is a wrapper around CPlayerSpecificCallback or another HypotheticBattle so it can provide you data, Internally it has a set of modified unit states and intercepts some calls to underlying callback and returns these internal states instead. These states in turn are wrappers around original units and contain modified bonuses (CStackWithBonuses). So if you need to emulate an attack you can call hypotheticbattle.getforupdate() and it will return the CStackWithBonuses which you can safely change.  

## BattleAI  

BattleAI's most important classes are the following:

- AttackPossibility - one particular way to attack on the battlefield. Each AttackPossibility instance has multiple ...DamageReduce attributes. These represent how much damage an enemy will lose after our attack. Effects can reduce this damage. We add them up and this value is used as attack score.

- PotentialTargets - a set of all AttackPossibility instances

- BattleExchangeVariant - it is an extension of AttackPossibility, a result of a set of units attacking each other for a fixed number of turns according to the turn order. It is kind of an oversimplified battle simulation. A set of units is restricted according to AttackPossibility which particular exchange extends. Exchanges can be waited (when stacks/units wait for a better time to attack) and non-waited (when stack acts right away). For non-waited exchanges the first attack score is taken from AttackPossibility (together with various effects like 2 hex breath, shooters blocking and so on). All the other attacks are simplified, only respect retaliations. At the end we have a final score.

- BattleExchangeEvaluator - calculates all possible BattleExchangeVariants and selects the best

- BattleEvaluator - is a top level logic layer which also adds spellcasts and movement to unreachable targets

BattleAI itself handles all the rest and issues actual commands

## Nullkiller AI

Adventure AI responsible for moving heroes on map, gathering things, developing town. Main idea is to gather all possible tasks on map, prioritize them and select the best one for each heroes. Initially was a fork of VCAI

### Parts
Gateway - a callback for server used to invoke AI actions when server thinks it is time to do something. Through this callback AI is informed about various events like hero level up, tile revialed, blocking dialogs and so on. In order to do this Gaateway implements specific interface. The interface is exactly the same for human and AI
Another important actor for server interaction is CCallback * cb. This one is used to retrieve gamestate information and ask server to do things like hero moving, spell casting and so on. Each AI has own instance of Gateway and it is a root object which holds all AI state. Gateway has an event method yourTurn which invokes makeTurn in another thread. The last passes control to Nullkiller engine.

Nullkiller engine - place where actual AI logic is organized. It contains a main loop for gathering and prioritizing things. Its algorithm:
* reset AI state, it avoids keeping some memory about the game in general to reduce amount of things serialized into savefile state. The only serialized things are in nullkiller->memory. This helps reducing save incompatibility. It should be mostly enough for AI to analyze data avaialble in CCallback
* main loop, loop iteration is called a pass
** update AI state, some state is lazy and updates once per day to avoid performance hit, some state is recalculated each loop iteration. At this stage analysers and pathfidner work
** gathering goals, prioritizing and decomposing them
** execute selected best goals

Analyzer - a module gathering data from CCallback *. Its goal to make some statistics and avoid making any significant decissions.
* HeroAnalyser - decides upong which hero suits better to be main (army carrier and fighter) and which is better to be a scout (gathering unguarded resources, exploring)
* BuildAnalyzer - prepares information on what we can build in our towns, and what resources we need to do this
* DangerHitMapAnalyser - checks if enemy hero can rich each tile, how fast and what is their army strangth
* Pathfinder - core thing used to calculate paths including bypassing monsters, quests, using advmap spells
* Graph - experimental thing connecting all objects into a network by paths (using common hero characteristics), does not work without maphack, allows simplified faster paths calculation. Possibly can be used for something else
* ArmyManager - for now only helps calculating best army from two CreatureSet objects. Later may be responsible for forming ideal army for each main hero so that we know what we need to build and buy.
* ObjectClusterizer - aggregates all objects into clusters depending on which object blocks way towards them.
* DeepDecomposer - sometimes pathfinder may return path through some object which canno be simply bypassed but instead it requires something to be done first. DeepDecomposer allows to detalizing such paths. Examples: building a boat requires capturing shipyard, bypassing bordergate requires visiting masterkey tent. See AbstractGoal
* FuzzyEngines - looks like some legacy from VCAI
* PriorityEvaluator - gathers information on task rewards, evaluates their priority using Fuzzy Light library (fuzzy logic)

### Goals
Units of activity in AI. Can be AbstractGoal, Task, Marker and Behavior

Task - simple thing which can be done right away in order to gain some reward. Or a composition of simple things in case if more than one action is needed to gain the reward.
* AdventureSpellCast - town portal, water walk, air walk, summon boat
* BuildBoat - builds a boat in a specific shipyard
* BuildThis - builds a building in a specified town
* BuyArmy - buys specific amount of army in AIValue in specific town
* DigAtTile - for grail, not implemented yet
* DismissHero - sometimes we may want to get rid of some scout
* ExchangeSwapTownHeroes - puts specifc hero in garrison or extracts hero from garrison. Also makes possible upgrades and buys army in town (used by defence and startup behaviors)
* ExecuteHeroChain - moves hero accross some path (or a few heroes forming a chain in order to move army to the target hero), can bypass simple obstacles like monsters, garrisons
* ExploreNeighborTile - after AI visits initial tile for exploration - makes a few sequential explorations of nearby tiles to save some performance
* RecruitHero - recruits specific hero in specifc town
* SaveResources - locks some resources for later use during next day
* StayAtTown - stay at town for the rest of the day (to regain mana)

Behavior - a core game activity
* CaptureObjectsBehavior - generally it is about visiting map objects which give reward. It can capture any object, even those which are behind monsters and so on. But due to performance considerations it is not allowed to handle monsters and quests now.
* ClusterBehavior - uses information of ObjectClusterizer to unblock objects hidden behind various blockers. It kills guards, completes quests, captures garrisons.
* BuildingBehavior - develops our towns
* BuyArmyBehavior - buys army in towns
* GatherArmyBehavior - picks army from towns and brings it to main hero by scout, or main itslef goes for it
* RecruitHeroBehavior - recruits hero it it is either stronger than any main or there is something to gather
* StartupBehavior - scripted behavior which helps a bit on the first day. It keeps main hero in town garrison and accumulates army from initial heroes bought in tavern
* StayAtTownBehavior - stay at town to gain mana from mage guild
* DefenceBehavior - defend towns by eliminating treatening heroes or hiding in town garrison

AbstractGoal - some goals can not be completed because it is not clear how to do this. They express desire to do something, not exact plan. DeepDecomposer is used to refine such goals until they are turned into such plan or discarded. Some examples:
* CaptureObject - you need to visit some object (flag a shipyard for instance) but do not know how
* CompleteQuest - you need to bypass bordergate or borderguard or questguard but do not know how
AbstractGoal usually comes in form of composition with some elementar task blocked by abstract objective. For instance CaptureObject(Shipyard), ExecuteHeroChain(visit x, build boat, visit enemy town). When such composition is decomposed it can turn into either a pair of herochains or into another abstract composition if path to shipyard is also blocked with something.
Sometimes such decomposition may form a loop of abstract goals and will be discarded in such case. Generally the current architecture attempts to avoid decomposition as quite a heavy operation.

Composition - a goal which can be both elementar (a set of tasks) or abstract (contains unresolved abstract goal at the end). Compositions express a chain of tasks in order to achieve some reward. They consist of sequences. Each sequence is a vector of goals. Only last sequence is actually executed or decomposed. All the rest adds value to reward evaluator.

Marker - a goal used to just add value (reward) into some composition. We want to capture some shipyard not just because but in order to capture a town (or something else) later. Thus when we are capturing a shipyard we should know that later we will unlock town so we contribute towards town reward as well.
