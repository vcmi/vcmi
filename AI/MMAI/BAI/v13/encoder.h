/*
 * encoder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "schema/base.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13
{
using GA = Schema::V13::GlobalAttribute;
using HA = Schema::V13::HexAttribute;
using PA = Schema::V13::PlayerAttribute;
using BS = Schema::BattlefieldState;

struct EncoderInput
{
	const std::string_view & attrname;
	const int a;
	const Schema::V13::Encoding e;
	const int n;
	const int vmax;
	const double p;
	const int v;
};

class Encoder
{
public:
	static void Encode(HA a, int v, BS & out);
	static void Encode(PA a, int v, BS & out);
	static void Encode(GA a, int v, BS & out);

	static void Encode(const EncoderInput & in, BS & out);

	static void EncodeAccumulatingExplicitNull(int v, int n, BS & out);
	static void EncodeAccumulatingImplicitNull(int v, int n, BS & out);
	static void EncodeAccumulatingMaskingNull(int v, int n, BS & out);
	static void EncodeAccumulatingStrictNull(int v, int n, BS & out);
	static void EncodeAccumulatingZeroNull(int v, int n, BS & out);

	static void EncodeBinaryExplicitNull(int v, int n, BS & out);
	static void EncodeBinaryMaskingNull(int v, int n, BS & out);
	static void EncodeBinaryStrictNull(int v, int n, BS & out);
	static void EncodeBinaryZeroNull(int v, int n, BS & out);

	static void EncodeCategoricalExplicitNull(int v, int n, BS & out);
	static void EncodeCategoricalImplicitNull(int v, int n, BS & out);
	static void EncodeCategoricalMaskingNull(int v, int n, BS & out);
	static void EncodeCategoricalStrictNull(int v, int n, BS & out);
	static void EncodeCategoricalZeroNull(int v, int n, BS & out);

	static void EncodeExpnormExplicitNull(int v, int vmax, double slope, BS & out);
	static void EncodeExpnormMaskingNull(int v, int vmax, double slope, BS & out);
	static void EncodeExpnormStrictNull(int v, int vmax, double slope, BS & out);
	static void EncodeExpnormZeroNull(int v, int vmax, double slope, BS & out);

	static void EncodeLinnormExplicitNull(int v, int vmax, BS & out);
	static void EncodeLinnormMaskingNull(int v, int vmax, BS & out);
	static void EncodeLinnormStrictNull(int v, int vmax, BS & out);
	static void EncodeLinnormZeroNull(int v, int vmax, BS & out);

	static float CalcExpnorm(int v, int vmax, double slope);
	static float CalcLinnorm(int v, int vmax);

private:
	static void EncodeAccumulating(int v, int n, BS & out);
	static void EncodeBinary(int v, int n, BS & out);
	static void EncodeCategorical(int v, int n, BS & out);
	static void EncodeExpnorm(int v, int vmax, double slope, BS & out);
	static void EncodeLinnorm(int v, int vmax, BS & out);
};
}
