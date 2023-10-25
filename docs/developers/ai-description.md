< [Documentation](../Readme.md) /  AI Description - WIP

... excrept from Slack chat with Danylchenko

There are two types of AI: adventure and battle.

**Adventure AIs** are responsible on moving heroes on map and developing towns  
**Battle AIs** are responsible on actually fighting: moving stack on battlefield  

We have 3 battle AIs so far: BattleAI - strongest, StupidAI - for neutrals, should be simple so that expirienced players can abuse it. And Empty AI - should do nothing at all. If needed another battle AI can be introduced.  


Each battle AI consist of a few classes, but the main class, kind of entry point usually has same name as package itself. In BattleAI it is BattleAI class. It implements some battle specific interface, do not remember. Main method there is activeStack(battle::Unit* stack). It is invoked by system when it is time to move your stack. The thing you use to interact with game and recieve gamestate is usually referencesi n code as cb. CPlayerSpecificCallback it should be. It has a lot of methods and can do anything. For instance it has battleGetUnitsIf() can retrieve all units on battlefield matching some lambda condition.
sides in battle are represented by two CArmedInstance objects. CHeroInstance is subclass of CArmedInstance. Same can be said abot CGDwelling, CGMonster and so on. CArmedInstance contains a set of stacks. When battle starts these stacks are converted in battle stacks. Usually Battle AIs reference them using interface battle::Unit *.
Units have bonuses. Mostly everything about unit is configured in form of bonuses. Attack, defense, health, retalitation, shooter or not, initial count of shots and so on.
So when you call unit->getAttack() it summarize all these bonuses and return resulting value.  

Now important part is HypotheticBattle. It is used to evaluate some effect, action without changing real gamestate. First of all it is a wrapper around CPlayerSpecificCallback or anther HypotheticBattle so it can provide you data, Internally it has a set of modified unit states and intercepts some calls to underlying callback and returns these internal states instead. These states in turn are wrappers around original units and contain modified bonuses (CStackWithBonuses). So if you need to emulate attack you can ask hypotheticbattle.getforupdate() and it wil lreturn you this CStackWithBonuses which you can safely change.  

Now about BattleAI. All possible attacks are measured using value I called damage reduce. It is how much damage enemy will loose after our attack. Also for negative effects we have our damage reduce. We get a difference and this value is used as attack score.  

AttackPossibility - one particular way to attack on battlefield.
PotentialTargets - a set of all AttackPossibility
BattleExchangeVariant - it is extension of AttackPossibility, a result of a set of units attacking each other fixed amount of turns according to turn order. Kind of oversimplified battle simulation. A set of units is restricted according to AttackPossibility which particular exchange extends. Exchanges can be waited (when stacks/units wait better time to attack) and non-waited (when stack acts right away). For non-waited exchanges the first attack score is taken from AttackPossibility (together with various effects like 2hex breath, shooters blocking and so on). All the other attacks are simplified, only respect retaliations. At the end we have a final score.  

BattleExchangeEvaluator - calculates all possible BattleExchangeVariant and select the best  
BattleEvaluator - is top level logic layer also adds spellcasts and movement to unreachable targets  
BattleAI itself handles all the rest and issue actual commands
