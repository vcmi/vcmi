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

#include "FuzzyModulation.h"

namespace fl {

    std::string FuzzyModClip::name() const {
        return "Clip";
    }

    flScalar FuzzyModClip::execute(flScalar mu1, flScalar mu2) const {
        return FuzzyOperation::Min(mu1, mu2);
    }

    std::string FuzzyModScale::name() const {
        return "Scale";
    }

    flScalar FuzzyModScale::execute(flScalar mu1, flScalar mu2) const {
        return FuzzyOperation::AlgebraicProduct(mu1, mu2);
    }
}
