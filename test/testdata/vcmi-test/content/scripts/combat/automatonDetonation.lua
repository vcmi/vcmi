local Base = require("combat/combatScript")
local BattleLog = require("battleLog")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:isEligible(unit, target)
	return unit:unitID() ~= target:unitID() and target:isValidTarget(false) and not target:isInvincible()
end

--- Returns all affected units
function Script:getAffectedUnits(battle, unit)
	local affectedUnits = {}
	local seenUnits = {}
	local hexes = unit:getSurroundingHexes()

	for i = 1, hexes:size() do
		local hex = hexes:at(i)
		local targetUnit = battle:getUnitByPos(hex, true)
		if targetUnit then
			local id = targetUnit:unitID()

			if not seenUnits[id] then
				seenUnits[id] = true

				if self:isEligible(unit, targetUnit) then
					table.insert(affectedUnits, targetUnit)
				end
			end
		end
	end

	return affectedUnits
end

--- Calculate the damage the explosion should do to each adjacent unit
function Script:getExplosionDamage(unit, killed)
	local baseDamage = 90 + 5 * killed
	local specialtyPercent = unit:getBonusesValue({ type = "AUTOMATON_EXPLOSION_DAMAGE" })

	baseDamage = math.ceil((baseDamage * (100 + specialtyPercent)) / 100)
	return baseDamage > 0 and baseDamage or 1
end

--- Plays the detonation death animation of 'unit' and damages all surrounding targets.
--- A clone detonates like anything else - what it leaves behind is not what set off the charge.
function Script:onDeath(server, battle, unit, other, payload)
	local entry = self:ownEntry(unit, payload)

	-- a death always carries the entry of the unit it is announced to, and one that killed nobody
	-- is never announced at all
	if not entry then return end

	local animation = unit:getCreature():getJsonKey() == "hota.factory:sentinelAutomaton" and "hota/factory/spells/detonationSentinel" or "hota/factory/spells/detonationAutomaton"
	server:showBattleAnimation(battle, { { unit = unit } }, animation, "hota/factory/creatures/automaton/AUTOSPEC", 1.0, true)

	local targets = self:getAffectedUnits(battle, unit)

	if #targets == 0 then return end

	local explosionDamage = self:getExplosionDamage(unit, entry.killed)
	local totalDamage, totalKilled = 0, 0

	for _, target in ipairs(targets) do
		-- per-target, since a cap on one of them must not follow the blast to the next
		local damage = explosionDamage
		local cap = target:getBonusesValue({ type = "DAMAGE_RECEIVED_CAP" })
		if cap > 0 then
			damage = math.max(math.floor(target:getMaxHealth() * cap / 100), 1)
		end
		local dealt, killed = server:damageUnit(battle, target, damage)
		totalDamage = totalDamage + dealt
		totalKilled = totalKilled + killed
	end

	local victim = #targets == 1 and targets[1] or nil
	local spell = LIBRARY:getSpellByName("abilityDetonation")
	BattleLog.spellDamage(server, battle, spell, victim, totalDamage, totalKilled)
end

return Script
