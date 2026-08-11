local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Kills creatures of the attacked stack outright, as the Mighty Gorgon's gaze and the sea dog's
--- accurate shot do. Scripted equivalent of the DEATH_STARE bonus.
---
--- Parameters:
---  val       - chance for each creature of the bearer's stack to kill one, in percent. For the
---              "commander" situation it is instead a flat number of kills, scaled by the level
---              ratio of the two stacks
---  situation - when the ability applies: "melee", "ranged", "rangedDistancePenalty",
---              "rangedWallPenalty", "rangedDistanceAndWallPenalty" or "commander"
---  spell     - spell cast to kill them. Defaults to death stare, and is what decides the
---              animation, the immunities and the wording of the combat log

local SPELL = "core:deathStare"

--- Which of the situations the attack that just happened is.
local function situationOf(battle, unit, other, payload)
	if not payload.ranged then return "melee" end

	local distance = battle:hasDistancePenalty(unit, other)
	local wall = battle:hasWallPenalty(unit, other)

	if distance and wall then return "rangedDistanceAndWallPenalty" end
	if distance then return "rangedDistancePenalty" end
	if wall then return "rangedWallPenalty" end

	return "ranged"
end

--- Creatures killed by rolling the chance once for every creature of the bearer's stack. At most
--- the share of the stack that could have rolled it dies, so a lucky roll cannot run away.
function Script:rolledKills(server, unit)
	local chance = self.val or 0

	if chance <= 0 then return 0 end

	local count = unit:getCount()
	local killed = server:rngBinomial(count, math.min(chance, 100) / 100)
	local cap = math.ceil(count * chance / 100)

	return math.min(killed, cap)
end

--- Commanders kill a fixed number instead of rolling, worth less against bigger creatures.
function Script:commanderKills(unit, other)
	local defenderLevel = other:getLevel()

	if defenderLevel <= 0 then return 0 end

	return math.floor(unit:getLevel() * (self.val or 0) / defenderLevel)
end

function Script:onAfterAttack(server, battle, unit, other, payload)
	if not other or not other:isAlive() then return end

	local situation = self.situation or "melee"
	local killed

	if situation == "commander" then
		killed = self:commanderKills(unit, other)
	elseif situation == situationOf(battle, unit, other, payload) then
		killed = self:rolledKills(server, unit)
	else
		return
	end

	if killed <= 0 then return end

	-- the spell is what filters out targets immune to the gaze, and what the client animates
	local spell = LIBRARY:getSpellByName(self.spell or SPELL)

	server:castSpell(battle, unit, spell, { other }, killed)
end

return Script
