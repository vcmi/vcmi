/*
 * RmgArea.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace rmg
{
	static const std::array<int3, 4> dirs4 = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0) };
	static const std::array<int3, 4> dirsDiagonal= { int3(1,1,0),int3(1,-1,0),int3(-1,1,0),int3(-1,-1,0) };

	using Tileset = std::set<int3>;
	using DistanceMap = std::map<int3, int>;
	void toAbsolute(Tileset & tiles, const int3 & position);
	void toRelative(Tileset & tiles, const int3 & position);
	
	class DLL_LINKAGE Area
	{
	public:
		Area() = default;
		Area(const Area &);
		Area(const Area &&);
		Area(const Tileset & tiles);
		Area(const Tileset & relative, const int3 & position); //create from relative positions
		Area & operator= (const Area &);
		
		const Tileset & getTiles() const;
		const std::vector<int3> & getTilesVector() const;
		const Tileset & getBorder() const; //lazy cache invalidation
		const Tileset & getBorderOutside() const; //lazy cache invalidation
		
		DistanceMap computeDistanceMap(std::map<int, Tileset> & reverseDistanceMap) const;
		
		Area getSubarea(std::function<bool(const int3 &)> filter) const;
		
		bool connected() const; //is connected
		bool empty() const;
		bool contains(const int3 & tile) const;
		bool contains(const std::vector<int3> & tiles) const;
		bool contains(const Area & area) const;
		bool overlap(const Area & area) const;
		bool overlap(const std::vector<int3> & tiles) const;
		int distanceSqr(const int3 & tile) const;
		int distanceSqr(const Area & area) const;
		int3 nearest(const int3 & tile) const;
		int3 nearest(const Area & area) const;
		
		void clear();
		void assign(const Tileset tiles); //do not use reference to allow assigment of cached data
		void add(const int3 & tile);
		void erase(const int3 & tile);
		void unite(const Area & area);
		void intersect(const Area & area);
		void subtract(const Area & area);
		void translate(const int3 & shift);
		
		friend Area operator+ (const Area & l, const int3 & r); //translation
		friend Area operator- (const Area & l, const int3 & r); //translation
		friend Area operator+ (const Area & l, const Area & r); //union
		friend Area operator* (const Area & l, const Area & r); //intersection
		friend Area operator- (const Area & l, const Area & r); //AreaL reduced by tiles from AreaR
		friend bool operator== (const Area & l, const Area & r);
		friend std::list<Area> connectedAreas(const Area & area, bool disableDiagonalConnections);
		
	private:
		
		void invalidate();
		void computeBorderCache();
		void computeBorderOutsideCache();
		
		mutable Tileset dTiles;
		mutable std::vector<int3> dTilesVectorCache;
		mutable Tileset dBorderCache;
		mutable Tileset dBorderOutsideCache;
		mutable int3 dTotalShiftCache;
	};
}

VCMI_LIB_NAMESPACE_END
