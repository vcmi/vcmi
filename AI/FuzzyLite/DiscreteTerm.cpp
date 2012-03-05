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
#include "DiscreteTerm.h"

namespace fl {

    DiscreteTerm::DiscreteTerm() : LinguisticTerm() {

    }

    DiscreteTerm::DiscreteTerm(const std::string& name, const std::vector<flScalar>& x,
            const std::vector<flScalar>& y)
    : LinguisticTerm(name) {
        setXY(x, y);
    }

    DiscreteTerm::DiscreteTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
            const std::vector<flScalar>& x, const std::vector<flScalar>& y)
    : LinguisticTerm(fuzzy_op, name) {
        setXY(x, y);
    }

    DiscreteTerm::~DiscreteTerm() {
    }

    void DiscreteTerm::setXY(const std::vector<flScalar>& x, const std::vector<flScalar>& y) {
        this->_x = x;
        this->_y = y;
    }

    void DiscreteTerm::setXY(int index, flScalar x, flScalar y) {
        this->_x.insert(this->_x.begin() + index, x);
        this->_y.insert(this->_y.begin() + index, y);
    }

    void DiscreteTerm::addXY(flScalar x, flScalar y) {
        this->_x.push_back(x);
        this->_y.push_back(y);
    }

    void DiscreteTerm::remove(int index) {
        this->_x.erase(this->_x.begin() + index);
        this->_y.erase(this->_y.begin() + index);
    }

    void DiscreteTerm::clear() {
        this->_x.clear();
        this->_y.clear();
    }

    int DiscreteTerm::numberOfCoords() const {
        return (int) this->_x.size();
    }

    flScalar DiscreteTerm::minimum() const {
        flScalar result = INFINITY;
        for (int i = 0; i < numberOfCoords(); ++i) {
            if (_x[i] < result) {
                result = _x[i];
            }
        }
        return isinf(result) ? NAN : result;
    }

    flScalar DiscreteTerm::maximum() const {
        flScalar result = -INFINITY;
        for (int i = 0; i < numberOfCoords(); ++i) {
            if (_x[i] > result) {
                result = _x[i];
            }
        }
        return isinf(result) ? NAN : result;
    }

    DiscreteTerm* DiscreteTerm::clone() const {
        return new DiscreteTerm(*this);
    }

    flScalar DiscreteTerm::membership(flScalar crisp) const {
        flScalar closer = INFINITY;
        flScalar result = NAN;
        for (int i = 0; i < numberOfCoords(); ++i) {
            if (fabs(crisp - _x[i]) < closer) {
                closer = fabs(crisp - _x[i]);
                result = _y[i];
            }
        }
        return result;
    }

    LinguisticTerm::eMembershipFunction DiscreteTerm::type() const {
        return LinguisticTerm::MF_DISCRETE;
    }

    std::string DiscreteTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Discrete (";
        for (int i = 0; i < numberOfCoords(); ++i) {
            ss << "{" << _x[i] << "," << _y[i] << "}";
            if (i < numberOfCoords() - 1) {
                ss << " ";
            }
        }
        ss << ")";
        return ss.str();
    }
}
