local BattleStackMoved = require("netpacks.BattleStackMoved")

apply = function(targets)
	local n = #targets;
	assert(n == 2, "2 destinations required ".. n .." provided")

	local unitId = targets[1][2]

	assert(type(unitId) == "number", "Invalid unit id")

	local dest = targets[2][1]

	assert(type(dest) == "number", "Invalid destination hex")

	local pack = BattleStackMoved.new()
	pack:setUnitId(unitId)
	pack:setDistance(0)
	pack:setTeleporting(true)
	pack:addTileToMove(dest)

	SERVER:moveUnit(pack)
end
