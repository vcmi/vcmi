# Configuration

``` javascript
{
 	//general purpose script, Lua or ERM, runs on server
 	"myScript":
	{
		"source":"path/to/script/with/ext",
		"implements":"ANYTHING"
	},


 	//custom battle spell effect, Lua only, runs on both client and server
 	//script ID will be used as effect 'type' (with mod prefix)
 	"mySpellEffect":
	{
		"source":"path/to/script/with/ext",
		"implements":"BATTLE_EFFECT"
	},

	//TODO: object|building type
 	//custom map object or building, Lua only, runs on both client and server
 	//script ID will be used as 'handler' (with mod prefix)
 	"myObjectType":
	{
		"source":"path/to/script/with/ext",
		"implements":"MAP_OBJECT"
	},
	//TODO: server query
	//TODO: client query

}
```

# Lua

## API Reference

TODO **In near future Lua API may change drastically several times.
Information here may be outdated**

### Globals

-   DATA - persistent table
    -   DATA.ERM contains ERM state, anything else is free to use.
-   GAME - IGameInfoCallback API
-   BATTLE - IBattleInfoCallback API
-   EVENT_BUS - opaque handle, for use with events API
-   SERVICES - root "raw" access to all static game objects
    -   SERVICES:artifacts()
    -   SERVICES:creatures()
    -   SERVICES:factions()
    -   SERVICES:heroClasses()
    -   SERVICES:heroTypes()
    -   SERVICES:spells()
    -   SERVICES:skills()
-   require(URI)
    -   works similar to usual Lua require
    -   require("ClassName") - loads additional API and returns it as
        table (for C++ classes)
    -   require("core:relative.path.to.module") - loads module from
        "SCRIPTS/LIB"
    -   TODO require("modName:relative.path.to.module") - loads module
        from dependent mod
    -   TODO require(":relative.path.to.module") - loads module from
        same mod
-   logError(text) - backup error log function

### Low level events API

``` Lua

-- Each event type must be loaded first
local PlayerGotTurn = require("events.PlayerGotTurn")

-- in this example subscription handles made global, do so if there is no better place
-- !!! do not store them in local variables
sub1 = 	PlayerGotTurn.subscribeAfter(EVENT_BUS, function(event)
		--do smth
	end)

sub2 = 	PlayerGotTurn.subscribeBefore(EVENT_BUS, function(event)
		--do smth
	end)
```

### Lua standard library

VCMI uses LuaJIT, which is Lua 5.1 API, see [upstream
documentation](https://www.lua.org/manual/5.1/manual.html)

Following libraries are supported

-   base
-   table
-   string
-   math
-   bit

# ERM

## Features

-   no strict limit on function/variable numbers (technical limit 32 bit
    integer except 0))
-   TODO semi compare
-   DONE macros

## Bugs

-   TODO Broken XOR support (clashes with \`X\` option)

## Triggers

-   TODO **!?AE** Equip/Unequip artifact
-   WIP **!?BA** when any battle occurs
-   WIP **!?BF** when a battlefield is prepared for a battle
-   TODO **!?BG** at every action taken by any stack or by the hero
-   TODO **!?BR** at every turn of a battle
-   *!?CM (client only) click the mouse button.*
-   TODO **!?CO** Commander triggers
-   TODO **!?DL** Custom dialogs
-   DONE **!?FU** function
-   TODO **!?GE** "global" event
-   TODO **!?GM** Saving/Loading
-   TODO **!?HE** when the hero \# is attacked by an enemy hero or
    visited by an allied hero
-   TODO **!?HL** hero gains a level
-   TODO **!?HM** every step a hero \# takes
-   *!?IP Multiplayer support.*
-   TODO **!?LE** (!$LE) An Event on the map
-   WIP **!?MF** stack taking physical damage(before an action)
-   TODO **!?MG** casting on the adventure map
-   *!?MM scroll text during a battle*
-   TODO **!?MR** Magic resistance
-   TODO **!?MW** Wandering Monsters
-   WIP **!?OB** (!$OB) visiting objects
-   DONE **!?PI** Post Instruction.
-   TODO **!?SN** Sound and ERA extensions
-   *!?TH town hall*
-   TODO **!?TL** Real-Time Timer
-   TODO **!?TM** timed events

## Receivers

### VCMI

-   **!!MC:S@varName@** - declare new "normal" variable (technically
    v-var with string key)
-   TODO Identifier resolver
-   WIP Bonus system

### ERA

-   DONE !!if !!el !!en
-   TODO !!br !!co
-   TODO !!SN:X

### WoG

-   TODO !!AR Артефакт (ресурс) в определенной позиции
-   TODO !!BA Битва
    -   !!BA:A$ return 1 for battle evaluation
-   TODO !!BF Препятствия на поле боя
-   TODO !!BG Действий монстров в бою
-   TODO !!BH Действия героя в бою
-   TODO !!BM Монстр в битве
-   WIP !!BU Универсальные параметры битвы
-   TODO !!CA Замок
-   TODO !!CD Разрушения замков
-   TODO !!CE События в замке
-   TODO !!CM Клика мышью
-   TODO !!DL Нестандартный диалог (только ТЕ или выше)
-   TODO !!CO Командиры
-   WIP !!DO Многократный вызов функции
-   TODO !!EA Бонусы опыта существ
-   TODO !!EX Опыт стека
-   DONE !!FU Однократный вызов функции
-   TODO !!GE Глобальное событие
-   WIP !!HE Герой
-   TODO !!HL Новый уровень героя
-   TODO !!HO Взаимодействия героев
-   TODO !!HT Подсказки по правому клику
-   WIP !!IF Диалоги и флагов
-   TODO !!IP Сетевой сервис битвы
-   TODO !!LE Локальное события
-   WIP !!MA Общие параметры монстров
-   DONE !!MC Макросы
-   WIP !!MF Получение физ. урона в бою
-   TODO !!MM Текст в битве
-   WIP !!MO Монстр в определенной позиции
-   TODO !!MP Контроль MP3
-   TODO !!MR Сопротивления магии
-   TODO !!MW Бродячих монстров
-   WIP !!OB Объект в определенной позиции
-   TODO !!OW Параметры игрока
-   TODO !!PM Пирамиды или новые объекты
-   TODO !!PO Информация квадрата карты
-   TODO (???) !!QW Журнала
-   TODO !!SN Проигрываемые звуков
-   TODO !!SS Настройка заклинаний (только ТЕ или выше)
-   TODO !!TL Контроль времени хода (только ТЕ или выше)
-   TODO !!TM Временный таймер
-   TODO !!TR Квадрата карты (почва, проходимость, т.п.)
-   TODO !!UN Универсальная команда
-   *!#VC Контроль переменных*
-   WIP !!VR Установка переменных

## Persistence