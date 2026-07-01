local Base = require("spellEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:canRemove(obstacle)
	local kind = obstacle:getObstacleType()
	if self.removeAbsolute  and kind == ENUM.ObstacleType.absolute     then return true end
	if self.removeUsual     and kind == ENUM.ObstacleType.usual        then return true end
	if self.removeAllSpells and kind == ENUM.ObstacleType.spellCreated then return true end
	if kind == ENUM.ObstacleType.spellCreated and self.removeSpells then
		local obstacleSpell = obstacle:getSpell()
		for _, s in ipairs(self.removeSpells) do
			if LIBRARY:getSpellByName(s) == obstacleSpell then return true end
		end
	end
	return false
end

function Script:getTargets(mechanics, target, alwaysMassive)
	local battle = mechanics:getBattle()
	local list = {}
	local function add(o)
		for _, existing in ipairs(list) do
			if existing == o then return end
		end
		table.insert(list, o)
	end
	if mechanics:isMassive() or alwaysMassive then
		for _, o in ipairs(battle:getAllObstacles()) do
			if self:canRemove(o) then add(o) end
		end
	else
		for _, dest in ipairs(target) do
			if dest.hex and dest.hex:isValid() then
				for _, o in ipairs(battle:getObstaclesOnPos(dest.hex, false)) do
					if self:canRemove(o) then add(o) end
				end
			end
		end
	end
	return list
end

function Script:applicableGeneral(mechanics, problem)
	if #self:getTargets(mechanics, {}, true) == 0 then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.noAppropriateTarget)
		return false
	end
	return true
end

function Script:applicableTarget(mechanics, problem, target)
	return #self:getTargets(mechanics, target, false) > 0
end

function Script:apply(mechanics, server, target)
	local battle = mechanics:getBattle()
	for _, obstacle in ipairs(self:getTargets(mechanics, target, false)) do
		server:removeObstacle(battle, obstacle)
	end
end

return Script
