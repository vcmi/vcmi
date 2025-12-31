/*
 * encoder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "schema/v13/constants.h"
#include "schema/v13/types.h"

#include "BAI/v13/encoder.h"
#include "common.h"

namespace MMAI::BAI::V13
{

namespace S13 = Schema::V13;
using Encoding = Schema::V13::Encoding;
using BS = Schema::BattlefieldState;
using clock = std::chrono::system_clock;

#define ADD_ZEROS_AND_RETURN(n, out) \
	out.insert((out).end(), n, 0);   \
	return

#define MAYBE_ADD_ZEROS_AND_RETURN(v, n, out) \
	if((v) <= 0)                              \
	{                                         \
		ADD_ZEROS_AND_RETURN(n, out);         \
	}

#define MAYBE_ADD_MASKED_AND_RETURN(v, n, out)                 \
	if((v) == S13::NULL_VALUE_UNENCODED)                       \
	{                                                          \
		(out).insert((out).end(), n, S13::NULL_VALUE_ENCODED); \
		return;                                                \
	}

#define MAYBE_THROW_STRICT_ERROR(v)      \
	if((v) == S13::NULL_VALUE_UNENCODED) \
	throw std::runtime_error("NULL values are not allowed for strict encoding")

void Encoder::Encode(const EncoderInput & in, BS & out)
{
	if(in.e == Encoding::RAW)
	{
		out.push_back(in.v);
		return;
	}

	auto v = in.v;

	if(in.v > in.vmax)
	{
		// THROW_FORMAT("Cannot encode value: %d (vmax=%d, a=%d, n=%d, e=%d)", v % vmax % EI(a) % n % EI(e));
		// Can happen (e.g. DMG_*_ACC_REL0 > 1 if there were resurrected stacks)

		// Warn at most once every 600s
		auto now = clock::now();
		static thread_local std::map<std::string, std::map<int, clock::time_point>> warns;
		auto & warned_at = warns[std::string(in.attrname)][EI(in.a)];

		if(std::chrono::duration_cast<std::chrono::seconds>(now - warned_at) > std::chrono::seconds(600))
		{
			// This is not critical; the value will be capped to vmax (should not occur often)
			logAi->info(
				"MMAI: Attribute value out of bounds: v=%d (vmax=%d, a=%d, e=%d, n=%d, attrname=%s)\n", in.v, in.vmax, EI(in.a), EI(in.e), in.n, in.attrname
			);
			warns[std::string(in.attrname)][EI(in.a)] = now;
		}
		v = in.vmax;
	}

	switch(in.e)
	{
		case Encoding::BINARY_EXPLICIT_NULL:
			EncodeBinaryExplicitNull(v, in.n, out);
			break;
		case Encoding::BINARY_MASKING_NULL:
			EncodeBinaryMaskingNull(v, in.n, out);
			break;
		case Encoding::BINARY_STRICT_NULL:
			EncodeBinaryStrictNull(v, in.n, out);
			break;
		case Encoding::BINARY_ZERO_NULL:
			EncodeBinaryZeroNull(v, in.n, out);
			break;
		case Encoding::EXPNORM_EXPLICIT_NULL:
			EncodeExpnormExplicitNull(v, in.vmax, in.p, out);
			break;
		case Encoding::EXPNORM_MASKING_NULL:
			EncodeExpnormMaskingNull(v, in.vmax, in.p, out);
			break;
		case Encoding::EXPNORM_STRICT_NULL:
			EncodeExpnormStrictNull(v, in.vmax, in.p, out);
			break;
		case Encoding::EXPNORM_ZERO_NULL:
			EncodeExpnormZeroNull(v, in.vmax, in.p, out);
			break;
		case Encoding::LINNORM_EXPLICIT_NULL:
			EncodeLinnormExplicitNull(v, in.vmax, out);
			break;
		case Encoding::LINNORM_MASKING_NULL:
			EncodeLinnormMaskingNull(v, in.vmax, out);
			break;
		case Encoding::LINNORM_STRICT_NULL:
			EncodeLinnormStrictNull(v, in.vmax, out);
			break;
		case Encoding::LINNORM_ZERO_NULL:
			EncodeLinnormZeroNull(v, in.vmax, out);
			break;
		case Encoding::CATEGORICAL_EXPLICIT_NULL:
			EncodeCategoricalExplicitNull(v, in.n, out);
			break;
		case Encoding::CATEGORICAL_IMPLICIT_NULL:
			EncodeCategoricalImplicitNull(v, in.n, out);
			break;
		case Encoding::CATEGORICAL_MASKING_NULL:
			EncodeCategoricalMaskingNull(v, in.n, out);
			break;
		case Encoding::CATEGORICAL_STRICT_NULL:
			EncodeCategoricalStrictNull(v, in.n, out);
			break;
		case Encoding::CATEGORICAL_ZERO_NULL:
			EncodeCategoricalZeroNull(v, in.n, out);
			break;
		case Encoding::ACCUMULATING_EXPLICIT_NULL:
			EncodeAccumulatingExplicitNull(v, in.n, out);
			break;
		case Encoding::ACCUMULATING_IMPLICIT_NULL:
			EncodeAccumulatingImplicitNull(v, in.n, out);
			break;
		case Encoding::ACCUMULATING_MASKING_NULL:
			EncodeAccumulatingMaskingNull(v, in.n, out);
			break;
		case Encoding::ACCUMULATING_STRICT_NULL:
			EncodeAccumulatingStrictNull(v, in.n, out);
			break;
		case Encoding::ACCUMULATING_ZERO_NULL:
			EncodeAccumulatingZeroNull(v, in.n, out);
			break;
		default:
			THROW_FORMAT("Unexpected Encoding: %d", EI(in.e));
	}
}

void Encoder::Encode(const S13::HexAttribute a, int v, BS & out)
{
	const auto & [_, e, n, vmax, p] = S13::HEX_ENCODING.at(EI(a));
	Encode(EncoderInput{.attrname = "HexAttribute", .a = EI(a), .e = e, .n = n, .vmax = vmax, .p = p, .v = v}, out);
}

void Encoder::Encode(const S13::PlayerAttribute a, int v, BS & out)
{
	const auto & [_, e, n, vmax, p] = S13::PLAYER_ENCODING.at(EI(a));
	Encode(EncoderInput{.attrname = "PlayerAttribute", .a = EI(a), .e = e, .n = n, .vmax = vmax, .p = p, .v = v}, out);
}

void Encoder::Encode(const S13::GlobalAttribute a, int v, BS & out)
{
	const auto & [_, e, n, vmax, p] = S13::GLOBAL_ENCODING.at(EI(a));
	Encode(EncoderInput{.attrname = "GlobalAttribute", .a = EI(a), .e = e, .n = n, .vmax = vmax, .p = p, .v = v}, out);
}

//
// ACCUMULATING
//
void Encoder::EncodeAccumulatingExplicitNull(int v, int n, BS & out)
{
	if(v == S13::NULL_VALUE_UNENCODED)
	{
		out.push_back(1);
		ADD_ZEROS_AND_RETURN(n - 1, out);
	}
	out.push_back(0);
	EncodeAccumulating(v, n - 1, out);
}

void Encoder::EncodeAccumulatingImplicitNull(int v, int n, BS & out)
{
	if(v == S13::NULL_VALUE_UNENCODED)
	{
		ADD_ZEROS_AND_RETURN(n, out);
	}
	EncodeAccumulating(v, n, out);
}

void Encoder::EncodeAccumulatingMaskingNull(int v, int n, BS & out)
{
	MAYBE_ADD_MASKED_AND_RETURN(v, n, out);
	EncodeAccumulating(v, n, out);
}

void Encoder::EncodeAccumulatingStrictNull(int v, int n, BS & out)
{
	MAYBE_THROW_STRICT_ERROR(v);
	EncodeAccumulating(v, n, out);
}

void Encoder::EncodeAccumulatingZeroNull(int v, int n, BS & out)
{
	if(v <= 0)
	{
		out.push_back(1);
		ADD_ZEROS_AND_RETURN(n - 1, out);
	}
	EncodeAccumulating(v, n, out);
}

void Encoder::EncodeAccumulating(int v, int n, BS & out)
{
	out.insert(out.end(), v + 1, 1);
	out.insert(out.end(), n - v - 1, 0);
}

//
// BINARY
//

void Encoder::EncodeBinaryExplicitNull(int v, int n, BS & out)
{
	out.push_back(v == S13::NULL_VALUE_UNENCODED);
	EncodeBinary(v, n - 1, out);
}

void Encoder::EncodeBinaryMaskingNull(int v, int n, BS & out)
{
	MAYBE_ADD_MASKED_AND_RETURN(v, n, out);
	EncodeBinary(v, n, out);
}

void Encoder::EncodeBinaryStrictNull(int v, int n, BS & out)
{
	MAYBE_THROW_STRICT_ERROR(v);
	EncodeBinary(v, n, out);
}

void Encoder::EncodeBinaryZeroNull(int v, int n, BS & out)
{
	EncodeBinary(v, n, out);
}

void Encoder::EncodeBinary(int v, int n, BS & out)
{
	MAYBE_ADD_ZEROS_AND_RETURN(v, n, out);

	int vtmp = v;
	for(int i = 0; i < n; ++i)
	{
		out.push_back(vtmp % 2);
		vtmp /= 2;
	}
}

//
// CATEGORICAL
//

void Encoder::EncodeCategoricalExplicitNull(int v, int n, BS & out)
{
	if(v == S13::NULL_VALUE_UNENCODED)
	{
		out.push_back(v == S13::NULL_VALUE_UNENCODED);
		ADD_ZEROS_AND_RETURN(n - 1, out);
	}
	out.push_back(0);
	EncodeCategorical(v, n - 1, out);
}

void Encoder::EncodeCategoricalImplicitNull(int v, int n, BS & out)
{
	if(v == S13::NULL_VALUE_UNENCODED)
	{
		ADD_ZEROS_AND_RETURN(n, out);
	}

	EncodeCategorical(v, n, out);
}

void Encoder::EncodeCategoricalMaskingNull(int v, int n, BS & out)
{
	MAYBE_ADD_MASKED_AND_RETURN(v, n, out);
	EncodeCategorical(v, n, out);
}

void Encoder::EncodeCategoricalStrictNull(int v, int n, BS & out)
{
	MAYBE_THROW_STRICT_ERROR(v);
	EncodeCategorical(v, n, out);
}

void Encoder::EncodeCategoricalZeroNull(int v, int n, BS & out)
{
	EncodeCategorical(v, n, out);
}

void Encoder::EncodeCategorical(int v, int n, BS & out)
{
	if(v <= 0)
	{
		out.push_back(1);
		ADD_ZEROS_AND_RETURN(n - 1, out);
	}

	for(int i = 0; i < n; ++i)
	{
		if(i == v)
		{
			out.push_back(1);
			ADD_ZEROS_AND_RETURN(n - i - 1, out);
		}
		else
		{
			out.push_back(0);
		}
	}
}

//
// EXPNORM
//

void Encoder::EncodeExpnormExplicitNull(int v, int vmax, double slope, BS & out)
{
	out.push_back(v == S13::NULL_VALUE_UNENCODED);
	EncodeExpnorm(v, vmax, slope, out);
}

void Encoder::EncodeExpnormMaskingNull(int v, int vmax, double slope, BS & out)
{
	if(v == S13::NULL_VALUE_UNENCODED)
	{
		out.push_back(S13::NULL_VALUE_ENCODED);
		return;
	}
	EncodeExpnorm(v, vmax, slope, out);
}

void Encoder::EncodeExpnormStrictNull(int v, int vmax, double slope, BS & out)
{
	MAYBE_THROW_STRICT_ERROR(v);
	EncodeExpnorm(v, vmax, slope, out);
}

void Encoder::EncodeExpnormZeroNull(int v, int vmax, double slope, BS & out)
{
	EncodeExpnorm(v, vmax, slope, out);
}

void Encoder::EncodeExpnorm(int v, int vmax, double slope, BS & out)
{
	if(v <= 0)
	{
		out.push_back(0);
		return;
	}

	out.push_back(CalcExpnorm(v, vmax, slope));
}

// Visualise on https://www.desmos.com/calculator:
// ln(1 + (x/M) * (exp(S)-1))/S
// Add slider "S" (slope) and "M" (vmax).
// Play with the sliders to see the nonlinearity (use M=1 for best view)
// XXX: slope cannot be 0
float Encoder::CalcExpnorm(int v, int vmax, double slope)
{
	auto ratio = static_cast<double>(v) / vmax;
	return std::log1p(ratio * (std::exp(slope) - 1.0)) / (slope + 1e-6);
}

//
// LINNORM
//

void Encoder::EncodeLinnormExplicitNull(int v, int vmax, BS & out)
{
	out.push_back(v == S13::NULL_VALUE_UNENCODED);
	EncodeLinnorm(v, vmax, out);
}

void Encoder::EncodeLinnormMaskingNull(int v, int vmax, BS & out)
{
	if(v == S13::NULL_VALUE_UNENCODED)
	{
		out.push_back(S13::NULL_VALUE_ENCODED);
		return;
	}
	EncodeLinnorm(v, vmax, out);
}

void Encoder::EncodeLinnormStrictNull(int v, int vmax, BS & out)
{
	MAYBE_THROW_STRICT_ERROR(v);
	EncodeLinnorm(v, vmax, out);
}

void Encoder::EncodeLinnormZeroNull(int v, int vmax, BS & out)
{
	EncodeLinnorm(v, vmax, out);
}

void Encoder::EncodeLinnorm(int v, int vmax, BS & out)
{
	if(v <= 0)
	{
		out.push_back(0);
		return;
	}

	// XXX: this is a simplified version for 0..1 norm
	out.push_back(CalcLinnorm(v, vmax));
}

float Encoder::CalcLinnorm(int v, int vmax)
{
	return static_cast<float>(v) / static_cast<float>(vmax);
}
}
