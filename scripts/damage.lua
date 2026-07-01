local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:sumBonusVal(unit, filterFn)
	local total = 0
	local list = unit:getBonuses(filterFn)
	for i = 1, list:size() do
		total = total + list:getBonus(i):getVal()
	end
	return total
end

function Script:isReceptive(mechanics, unit)
	local spell = mechanics:getSpell()
	if spell:isMagical() then
		if self:sumBonusVal(unit, function(b)
			return b:getType() == "SPELL_DAMAGE_REDUCTION" and b:getSubtype() == "any"
		end) >= 100 then
			return false
		end
	end
	for _, school in ipairs(spell:getSchools()) do
		if self:sumBonusVal(unit, function(b)
			local sub = b:getSubtype()
			if sub == "any" then return false end
			return b:getType() == "SPELL_DAMAGE_REDUCTION"
				and LIBRARY:getSpellSchoolByName(sub) == school
		end) >= 100 then
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
		server:appendLog(battle, {
			append         = { "core.genrltxt.376" },
			replaceStrings = { spell:getNameTextID() },
			replaceNumbers = { damage }
		})
		if kills > 0 then
			if kills > 1 then
				server:appendLog(battle, {
					append         = { "core.genrltxt.379" },
					replaceStrings = { multiple and "core.genrltxt.43"
					                            or firstUnit:getCreature():getNameTextID(0) },
					replaceNumbers = { kills }
				})
			else
				server:appendLog(battle, {
					append         = { "core.genrltxt.378" },
					replaceStrings = { multiple and "core.genrltxt.42"
					                            or firstUnit:getCreature():getNameTextID(1) }
				})
			end
		end
	end
end

return Script
