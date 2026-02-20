/*
 * GeometryAlgorithm.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class GeometryAlgorithm
{
public:
	struct Node
	{
		double x;
		double y;
		double dx = 0;
		double dy = 0;
		int id;
	};

	struct Edge
	{
		int from;
		int to;
	};

	static double distance(double x1, double y1, double x2, double y2);
	static bool edgesIntersect(const Node& a, const Node& b, const Node& c, const Node& d);
	static void forceDirectedLayout(std::vector<Node>& nodes, const std::vector<Edge>& edges, int iterations, double width, double height);
};
