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
 * File:   FuzzyDefuzzifier.h
 * Author: jcrada
 *
 * Created on April 6, 2010, 5:16 PM
 */

#ifndef FL_FUZZYDEFUZZIFIER_H
#define	FL_FUZZYDEFUZZIFIER_H

#include "flScalar.h"
#include "AreaCentroidAlgorithm.h"
#include <string>
namespace fl {

    class LinguisticTerm;

    class FuzzyDefuzzifier {
    public:
        virtual ~FuzzyDefuzzifier();

        virtual std::string name() const = 0;
        virtual flScalar defuzzify(const LinguisticTerm* term, int number_of_samples,
                const AreaCentroidAlgorithm* algorithm) const = 0;
    };

    class CoGDefuzzifier : public FuzzyDefuzzifier {
    public:
        virtual std::string name() const;
        virtual flScalar defuzzify(const LinguisticTerm* term, int number_of_samples,
                const AreaCentroidAlgorithm* algorithm) const;
    };

    class TakagiSugenoDefuzzifier : public FuzzyDefuzzifier {
    public:
        virtual std::string name() const;
        virtual flScalar defuzzify(const LinguisticTerm* term, int number_of_samples,
                const AreaCentroidAlgorithm* algorithm) const;
    };
}

#endif	/* FL_FUZZYDEFUZZIFIER_H */

