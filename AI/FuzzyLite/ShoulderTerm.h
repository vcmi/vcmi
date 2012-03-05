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
 * ShoulderTerm.h
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#ifndef FL_SHOULDERTERM_H_
#define FL_SHOULDERTERM_H_

#include "LinguisticTerm.h"

#include <math.h>
namespace fl {

    class ShoulderTerm : public LinguisticTerm {
    private:
        bool _left;

    public:
        ShoulderTerm();
        ShoulderTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY, bool left = true);
        ShoulderTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY, bool left = true);
        virtual ~ShoulderTerm();

        virtual void setLeft(bool left);
        virtual bool isLeft() const;

        virtual void setRight(bool right);
        virtual bool isRight() const;

        virtual ShoulderTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;

        virtual eMembershipFunction type() const;
        virtual std::string toString() const;
    };

    class RightShoulderTerm : public ShoulderTerm{
    public:
        RightShoulderTerm();
        RightShoulderTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        RightShoulderTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY);
        virtual ~RightShoulderTerm();
    };

    class LeftShoulderTerm : public ShoulderTerm{
    public:
        LeftShoulderTerm();
        LeftShoulderTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        LeftShoulderTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY);
        virtual ~LeftShoulderTerm();
    };

} // namespace fuzzy_lite

#endif /* FL_SHOULDERTERM_H_ */
