local ReceiverBase = require("core:erm.ReceiverBase")

local HE = ReceiverBase:new()

function HE:new(ERM, p1, p2, p3)
	assert(p1 ~= nil, "!!HE requires hero identifier")

	if p2 and p3 then
		--by coordinates
		error("!!HEx/y/l: form is not implemented")
	else
		-- assume p1 is identifier
		local hero = GAME:getHeroWithSubid(p1)

		if not hero then
			logError("Hero with id ".. tostring(p1) .. " not found")
		end

		return ReceiverBase.new(self,
		{
			id=p1,
			ERM=ERM,
			hero=hero
		})
	end
end

function HE:A(x, ...)
	--artifacts
	logError("!!HE:A is not implemented")
end

HE:p1Dispatcher("B")

function HE:B0(x, ...)
	--name
	logError("!!HE:B is not implemented")
end

function HE:B1(x, ...)
	--bio
	logError("!!HE:B is not implemented")
end

function HE:B2(x, ...)
	--class
	logError("!!HE:B is not implemented")
end

function HE:B3(x, _)
	--get default bio
	logError("!!HE:B is not implemented")
end

function HE:C(x, ...)

	local argc = select("#", ...)

	if argc == 14 then
		return self:C14(x, ...)
	elseif argc == 4 then
		local N = select(1, ...)

		return nil, self["C"..tostring(N)](self, x, select(2, ...))
	else
		logError("!!HE:C extended form is not implemented")
	end
end

function HE:C0(x, slot, typ, count)
	--change creatures by slot

	if typ~=nil or count ~=nil then
		logError("!!HE:C0 set is not implemented")
		return
	end

	local stack = self.hero:getStack(slot)

	if not stack then
		return nil, -1, 0
	else
		return nil, stack:getType():getIndex(), stack:getCount()
	end

end

function HE:C1(x, ...)
	--change creatures by type
end

function HE:C05(x, ...)
	--change creatures by slot adv
end

function HE:C15(x, ...)
	--change creatures by type adv
end


function HE:C2(x, ...)
	--add creatures
end

function HE:C14(x, ...)
	-- change creatures with Query
end

function HE:D(x, ...)
	logError("!!HE:D not implemented")
end
function HE:E(x, ...)
	logError("!!HE:E is not implemented")
end
function HE:F(x, ...)
	logError("!!HE:F is not implemented")
end
function HE:G(x, ...)
	logError("!!HE:G is not implemented")
end
function HE:H(x, ...)
	logError("!!HE:H is not implemented")
end
function HE:I(x, ...)
	logError("!!HE:I is not implemented")
end
function HE:K(x, ...)
	logError("!!HE:K is not implemented")
end
function HE:L(x, ...)
	logError("!!HE:L is not implemented")
end
function HE:M(x, ...)
	logError("!!HE:M is not implemented")
end
function HE:N(x, ...)
	logError("!!HE:N is not implemented")
end

function HE:O(x, owner)
	if owner~=nil then
		logError("!!HE:O set is not implemented")
		return
	else
		if not self.hero then
			return 255
		end
		return self.hero:getOwner()
	end
end

function HE:P(x, ...)
	logError("!!HE:P is not implemented")
end
function HE:R(x, ...)
	logError("!!HE:R is not implemented")
end
function HE:S(x, ...)
	logError("!!HE:S is not implemented")
end
function HE:T(x, ...)
	logError("!!HE:T is not implemented")
end
function HE:U(x, ...)
	logError("!!HE:U is not implemented")
end
function HE:V(x, ...)
	logError("!!HE:V is not implemented")
end
function HE:W(x, ...)
	logError("!!HE:A is not implemented")
end
function HE:X(x, ...)
	logError("!!HE:X is not implemented")
end
function HE:Y(x, ...)
	logError("!!HE:Y is ot implemented")
end


return HE
