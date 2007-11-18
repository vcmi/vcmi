-------------------------
--
-------------------------
function find(t, searched)
	print("searching")
	for index,value in ipairs(t) 
	do 
		print(index.." "..value)
		if value==searched
		then
			return index
		end
	end
	return nil
end

function contain(t, searched)
	print("searching")
	for index,value in ipairs(t) 
	do 
		print(index.." "..value)
		if value==searched
		then
			return true
		end
	end
	return false
end

function push_back(t,value)
	t[(#t)+1] = value
end

myObjects = {}
name = ""
visited = {}
--------------------------------------------------------------
function newObject_23 (ob)
	myObjects[ob] = {}
	print("Dostaje info o nowym Marletto Tower spod adresu " .. ob)
	print(vcmi.getPos(ob))
	visited[true],visited[false] = vcmi.getGnrlText(352), vcmi.getGnrlText(353)
end

function hoverText_23(Object)
	local Hero = vcmi.getSelectedHero()
	local ret = name.." "
	if Hero>-1
	then
		ret = ret..visited[contain(myObjects[ObjectAddress],Hero)]
	end
	return name..visited[contain(myObjects[ObjectAddress],Hero)]
	
end

function heroVisit_23(ObjectAddress, HeroID)
	print("Hero with ID " .. HeroID .. " has visited object at " .. ObjectAddress)
	if find(myObjects[ObjectAddress],HeroID)
	then
		print("Ten bohater juz tu byl")
	else
		print("Bierz obrone...")
		vcmi.changePrimSkill(HeroID, 1, 1)
		push_back(myObjects[ObjectAddress],HeroID)
	end
end
