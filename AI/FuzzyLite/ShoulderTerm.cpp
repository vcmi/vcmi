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
 * ShoulderTerm.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#include "ShoulderTerm.h"
namespace fl {

    ShoulderTerm::ShoulderTerm() :
    LinguisticTerm(), _left(true) {

    }

    ShoulderTerm::ShoulderTerm(const std::string& name, flScalar minimum,
            flScalar maximum, bool left) :
    LinguisticTerm(name, minimum, maximum), _left(left) {

    }

    ShoulderTerm::ShoulderTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
            flScalar minimum, flScalar maximum, bool left) :
    LinguisticTerm(fuzzy_op, name, minimum, maximum), _left(left) {

    }

    ShoulderTerm::~ShoulderTerm() {

    }

    void ShoulderTerm::setLeft(bool left) {
        this->_left = left;

    }

    bool ShoulderTerm::isLeft() const {
        return this->_left;
    }

    void ShoulderTerm::setRight(bool right) {
        this->_left = !right;
    }

    bool ShoulderTerm::isRight() const {
        return !this->_left;
    }

    ShoulderTerm* ShoulderTerm::clone() const {
        return new ShoulderTerm(*this);
    }

    flScalar ShoulderTerm::membership(flScalar crisp) const {
        if (isLeft()) {
            if (crisp < minimum())
                return flScalar(1.0);
            if (crisp > maximum())
                return flScalar(0.0);
            return FuzzyOperation::Scale(minimum(), maximum(), crisp, 1, 0);
        } else {
            if (crisp < minimum())
                return flScalar(0.0);
            if (crisp > maximum())
                return flScalar(1.0);
            return FuzzyOperation::Scale(minimum(), maximum(), crisp, 0, 1);
        }
    }

    LinguisticTerm::eMembershipFunction ShoulderTerm::type() const {
        return MF_SHOULDER;
    }

    std::string ShoulderTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Shoulder " << (isLeft() ? "left" : "right") << "(" << minimum() << " " << maximum() << ")";
        return ss.str();
    }

    //**********Right and left...

    RightShoulderTerm::RightShoulderTerm() : ShoulderTerm() {
        setRight(true);
    }

    RightShoulderTerm::RightShoulderTerm(const std::string& name, flScalar minimum, flScalar maximum)
    : ShoulderTerm(name, minimum, maximum) {
        setRight(true);
    }

    RightShoulderTerm::RightShoulderTerm(const FuzzyOperator& fuzzy_op, const std::string& name, flScalar minimum, flScalar maximum)
    : ShoulderTerm(fuzzy_op, name, minimum, maximum) {
        setRight(true);
    }

    RightShoulderTerm::~RightShoulderTerm() {

    }

    LeftShoulderTerm::LeftShoulderTerm() : ShoulderTerm() {
        setLeft(true);
    }

    LeftShoulderTerm::LeftShoulderTerm(const std::string& name, flScalar minimum, flScalar maximum)
    : ShoulderTerm(name, minimum, maximum) {
        setLeft(true);
    }

    LeftShoulderTerm::LeftShoulderTerm(const FuzzyOperator& fuzzy_op, const std::string& name, flScalar minimum, flScalar maximum)
    : ShoulderTerm(fuzzy_op, name, minimum, maximum) {
        setLeft(true);
    }

    LeftShoulderTerm::~LeftShoulderTerm() {

    }


} // namespace fuzzy_lite
