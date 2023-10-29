< [Documentation](../Readme.md) /  AI Description

There are two types of AI: adventure and battle.

**Adventure AIs** are responsible for moving heroes across the map and developing towns  
**Battle AIs** are responsible for fighting, i.e. moving stacks on the battlefield  

We have 3 battle AIs so far:
* BattleAI - strongest
* StupidAI - for neutrals, should be simple so that experienced players can abuse it
* Empty AI - should do nothing at all. If needed another battle AI can be introduced.  

Each battle AI consist of a few classes, but the main class, kind of entry point usually has the same name as the package itself. In BattleAI it is the BattleAI class. It implements some battle specific interface, do not remember. Main method there is activeStack(battle::Unit* stack). It is invoked by the system when it's time to move your stack. The thing you use to interact with the game and receive the gamestate is usually referenced in the code as cb. CPlayerSpecificCallback it should be. It has a lot of methods and can do anything. For instance it has battleGetUnitsIf(), which returns all units on the battlefield matching some lambda condition.
Each side in a battle is represented by an CArmedInstance object. CHeroInstance and CGDwelling, CGMonster and more are subclasses of CArmedInstance. CArmedInstance contains a set of stacks. When the battle starts, these stacks are converted to battle stacks. Usually Battle AIs reference them using the interface battle::Unit *.
Units have bonuses. Nearly everything aspect of a unit is configured in the form of bonuses. Attack, defense, health, retalitation, shooter or not, initial count of shots and so on.
When you call unit->getAttack() it summarizes all these bonuses and returns the resulting value.  

One important class is HypotheticBattle. It is used to evaluate the effects of an action without changing the actual gamestate. It is a wrapper around CPlayerSpecificCallback or another HypotheticBattle so it can provide you data, Internally it has a set of modified unit states and intercepts some calls to underlying callback and returns these internal states instead. These states in turn are wrappers around original units and contain modified bonuses (CStackWithBonuses). So if you need to emulate an attack you can call hypotheticbattle.getforupdate() and it will return the CStackWithBonuses which you can safely change.  

# BattleAI  

BattleAI's most important classes are the following:

- AttackPossibility - one particular way to attack on the battlefield. Each AttackPossibility instance has multiple ...DamageReduce attributes. These represent how much damage an enemy will lose after our attack. Effects can reduce this damage. We add them up and this value is used as attack score.

- PotentialTargets - a set of all AttackPossibility instances

- BattleExchangeVariant - it is an extension of AttackPossibility, a result of a set of units attacking each other for a fixed number of turns according to the turn order. It is kind of an oversimplified battle simulation. A set of units is restricted according to AttackPossibility which particular exchange extends. Exchanges can be waited (when stacks/units wait for a better time to attack) and non-waited (when stack acts right away). For non-waited exchanges the first attack score is taken from AttackPossibility (together with various effects like 2 hex breath, shooters blocking and so on). All the other attacks are simplified, only respect retaliations. At the end we have a final score.

- BattleExchangeEvaluator - calculates all possible BattleExchangeVariants and selects the best

- BattleEvaluator - is a top level logic layer which also adds spellcasts and movement to unreachable targets

BattleAI itself handles all the rest and issues actual commands
