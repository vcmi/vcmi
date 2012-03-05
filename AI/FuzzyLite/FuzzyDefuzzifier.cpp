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

#include "FuzzyDefuzzifier.h"
#include "CompoundTerm.h"
#include "TakagiSugenoTerm.h"
namespace fl {

    FuzzyDefuzzifier::~FuzzyDefuzzifier() {
    }

    std::string CoGDefuzzifier::name() const {
        return "CoG";
    }

    flScalar CoGDefuzzifier::defuzzify(const LinguisticTerm* term, int number_of_samples,
            const AreaCentroidAlgorithm* algorithm) const {
        flScalar x, y;
        algorithm->centroid(term, x, y, number_of_samples);
        return x;
    }

    std::string TakagiSugenoDefuzzifier::name() const {
        return "TSK";
    }

    flScalar TakagiSugenoDefuzzifier::defuzzify(const LinguisticTerm* term, int number_of_samples,
            const AreaCentroidAlgorithm* algorithm) const {
        const CompoundTerm* out = dynamic_cast<const CompoundTerm*> (term);
        if (!out) {
            FL_LOG("<CompoundTerm> expected, but <" << term->toString() << "> found." <<
                    "Takagi-Sugeno defuzzification works only on the output of Takagi-Sugeno Rules." );
            return -1;
        }
        flScalar result = 0;
        flScalar sum_wi = 0;
        for (int i = 0; i < out->numberOfTerms(); ++i) {
            const TakagiSugenoTerm* t = dynamic_cast<const TakagiSugenoTerm*> (&out->term(i));
            if (!t) {
                FL_LOG("<TakagiSugenoTerm> expected, but <" << out->term(i).toString() << "> found." <<
                        "Takagi-Sugeno defuzzification works only on the output of Takagi-Sugeno Rules");
                return -1;
            }
            result += t->value() * t->weight();
            sum_wi += t->weight();
        }
        return result / sum_wi;
    }
}

