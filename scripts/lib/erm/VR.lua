local ReceiverBase = require("core:erm.ReceiverBase")

local VR = ReceiverBase:new()

function VR:new(ERM, v)
	assert(v ~= nil, "!!VR requires variable identifier")
	return ReceiverBase.new(self, {v=v, ERM=ERM})
end

local match = string.match

local function trim(s)
   return match(s,'^()%s*$') and '' or match(s,'^%s*(.*%S)')
end

function VR:H(flagIndex)
	local v = trim(self.v)
	self.ERM.F[flagIndex] = v ~= ''
end

function VR:U(subString)
	self.ERM.F['1'] = string.find(self.v, subString) > 0
end

function VR:M1(startIndex, length)
	return string.sub(self.v, startIndex - 1, startIndex - 1 + length)
end

function VR:M2(wordIndex)
	local words = string.gmatch(self.v, "[^%s]+")
	local i = 0

	for w in words do
		if i == wordIndex then
			return w
		end
		i = i + 1
	end
end

function VR:M3(val, radix)
	radix = radix or 10
	
	if(type(val) ~= "number") then
		error("The first parameter should be of numeric type")
	end
	
	if(type(radix) ~= "number") then
		error("The second parameter should be of numeric type. Default value is 10.")
	end
	
	if radix == 10 then
		return tostring(val)
	elseif radix == 16 then
		return string.format("%x", val)
	else
		error("The second parameter value is invalid. Only 10 and 16 radix are supported for now.")
	end
end

function VR:M4()
	return string.len(self.v)
end

function VR:M5()
	local firstPos = string.find(str, "[^%s]+")
	
	return firstPos
end

function VR:M6()
	local lastPos = 1 + string.len(self.v) - string.find(string.reverse(self.v), "[^%s]+")
	
	return lastPos
end


return VR
