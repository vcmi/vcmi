local InfoWindow = require("netpacks.InfoWindow")

local ReceiverBase = require("core:erm.ReceiverBase")

local IF = ReceiverBase:new()

--[[
according to the ERM help:
"%%" -> "%"
"%F#" -> current value of # flag.
"%Vf"..."%Vt" -> current value of corresponding variable.
"%W1"..."%W100" -> current value of corresponding hero variable.
"%X1"..."%X16" -> current value of corresponding function parameter.
"%Y1"..."%Y100" -> current value of corresponding local variable.
"%Z1"..."%Z500" -> current value of corresponding string variable.
"%$macro$" -> macro name of corresponding variable
"%Dd" -> current day of week
"%Dw" -> current week
"%Dm" -> current month
"%Da" -> current day from beginning of the game
"%Gc" -> the color of current gamer in text
]]

function IF:M(x, message)
	local pack = InfoWindow.new()

	local _self = self

	local onMatch1 = function (key1, key2)

		if key1 == 'X' then
			return x[key2]
		end

		if key1 == 'V' then
			return _self.ERM.v[key2]
		end

		if key1 == 'Z' then
			return _self.ERM.z[key2]
		end

		if key1 == 'F' then
			return _self.ERM.flag[key2]
		end

		return nil
	end

	message = string.gsub(message, "%%([FVWXYZ])(%d+)", onMatch1)

	message = string.gsub(message, "(%%)(%%)", "%1")

	pack:addText(message)

	SERVER:commitPackage(pack)
end

return IF
