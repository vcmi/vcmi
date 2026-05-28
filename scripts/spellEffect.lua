local Script = {}
Script.__index = Script
Script.type = "spellEffect"

-- TODO
-- initializes parameters of the script using spell effect json
-- returns converted parameters that contain resolved identifiers
function Script:initialize()
	return self
end

--- Returns true if specified target can be affected by the spell
--- if target can not be affected, script needs to call `problem:add`
--- to explain the reason to the player
function Script:applicableTarget(mechanics, problem, target)
    return true
end

--- Returns true if spell can be casted in general
--- if no valid targets exist, script needs to call `problem:add`
--- to explain the reason to the player
function Script:applicableGeneral(mechanics, problem)
	return true
end

--- Actually casts the spells and applies all changes caused by spell
--- use `server` parameter to apply changes on specified target(s)
function Script:apply(mechanics, server, target)
	return
end

--- converts specified range into actual list of affected targets
--- for example, area damage spells should locate all units on affected hexes
--- and return list of affected units
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	return spellTarget
end

--- TODO
function Script:getHealthChange(mechanics, problem, target)
    return true
end

--- TODO
function Script:adjustAffectedHexes(mechanics, problem, target)
    return true
end

--- TODO
function Script:adjustTargetTypes(mechanics, problem, target)
    return true
end

return Script
