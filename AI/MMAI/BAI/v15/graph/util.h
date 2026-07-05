#pragma once

namespace MMAI::BAI::V15::Graph
{
inline int permille(int v, int max)
{
	// Multiplying by 1000 might cause int32 overflow
	// => use temp long
	return static_cast<int>((1000LL * v) / max);
}

inline int permille(double v, int max)
{
	return static_cast<int>((1000 * v) / max);
}
}
