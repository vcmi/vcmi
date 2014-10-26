/*
 Author: Juan Rada-Vilela, Ph.D.
 Copyright (C) 2010-2014 FuzzyLite Limited
 All rights reserved

 This file is part of fuzzylite.

 fuzzylite is free software: you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation, either version 3 of the License, or (at your option)
 any later version.

 fuzzylite is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with fuzzylite.  If not, see <http://www.gnu.org/licenses/>.

 fuzzylite™ is a trademark of FuzzyLite Limited.

 */

#include "fl/defuzzifier/WeightedSum.h"

#include "fl/term/Accumulated.h"
#include "fl/term/Activated.h"
#include "fl/norm/SNorm.h"
#include "fl/norm/TNorm.h"

#include <map>
namespace fl {

    WeightedSum::WeightedSum(Type type) : WeightedDefuzzifier(type) {
    }

    WeightedSum::WeightedSum(const std::string& type) : WeightedDefuzzifier(type) {

    }

    WeightedSum::~WeightedSum() {
    }

    std::string WeightedSum::className() const {
        return "WeightedSum";
    }

    scalar WeightedSum::defuzzify(const Term* term,
            scalar minimum, scalar maximum) const {
        const Accumulated* fuzzyOutput = dynamic_cast<const Accumulated*> (term);
        if (not fuzzyOutput) {
            std::ostringstream ss;
            ss << "[defuzzification error]"
                    << "expected an Accumulated term instead of"
                    << "<" << term->toString() << ">";
            throw fl::Exception(ss.str(), FL_AT);
        }

        minimum = fuzzyOutput->getMinimum();
        maximum = fuzzyOutput->getMaximum();


        scalar sum = 0.0;

        if (not fuzzyOutput->getAccumulation()) {
            Type type = _type;
            for (int i = 0; i < fuzzyOutput->numberOfTerms(); ++i) {
                Activated* activated = fuzzyOutput->getTerm(i);
                scalar w = activated->getDegree();

                if (type == Automatic) type = inferType(activated->getTerm());

                scalar z = (type == TakagiSugeno)
                        //? activated.getTerm()->membership(fl::nan) Would ensure no Tsukamoto applies, but Inverse Tsukamoto with Functions would not work.
                        ? activated->getTerm()->membership(w) //Provides Takagi-Sugeno and Inverse Tsukamoto of Functions
                        : tsukamoto(activated->getTerm(), w, minimum, maximum);

                sum += w * z;
            }
        } else {
            typedef std::map<const Term*, std::vector<Activated*> > TermGroup;
            TermGroup groups;
            for (int i = 0; i < fuzzyOutput->numberOfTerms(); ++i) {
                Activated* value = fuzzyOutput->getTerm(i);
                const Term* key = value->getTerm();
                groups[key].push_back(value);
            }
            TermGroup::const_iterator it = groups.begin();
            Type type = _type;
            while (it != groups.end()) {
                const Term* activatedTerm = it->first;
                scalar accumulatedDegree = 0.0;
                for (std::size_t i = 0; i < it->second.size(); ++i)
                    accumulatedDegree = fuzzyOutput->getAccumulation()->compute(
                        accumulatedDegree, it->second.at(i)->getDegree());

                if (type == Automatic) type = inferType(activatedTerm);

                scalar z = (type == TakagiSugeno)
                        //? activated.getTerm()->membership(fl::nan) Would ensure no Tsukamoto applies, but Inverse Tsukamoto with Functions would not work.
                        ? activatedTerm->membership(accumulatedDegree) //Provides Takagi-Sugeno and Inverse Tsukamoto of Functions
                        : tsukamoto(activatedTerm, accumulatedDegree, minimum, maximum);

                sum += accumulatedDegree * z;

                ++it;
            }
        }
        return sum;
    }

    WeightedSum* WeightedSum::clone() const {
        return new WeightedSum(*this);
    }

    Defuzzifier* WeightedSum::constructor() {
        return new WeightedSum;
    }

}
