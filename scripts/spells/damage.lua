local Base = require("spells/unitEffect")
local BattleLog = require("battleLog")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:isReceptive(mechanics, unit)
	local spell = mechanics:getSpell()
	if spell:isMagical() then
		if unit:getBonusesValue({type = "SPELL_DAMAGE_REDUCTION", subtype = "any"}) >= 100 then
			return false
		end
	end
	local reductions = unit:getBonuses({type = "SPELL_DAMAGE_REDUCTION"})
	for _, school in ipairs(spell:getSchools()) do
		-- the subtype of each names a school, and telling apart the school it names from the "any"
		-- that covers all of them is not something the filter can ask for
		local matching = reductions:filter(function(b)
			local sub = b:getSubtype()
			if sub == "any" then return false end
			return LIBRARY:getSpellSchoolByName(sub) == school
		end)
		local total = 0
		for i = 1, matching:size() do
			total = total + matching:getBonus(i):getVal()
		end
		if total >= 100 then
			return false
		end
	end
	return Base.isReceptive(self, mechanics, unit)
end

function Script:damageForTarget(targetIndex, mechanics, unit)
	local base
	if self.killByPercentage then
		local toKill = math.floor(unit:getCount() * mechanics:getEffectValue() / 100)
		base = toKill * unit:getMaxHealth()
	elseif self.killByCount then
		base = mechanics:getEffectValue() * unit:getMaxHealth()
	else
		base = mechanics:adjustEffectValue(unit)
	end
	local chainLength = self.chainLength or 0
	if chainLength > 1 and targetIndex > 0 then
		base = math.floor((self.chainFactor ^ targetIndex) * base)
	end
	return base
end

function Script:getHealthChange(mechanics, spellTarget)
	local result = { hpDelta = 0, unitsDelta = 0 }
	for i, dest in ipairs(spellTarget) do
		local unit = dest.unit
		if unit and unit:isAlive() then
			local amount = self:damageForTarget(i - 1, mechanics, unit)
			local copy = unit:copy()
			local hpBefore    = copy:getAvailableHealth()
			local countBefore = copy:getCount()
			copy:damage(amount)
			result.hpDelta    = result.hpDelta    - (hpBefore    - copy:getAvailableHealth())
			result.unitsDelta = result.unitsDelta - (countBefore - copy:getCount())
		end
	end
	return result
end

function Script:apply(mechanics, server, target)
	local battle   = mechanics:getBattle()
	local describe = server:describeChanges()
	local firstUnit, totalDamage, totalKilled, multiple = nil, 0, 0, false

	for i, dest in ipairs(target) do
		local unit = dest.unit
		if unit and unit:isAlive() then
			local amount = self:damageForTarget(i - 1, mechanics, unit)
			local dmg, killed = server:damageUnit(battle, unit, amount)
			if describe then
				if firstUnit then multiple = true else firstUnit = unit end
				totalDamage = totalDamage + dmg
				totalKilled = totalKilled + killed
			end
		end
	end

	if describe and firstUnit and totalDamage > 0 then
		self:describeEffect(server, battle, mechanics, firstUnit, totalKilled, totalDamage, multiple)
	end
end

function Script:describeEffect(server, battle, mechanics, firstUnit, kills, damage, multiple)
	local spell    = mechanics:getSpell()
	local spellKey = spell:getJsonKey()

	if spellKey:find("deathStare") and not multiple then
		local casterNameID = mechanics:getCasterNameTextID()
		if kills > 1 then
			server:appendLog(battle, {
				append         = { "core.genrltxt.119" },
				replaceStrings = { firstUnit:getCreature():getNameTextID(0), casterNameID },
				replaceNumbers = { kills }
			})
		else
			server:appendLog(battle, {
				append         = { "core.genrltxt.118" },
				replaceStrings = { firstUnit:getCreature():getNameTextID(1), casterNameID }
			})
		end

	elseif spellKey:find("accurateShot") and not multiple then
		local textID = mechanics:getPluralFormTextID(
			"vcmi.battleWindow.accurateShot.resultDescription", kills)
		server:appendLog(battle, {
			append         = { textID },
			replaceStrings = { firstUnit:getCreature():getNameTextID(kills) },
			replaceNumbers = { kills }
		})

	elseif spellKey:find("thunderbolt") and not multiple then
		server:appendLog(battle, {
			append         = { "core.genrltxt.367" },
			replaceStrings = { firstUnit:getCreature():getNameTextID(0) }
		})
		server:appendLog(battle, {
			append         = { "core.genrltxt.343" },
			replaceNumbers = { damage }
		})

	else
		-- an area spell kills creatures of several stacks, so the log names none of them
		BattleLog.spellDamage(server, battle, spell, not multiple and firstUnit or nil, damage, kills)
	end
end

return Script
