local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:deepCopyBonus(b)
	local copy = {}
	for k, v in pairs(b) do
		copy[k] = v
	end
	return copy
end

function Script:convertBonuses(mechanics)
	local duration = mechanics:getEffectDuration()
	local spellKey = mechanics:getSpell():getJsonKey()
	local converted = {}

	for name, b in pairs(self.bonus or {}) do
		local nb = self:deepCopyBonus(b)

		if not nb.turns or nb.turns == 0 then
			nb.turns = duration
		end

		nb.sourceType = "SPELL_EFFECT"
		nb.sourceID = spellKey

		converted[name] = nb
	end

	return converted
end

function Script:applyHeroSpecialty(mechanics, buffer, unit)
	local hero = mechanics:getHeroCaster()
	if not hero then return end

	local spellKey = mechanics:getSpell():getJsonKey()
	local tier = math.max(unit:creatureLevel(), 1)

	local peculiar = hero:getBonuses(function(b)
		return b:getType() == "SPECIAL_PECULIAR_ENCHANT" and b:getSubtype() == spellKey
	end)
	if peculiar:size() > 0 then
		local mode = peculiar:getBonus(1):getParametersAsNumber()
		local power = 0
		if mode == 0 then
			if tier <= 2 then power = 3
			elseif tier <= 4 then power = 2
			elseif tier <= 6 then power = 1
			end
		end
		if mechanics:isNegative() then power = -power end
		if power ~= 0 then
			for _, nb in pairs(buffer) do
				nb.val = (nb.val or 0) + power
			end
		end
	end

	local addVal = hero:getBonuses(function(b)
		return b:getType() == "SPECIAL_ADD_VALUE_ENCHANT" and b:getSubtype() == spellKey
	end)
	if addVal:size() > 0 then
		local addAmount = addVal:getBonus(1):getParametersAsNumber()
		for _, nb in pairs(buffer) do
			nb.val = (nb.val or 0) + addAmount
		end
	end

	local fixedVal = hero:getBonuses(function(b)
		return b:getType() == "SPECIAL_FIXED_VALUE_ENCHANT" and b:getSubtype() == spellKey
	end)
	if fixedVal:size() > 0 then
		local fixedAmount = fixedVal:getBonus(1):getParametersAsNumber()
		for _, nb in pairs(buffer) do
			nb.val = fixedAmount
		end
	end
end

function Script:describeEffect(server, battle, unit, bonuses)
	-- Age spell: STACK_HEALTH bonus with negative val gets a custom message
	for _, nb in pairs(bonuses) do
		if nb.type == "STACK_HEALTH" and (nb.val or 0) < 0 then
			local healthBonuses = unit:getBonuses(function(b)
				return b:getType() == "STACK_HEALTH"
			end)
			local oldHealth = 0
			for i = 1, healthBonuses:size() do
				oldHealth = oldHealth + healthBonuses:getBonus(i):getVal()
			end
			local lost = oldHealth - (oldHealth + nb.val)
			local ageTextID = unit:getCount() == 1
				and "core.genrltxt.551"
				or  "core.genrltxt.552"
			server:appendLog(battle, {
				append         = { ageTextID },
				replaceStrings = { unit:getCreature():getNameTextID(unit:getCount()) },
				replaceNumbers = { lost }
			})
			return
		end
	end

	if not self.battleLogPlural or self.battleLogPlural == "" then return end

	local textID = (self.battleLogSingular and self.battleLogSingular ~= "" and unit:getCount() == 1) and self.battleLogSingular or self.battleLogPlural
	local nameTextID = unit:getCreature():getNameTextID(unit:getCount())
	server:appendLog(battle, {
		append         = { textID },
		replaceStrings = { nameTextID }
	})
end

function Script:apply(mechanics, server, target)
	local battle   = mechanics:getBattle()
	local describe = server:describeChanges()
	local converted = self:convertBonuses(mechanics)

	for _, dest in ipairs(target) do
		local unit = dest.unit
		if not unit or not unit:isAlive() then goto continue end

		local buffer = {}
		for name, nb in pairs(converted) do
			buffer[name] = self:deepCopyBonus(nb)
		end

		self:applyHeroSpecialty(mechanics, buffer, unit)

		if describe then
			self:describeEffect(server, battle, unit, buffer)
		end

		for _, nb in pairs(buffer) do
			server:addUnitBonus(battle, unit, nb, self.cumulative or false)
		end

		::continue::
	end
end

return Script
