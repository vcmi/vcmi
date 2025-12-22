/*
 * encoder_test.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v13/encoder.h"
#include "schema/v13/constants.h"
#include "schema/v13/types.h"
#include "test/googletest/googletest/include/gtest/gtest.h"
#include <stdexcept>

using Encoder = MMAI::BAI::V13::Encoder;
namespace SV = Schema::V13;

TEST(Encoder, Encode)
{
	{
		constexpr auto a = SV::HexAttribute::Y_COORD;
		constexpr auto e = std::get<1>(SV::HEX_ENCODING.at(int(a)));
		constexpr auto n = std::get<2>(SV::HEX_ENCODING.at(int(a)));
		static_assert(e == SV::Encoding::CATEGORICAL_STRICT_NULL, "test needs to be updated");
		static_assert(n == 11, "test needs to be updated");

		{
			auto have = std::vector<float>{};
			auto want = std::vector<float>{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			Encoder::Encode(a, 0, have);
			ASSERT_EQ(want, have);
		}
		{
			auto have = std::vector<float>{};
			auto want = std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
			Encoder::Encode(a, 9, have);
			ASSERT_EQ(want, have);
		}
		{
			auto have = std::vector<float>{};
			ASSERT_THROW(Encoder::Encode(a, -1, have), std::runtime_error);
		}

		// This no longer throws (emits warning instead)
		// {
		//     auto have = std::vector<float> {};
		//     ASSERT_THROW(Encoder::Encode(a, 666, have), std::runtime_error);
		// }
	}
}

TEST(Encoder, AccumulatingExplicitNull)
{
	static_assert(EI(SV::Encoding::ACCUMULATING_EXPLICIT_NULL) == 0, "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 1, 1, 1, 1};
		Encoder::EncodeAccumulatingExplicitNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 1, 0, 0, 0};
		Encoder::EncodeAccumulatingExplicitNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingExplicitNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, AccumulatingImplicitNull)
{
	static_assert(EI(SV::Encoding::ACCUMULATING_IMPLICIT_NULL) == 1 + EI(SV::Encoding::ACCUMULATING_EXPLICIT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 1, 1, 0};
		Encoder::EncodeAccumulatingImplicitNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingImplicitNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingImplicitNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, AccumulatingMaskingNull)
{
	static_assert(EI(SV::Encoding::ACCUMULATING_MASKING_NULL) == 1 + EI(SV::Encoding::ACCUMULATING_IMPLICIT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 1, 1, 0};
		Encoder::EncodeAccumulatingMaskingNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingMaskingNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{-1, -1, -1, -1, -1};
		Encoder::EncodeAccumulatingMaskingNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, AccumulatingStrictNull)
{
	static_assert(EI(SV::Encoding::ACCUMULATING_STRICT_NULL) == 1 + EI(SV::Encoding::ACCUMULATING_MASKING_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 1, 1, 0};
		Encoder::EncodeAccumulatingStrictNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingStrictNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		ASSERT_THROW(Encoder::EncodeAccumulatingStrictNull(-1, 5, have), std::runtime_error);
	}
}

TEST(Encoder, AccumulatingZeroNull)
{
	static_assert(EI(SV::Encoding::ACCUMULATING_ZERO_NULL) == 1 + EI(SV::Encoding::ACCUMULATING_STRICT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 1, 1, 0};
		Encoder::EncodeAccumulatingZeroNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingZeroNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeAccumulatingZeroNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, BinaryExplicitNull)
{
	static_assert(EI(SV::Encoding::BINARY_EXPLICIT_NULL) == 1 + EI(SV::Encoding::ACCUMULATING_ZERO_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 1, 1, 0, 0};
		Encoder::EncodeBinaryExplicitNull(0b11, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeBinaryExplicitNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeBinaryExplicitNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, BinaryMaskingNull)
{
	static_assert(EI(SV::Encoding::BINARY_MASKING_NULL) == 1 + EI(SV::Encoding::BINARY_EXPLICIT_NULL), "Encoding list has changed");

	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 0, 0, 0};
		Encoder::EncodeBinaryMaskingNull(0b11, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeBinaryMaskingNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{-1, -1, -1, -1, -1};
		Encoder::EncodeBinaryMaskingNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, BinaryStrictNull)
{
	static_assert(EI(SV::Encoding::BINARY_STRICT_NULL) == 1 + EI(SV::Encoding::BINARY_MASKING_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 0, 0, 0};
		Encoder::EncodeBinaryStrictNull(0b11, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeBinaryStrictNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		ASSERT_THROW(Encoder::EncodeBinaryStrictNull(-1, 5, have), std::runtime_error);
	}
}

TEST(Encoder, BinaryZeroNull)
{
	static_assert(EI(SV::Encoding::BINARY_ZERO_NULL) == 1 + EI(SV::Encoding::BINARY_STRICT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 1, 0, 0, 0};
		Encoder::EncodeBinaryZeroNull(0b11, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeBinaryZeroNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeBinaryZeroNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, CategoricalExplicitNull)
{
	static_assert(EI(SV::Encoding::CATEGORICAL_EXPLICIT_NULL) == 1 + EI(SV::Encoding::BINARY_ZERO_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 1};
		Encoder::EncodeCategoricalExplicitNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 1, 0, 0, 0};
		Encoder::EncodeCategoricalExplicitNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeCategoricalExplicitNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, CategoricalImplicitNull)
{
	static_assert(EI(SV::Encoding::CATEGORICAL_IMPLICIT_NULL) == 1 + EI(SV::Encoding::CATEGORICAL_EXPLICIT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 1, 0};
		Encoder::EncodeCategoricalImplicitNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeCategoricalImplicitNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 0, 0};
		Encoder::EncodeCategoricalImplicitNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, CategoricalMaskingNull)
{
	static_assert(EI(SV::Encoding::CATEGORICAL_MASKING_NULL) == 1 + EI(SV::Encoding::CATEGORICAL_IMPLICIT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 1, 0};
		Encoder::EncodeCategoricalMaskingNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeCategoricalMaskingNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{-1, -1, -1, -1, -1};
		Encoder::EncodeCategoricalMaskingNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, CategoricalStrictNull)
{
	static_assert(EI(SV::Encoding::CATEGORICAL_STRICT_NULL) == 1 + EI(SV::Encoding::CATEGORICAL_MASKING_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 1, 0};
		Encoder::EncodeCategoricalStrictNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeCategoricalStrictNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		ASSERT_THROW(Encoder::EncodeCategoricalStrictNull(-1, 5, have), std::runtime_error);
	}
}

TEST(Encoder, CategoricalZeroNull)
{
	static_assert(EI(SV::Encoding::CATEGORICAL_ZERO_NULL) == 1 + EI(SV::Encoding::CATEGORICAL_STRICT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0, 0, 1, 0};
		Encoder::EncodeCategoricalZeroNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeCategoricalZeroNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0, 0, 0, 0};
		Encoder::EncodeCategoricalZeroNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, NormalizedExpExplicitNull)
{
	static_assert(EI(SV::Encoding::EXPNORM_EXPLICIT_NULL) == 1 + EI(SV::Encoding::CATEGORICAL_ZERO_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0.876};
		Encoder::EncodeExpnormExplicitNull(3, 5, 4, have);
		ASSERT_EQ(have.size(), 2);
		ASSERT_EQ(want.at(0), have.at(0));
		ASSERT_NEAR(want.at(1), have.at(1), 1e-3);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0};
		Encoder::EncodeExpnormExplicitNull(0, 5, 4, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0};
		Encoder::EncodeExpnormExplicitNull(-1, 5, 4, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, NormalizedExpMaskingNull)
{
	static_assert(EI(SV::Encoding::EXPNORM_MASKING_NULL) == 1 + EI(SV::Encoding::EXPNORM_EXPLICIT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0.876};
		Encoder::EncodeExpnormMaskingNull(3, 5, 4, have);
		ASSERT_EQ(have.size(), 1);
		ASSERT_NEAR(want.at(0), have.at(0), 1e-3);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeExpnormMaskingNull(0, 5, 4, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{-1};
		Encoder::EncodeExpnormMaskingNull(-1, 5, 4, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, NormalizedExpStrictNull)
{
	static_assert(EI(SV::Encoding::EXPNORM_STRICT_NULL) == 1 + EI(SV::Encoding::EXPNORM_MASKING_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0.876};
		Encoder::EncodeExpnormStrictNull(3, 5, 4, have);
		ASSERT_EQ(have.size(), 1);
		ASSERT_NEAR(want.at(0), have.at(0), 1e-3);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeExpnormStrictNull(0, 5, 4, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		ASSERT_THROW(Encoder::EncodeExpnormStrictNull(-1, 5, 4, have), std::runtime_error);
	}
}

TEST(Encoder, NormalizedExpZeroNull)
{
	static_assert(EI(SV::Encoding::EXPNORM_ZERO_NULL) == 1 + EI(SV::Encoding::EXPNORM_STRICT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0.876};
		Encoder::EncodeExpnormZeroNull(3, 5, 4, have);
		ASSERT_EQ(have.size(), 1);
		ASSERT_NEAR(want.at(0), have.at(0), 1e-3);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeExpnormZeroNull(0, 5, 4, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeExpnormZeroNull(-1, 5, 4, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, NormalizedLinExplicitNull)
{
	static_assert(EI(SV::Encoding::LINNORM_EXPLICIT_NULL) == 1 + EI(SV::Encoding::EXPNORM_ZERO_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0.6};
		Encoder::EncodeLinnormExplicitNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0, 0};
		Encoder::EncodeLinnormExplicitNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{1, 0};
		Encoder::EncodeLinnormExplicitNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, NormalizedMaskingNull)
{
	static_assert(EI(SV::Encoding::LINNORM_MASKING_NULL) == 1 + EI(SV::Encoding::LINNORM_EXPLICIT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0.6};
		Encoder::EncodeLinnormMaskingNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeLinnormMaskingNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{-1};
		Encoder::EncodeLinnormMaskingNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}

TEST(Encoder, NormalizedStrictNull)
{
	static_assert(EI(SV::Encoding::LINNORM_STRICT_NULL) == 1 + EI(SV::Encoding::LINNORM_MASKING_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0.6};
		Encoder::EncodeLinnormStrictNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeLinnormStrictNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		ASSERT_THROW(Encoder::EncodeLinnormStrictNull(-1, 5, have), std::runtime_error);
	}
}

TEST(Encoder, NormalizedZeroNull)
{
	static_assert(EI(SV::Encoding::LINNORM_ZERO_NULL) == 1 + EI(SV::Encoding::LINNORM_STRICT_NULL), "Encoding list has changed");
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0.6};
		Encoder::EncodeLinnormZeroNull(3, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeLinnormZeroNull(0, 5, have);
		ASSERT_EQ(want, have);
	}
	{
		auto have = std::vector<float>{};
		auto want = std::vector<float>{0};
		Encoder::EncodeLinnormZeroNull(-1, 5, have);
		ASSERT_EQ(want, have);
	}
}
