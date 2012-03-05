/*   Copyright 2010 Juan Rada-Vilela

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */
/*
 * Hedge.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */
#include "Hedge.h"

namespace fl {

    Hedge::~Hedge() {
    }

    std::string HedgeNot::name() const {
        return "not";
    }

    flScalar HedgeNot::hedge(flScalar mu) const {
        return 1 - mu;
    }

    std::string HedgeSomewhat::name() const {
        return "somewhat";
    }

    flScalar HedgeSomewhat::hedge(flScalar mu) const {
        return sqrt(mu);
    }

    std::string HedgeVery::name() const {
        return "very";
    }

    flScalar HedgeVery::hedge(flScalar mu) const {
        return mu * mu;
    }

    std::string HedgeAny::name() const {
        return "any";
    }

    flScalar HedgeAny::hedge(flScalar mu) const {
        return 1.0;
    }
}
