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
#include "LinguisticVariable.h"
#include "FuzzyExceptions.h"
#include "StrOp.h"
#include <algorithm>
#include <math.h>

#include "ShoulderTerm.h"
#include "TriangularTerm.h"
#include "TrapezoidalTerm.h"

namespace fl {

    LinguisticVariable::LinguisticVariable() :
    _fuzzy_operator(NULL) {
        setFuzzyOperator(FuzzyOperator::DefaultFuzzyOperator());
    }

    LinguisticVariable::LinguisticVariable(const std::string& name) :
    _fuzzy_operator(NULL), _name(name) {
        setFuzzyOperator(FuzzyOperator::DefaultFuzzyOperator());
    }

    LinguisticVariable::LinguisticVariable(const FuzzyOperator& fuzzy_op,
            const std::string& name) :
    _fuzzy_operator(&fuzzy_op), _name(name) {

    }

    LinguisticVariable::~LinguisticVariable() {
        for (int i = numberOfTerms() - 1; i >= 0; --i) {
            delete removeTerm(i);
        }
    }

    std::string LinguisticVariable::name() const {
        return this->_name;
    }

    void LinguisticVariable::setName(const std::string& name) {
        this->_name = name;
    }

    void LinguisticVariable::setFuzzyOperator(const FuzzyOperator& fuzzy_operator) {
        this->_fuzzy_operator = &fuzzy_operator;
    }

    const FuzzyOperator& LinguisticVariable::fuzzyOperator() const {
        return *this->_fuzzy_operator;
    }

    int LinguisticVariable::positionFor(const LinguisticTerm* lterm) {
        for (int i = 0; i < numberOfTerms(); ++i) {
            if (FuzzyOperation::IsLEq(lterm->minimum(), term(i)->minimum())) {
                if (FuzzyOperation::IsEq(lterm->minimum(), term(i)->minimum())) {
                    return lterm->membership(lterm->minimum()) > term(i)->membership(lterm->minimum())
                            ? i : i + 1;
                }
                return i;
            }
        }
        return numberOfTerms();
    }

    void LinguisticVariable::addTerm(LinguisticTerm* term) {
        int pos = positionFor(term);
        _terms.insert(_terms.begin() + pos, term);
    }

    LinguisticTerm* LinguisticVariable::term(int index) const {
        return _terms[index];
    }

    LinguisticTerm* LinguisticVariable::term(const std::string& name) const {
        int index = indexOf(name);
        return index == -1 ? NULL : term(index);
    }

    int LinguisticVariable::indexOf(const std::string& name) const {
        for (int i = 0; i < numberOfTerms(); ++i) {
            if (term(i)->name() == name) {
                return i;
            }
        }
        return -1;
    }

    LinguisticTerm* LinguisticVariable::removeTerm(int index) {
        LinguisticTerm* result = _terms[index];
        _terms.erase(_terms.begin() + index);
        return result;
    }

    LinguisticTerm* LinguisticVariable::removeTerm(const std::string& name) {
        int index = indexOf(name);
        return index == -1 ? NULL : removeTerm(index);
    }

    LinguisticTerm* LinguisticVariable::firstTerm() const {
        return numberOfTerms() > 0 ? term(0) : NULL;
    }

    LinguisticTerm* LinguisticVariable::lastTerm() const {
        return numberOfTerms() > 0 ? term(numberOfTerms() - 1) : NULL;
    }

    bool LinguisticVariable::isEmpty() const {
        return numberOfTerms() == 0;
    }

    int LinguisticVariable::numberOfTerms() const {
        return _terms.size();
    }

    flScalar LinguisticVariable::minimum() const {
        return numberOfTerms() == 0 ? NAN : firstTerm()->minimum();
    }

    flScalar LinguisticVariable::maximum() const {
        return numberOfTerms() == 0 ? NAN : lastTerm()->maximum();
    }

