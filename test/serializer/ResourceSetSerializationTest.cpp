/*
 * ResourceSetSerializationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/ResourceSet.h"
#include "../../lib/serializer/CMemorySerializer.h"

/// Saves made before SPARSE_RESOURCE_SET store amount of every resource, including resources with no amount
TEST(ResourceSetSerializationTest, oldFormatIsConverted)
{
	CMemorySerializer mem;
	std::vector<TResource> amounts = {5, 0, 0, 0, 0, 2, 1000};
	mem.oser & amounts;
	mem.iser.version = static_cast<ESerializationVersion>(static_cast<int>(ESerializationVersion::SPARSE_RESOURCE_SET) - 1);

	ResourceSet loaded;
	mem.iser & loaded;

	ResourceSet expected;
	expected[EGameResID::WOOD] = 5;
	expected[EGameResID::GEMS] = 2;
	expected[EGameResID::GOLD] = 1000;
	EXPECT_EQ(loaded, expected);
	EXPECT_EQ(std::distance(loaded.begin(), loaded.end()), 3);
	EXPECT_TRUE(mem.isFullyRead());
}
