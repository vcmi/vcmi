local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Arrow Towers have custom logic for determining their base damage
function Script:getBaseDamageSingle(info)
	if not info.attacker:isTurret() then return Base.getBaseDamageSingle(self, info) end

	local turretDamage = info.battle:getTurretDamageRange(info.attacker)

	-- no town to ask means no siege, which should not happen to a tower - its own damage will do
	if #turretDamage ~= 2 then return Base.getBaseDamageSingle(self, info) end

	return turretDamage[1], turretDamage[2]
end

return Script