    CompoundTerm LinguisticVariable::compound() const {
        CompoundTerm result("Accumulated " + name(), minimum(), maximum());
        for (int i = 0; i < numberOfTerms(); ++i) {
            result.addTerm(*term(i));
        }
        return result;
    }

    std::string LinguisticVariable::fuzzify(flScalar crisp) const {
        std::vector<std::string> fuzzyness;
        for (int i = 0; i < numberOfTerms(); ++i) {
            flScalar degree = term(i)->membership(crisp);
            fuzzyness.push_back(StrOp::ScalarToString(degree) + "/"
                    + term(i)->name());
        }
        //        StringOperator::sort(fuzzyness, false);
        std::string result;
        for (size_t i = 0; i < fuzzyness.size(); ++i) {
            result += fuzzyness[i] + (i < fuzzyness.size() - 1 ? " + " : "");
        }
        return result;
    }

    LinguisticTerm* LinguisticVariable::bestFuzzyApproximation(flScalar crisp) {
        LinguisticTerm* result = NULL;
        flScalar highest_degree = -1.0;
        for (int i = 0; i < numberOfTerms(); ++i) {
            flScalar degree = term(i)->membership(crisp);
            if (degree > highest_degree) {
                result = term(i);
                highest_degree = degree;
            }
        }
        return result;
    }

    void LinguisticVariable::createTerms(int number_of_terms,
            LinguisticTerm::eMembershipFunction mf, flScalar min, flScalar max,
            const std::vector<std::string>& labels) {
        std::vector<std::string> final_labels;
        for (int i = 0; i < number_of_terms; ++i) {
            if ((int) labels.size() <= i) {
                final_labels.push_back(name() + "-" + StrOp::IntToString(i + 1));
            } else {
                final_labels.push_back(labels[i]);
            }
        }
        fl::flScalar intersection = NAN; //Proportion of intersection between terms
        if (mf == LinguisticTerm::MF_TRAPEZOIDAL) {
            intersection = 4.0 / 5.0;
        } else {
            intersection = 0.5;
        }
        //TODO: What is the intersection in other terms?

        fl::flScalar term_range = (max - min) / (number_of_terms - number_of_terms / 2);
        fl::flScalar current_step = min + (1 - intersection) * term_range;
        for (int i = 0; i < number_of_terms; ++i) {
            fl::LinguisticTerm* term = NULL;
            switch (mf) {
                case LinguisticTerm::MF_TRIANGULAR:
                    term = new fl::TriangularTerm(final_labels.at(i), 
                            current_step - (1 - intersection) * term_range,
                            current_step + intersection * term_range);
                    break;
                case LinguisticTerm::MF_SHOULDER:
                    if (i == 0 || i == number_of_terms - 1) {
                        term = new fl::ShoulderTerm(final_labels[i],
                                current_step - (1 - intersection) * term_range,
                                current_step + intersection * term_range, i == 0);
                    } else {
                        term = new fl::TriangularTerm(final_labels.at(i),
                                current_step - (1 - intersection) * term_range, 
                                current_step + intersection * term_range);
                    }
                    break;
                case LinguisticTerm::MF_TRAPEZOIDAL:

                    term = new fl::TrapezoidalTerm(final_labels.at(i),
                            current_step - (1 - intersection) * term_range,
                            current_step + intersection * term_range);
//                    term = new fl::TriangularTerm(final_labels.at(i), current_step
//                            - (1 - intersection) * term_range, current_step + intersection
//                            * term_range);
                    break;
                default:
                    throw fl::InvalidArgumentException(FL_AT, "Invalid option for membership function");
            }
            current_step += intersection * term_range;
            addTerm(term);
        }
    }

    std::string LinguisticVariable::toString() const {
        std::string result(name());
        result += "={ ";
        for (int i = 0; i < numberOfTerms(); ++i) {
            result += term(i)->name() + " ";
        }
        result += "}";
        return result;
    }
}
