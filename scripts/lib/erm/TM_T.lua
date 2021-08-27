local TriggerBase = require("core:erm.TriggerBase")
local PlayerGotTurn = require("events.PlayerGotTurn")
local bit = bit
local band, bor = bit.band, bit.bor
local lshift, rshift = bit.lshift, bit.rshift

local TM_T = TriggerBase:new()

function TM_T:new(o)
	o = TriggerBase.new(self, o)
	o.sub = PlayerGotTurn.subscribeAfter(EVENT_BUS, function(event)
		o:update(event)
	end)
	o.timerId = tostring(o.id[1])
	return o
end

function TM_T:update(event)
	local timerData = DATA.ERM.timers[self.timerId]
	if not timerData then
		error("Timer" ..self.timerId .." is not set")
	end

	local today = self.ERM.Q.c -- this assumes that ERM updates before this
	local activePlayer = event:getPlayer()

	assert(type(activePlayer) == "number")

	--TODO: special player constants

	if activePlayer < 0 or activePlayer > 8 then
		return
	end

	if band(timerData.players, lshift(1, activePlayer)) == 0 then
		return
	end

	if (today < timerData.dayFirst) or ((today > timerData.dayLast) and (timerData.dayLast ~= 0)) then
		return
	end
--
	local d = today - timerData.dayFirst
--
	if d % timerData.interval == 0 then
		self:call(event)
	end

end

return TM_T
