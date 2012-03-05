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
#include "LinguisticTerm.h"

#include "TriangularTerm.h"
#include "TrapezoidalTerm.h"
#include "ShoulderTerm.h"
#include "FuzzyException.h"
namespace fl {

    LinguisticTerm::LinguisticTerm() :
    _fuzzy_operator(NULL), _name(""), _minimum(-INFINITY), _maximum(INFINITY),
    _modulation(flScalar(1.0)) {
        setFuzzyOperator(FuzzyOperator::DefaultFuzzyOperator());
    }

    LinguisticTerm::LinguisticTerm(const std::string& name, flScalar minimum,
            flScalar maximum) :
    _fuzzy_operator(NULL), _name(name), _minimum(minimum), _maximum(maximum),
    _modulation(flScalar(1.0)) {
        setFuzzyOperator(FuzzyOperator::DefaultFuzzyOperator());
    }

    LinguisticTerm::LinguisticTerm(const FuzzyOperator& fuzzy_op,
            const std::string& name, flScalar minimum, flScalar maximum) :
    _fuzzy_operator(&fuzzy_op), _name(name), _minimum(minimum), _maximum(maximum),
    _modulation(flScalar(1.0)) {

    }

    LinguisticTerm::~LinguisticTerm() {

    }

    void LinguisticTerm::setFuzzyOperator(const FuzzyOperator& fuzzy_operator) {
        this->_fuzzy_operator = &fuzzy_operator;
    }

    const FuzzyOperator& LinguisticTerm::fuzzyOperator() const {
        return *this->_fuzzy_operator;
    }

    void LinguisticTerm::setName(const std::string& name) {
        this->_name = name;
    }

    std::string LinguisticTerm::name() const {
        return this->_name;
    }

    void LinguisticTerm::setMinimum(flScalar min) {
        this->_minimum = min;
    }

    flScalar LinguisticTerm::minimum() const {
        return _minimum;
    }

    void LinguisticTerm::setMaximum(flScalar max) {
        this->_maximum = max;
    }

    flScalar LinguisticTerm::maximum() const {
        return _maximum;
    }

    void LinguisticTerm::setModulation(flScalar degree) {
        this->_modulation = degree;
    }

    flScalar LinguisticTerm::modulation() const {
        return this->_modulation;
    }

    flScalar LinguisticTerm::defuzzify() const {
        return fuzzyOperator().defuzzify(this);
    }

    flScalar LinguisticTerm::area() const {
        return fuzzyOperator().area(this);
    }

    void LinguisticTerm::centroid(flScalar& x, flScalar& y) const {
        fuzzyOperator().centroid(this, x, y);
    }

    flScalar LinguisticTerm::areaAndCentroid(flScalar& x, flScalar& y) const {
        return fuzzyOperator().areaAndCentroid(this, x, y);
    }

    void LinguisticTerm::samples(std::vector<flScalar>& x, std::vector<flScalar>& y,
            int samples, int out_of_range) const {
        flScalar step_size = (maximum() - minimum()) / samples;
        flScalar step = minimum() - (out_of_range * step_size);
        for (int i = 0 - out_of_range; i < samples + out_of_range + 1; ++i, step
                += step_size) {
            x.push_back(step);
            y.push_back(membership(step));
        }
    }

    std::string LinguisticTerm::toString() const {
        return "TERM " + name() + " := ";
    }

}
