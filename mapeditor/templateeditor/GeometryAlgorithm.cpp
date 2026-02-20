/*
 * GeometryAlgorithm.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GeometryAlgorithm.h"

double GeometryAlgorithm::distance(double x1, double y1, double x2, double y2)
{
	return std::sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)) + 1e-9;
}

bool GeometryAlgorithm::edgesIntersect(const Node& a, const Node& b, const Node& c, const Node& d)
{
	auto cross = [](double x1, double y1, double x2, double y2) {
		return x1 * y2 - y1 * x2;
	};

	double dx1 = b.x - a.x;
	double dy1 = b.y - a.y;
	double dx2 = d.x - c.x;
	double dy2 = d.y - c.y;

	double delta = cross(dx1, dy1, dx2, dy2);
	if (std::abs(delta) < 1e-10) return false; // Parallel

	// Compute intersection
	double s = cross(c.x - a.x, c.y - a.y, dx2, dy2) / delta;
	double t = cross(c.x - a.x, c.y - a.y, dx1, dy1) / delta;

	return s > 0 && s < 1 && t > 0 && t < 1;
}

void GeometryAlgorithm::forceDirectedLayout(std::vector<Node> & nodes, const std::vector<Edge> & edges, int iterations, double width, double height)
{
	const double area = width * height;
	const double k = std::sqrt(area / nodes.size());

	for (int it = 0; it < iterations; ++it)
	{
		// Reset forces
		for (auto& node : nodes)
			node.dx = node.dy = 0;

		// Repulsive forces
		for (size_t i = 0; i < nodes.size(); ++i)
		{
			for (size_t j = i + 1; j < nodes.size(); ++j)
			{
				double dx = nodes[i].x - nodes[j].x;
				double dy = nodes[i].y - nodes[j].y;
				double dist = distance(nodes[i].x, nodes[i].y, nodes[j].x, nodes[j].y);
				double force = (k * k) / dist;

				nodes[i].dx += (dx / dist) * force;
				nodes[i].dy += (dy / dist) * force;
				nodes[j].dx -= (dx / dist) * force;
				nodes[j].dy -= (dy / dist) * force;
			}
		}

		// Attractive forces
		for (const auto& edge : edges)
		{
			Node& u = nodes[edge.from];
			Node& v = nodes[edge.to];
			double dx = u.x - v.x;
			double dy = u.y - v.y;
			double dist = distance(u.x, u.y, v.x, v.y);
			double force = (dist * dist) / k;

			double fx = (dx / dist) * force;
			double fy = (dy / dist) * force;

			u.dx -= fx;
			u.dy -= fy;
			v.dx += fx;
			v.dy += fy;
		}

		// Edge crossing penalty
		for (size_t i = 0; i < edges.size(); ++i) {
			for (size_t j = i + 1; j < edges.size(); ++j) {
				const Edge& e1 = edges[i];
				const Edge& e2 = edges[j];

				if (e1.from == e2.from || e1.from == e2.to ||
				   e1.to == e2.from || e1.to == e2.to)
					continue; // Skip if they share nodes

				Node& a = nodes[e1.from];
				Node& b = nodes[e1.to];
				Node& c = nodes[e2.from];
				Node& d = nodes[e2.to];

				if (edgesIntersect(a, b, c, d)) {
					double strength = 0.05;

					a.dx += strength * (a.x - c.x);
					a.dy += strength * (a.y - c.y);
					b.dx += strength * (b.x - d.x);
					b.dy += strength * (b.y - d.y);
					c.dx += strength * (c.x - a.x);
					c.dy += strength * (c.y - a.y);
					d.dx += strength * (d.x - b.x);
					d.dy += strength * (d.y - b.y);
				}
			}
		}

		// Apply displacement
		for (auto& node : nodes)
		{
			node.x += std::max(-5.0, std::min(5.0, node.dx));
			node.y += std::max(-5.0, std::min(5.0, node.dy));

			// Keep within bounds
			node.x = std::min(width, std::max(0.0, node.x));
			node.y = std::min(height, std::max(0.0, node.y));
		}
	}

	for (auto& node : nodes)
	{
		// Center around 0
		node.x -= width / 2;
		node.y -= height / 2;
	}
}
