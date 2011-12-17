#pragma once

#include "../../Global.h"

// This header should be treated as a pre compiled header file(PCH) in the compiler building settings.

// Here you can add specific libraries and macros which are specific to this project.
#include <boost/variant.hpp>
#include <boost/version.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = spirit::ascii;
namespace phoenix = boost::phoenix;
