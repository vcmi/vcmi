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
 * File:   Term.h
 * Author: jcrada
 *
 * Created on December 9, 2009, 12:27 AM
 */

#ifndef FL_TERM_H
#define	FL_TERM_H

#include "flScalar.h"
#include <vector>
#include <math.h>
#include <string>
#include "defs.h"
#include "FuzzyOperation.h"
#include "FuzzyOperator.h"
namespace fl {
    class FuzzyOperator; //forward-declaration

    class LinguisticTerm {
    public:

        enum eMembershipFunction {
            MF_TRIANGULAR = 0, MF_TRAPEZOIDAL, MF_RECTANGULAR, MF_SHOULDER, MF_SINGLETON,
            MF_COMPOUND, MF_FUNCTION, MF_TAKAGI_SUGENO, MF_DISCRETE,
            //to implement
            MF_SIGMOIDAL, MF_COSINE, MF_GAUSSIAN, 
        };
    private:
        const FuzzyOperator* _fuzzy_operator;
        std::string _name;
        flScalar _minimum;
        flScalar _maximum;
        flScalar _modulation;

    public:
        LinguisticTerm();
        LinguisticTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        LinguisticTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY);
        virtual ~LinguisticTerm();

        virtual void setFuzzyOperator(const FuzzyOperator& fuzzy_op);
        virtual const FuzzyOperator& fuzzyOperator() const;

        virtual void setName(const std::string& name);
        virtual std::string name() const;

        virtual void setMinimum(flScalar min);
        virtual flScalar minimum() const;

        virtual void setMaximum(flScalar max);
        virtual flScalar maximum() const;

        virtual void setModulation(flScalar degree);
        virtual flScalar modulation() const;

        virtual flScalar defuzzify() const;
        
        virtual std::string toString() const;

        virtual LinguisticTerm* clone() const = 0;
        virtual flScalar membership(flScalar crisp) const = 0;
        virtual eMembershipFunction type() const = 0;
        

        virtual void samples(std::vector<flScalar>& x, std::vector<flScalar>& y,
                int samples = FL_SAMPLE_SIZE, int out_of_range = 0) const;

        virtual flScalar area() const;

        virtual void centroid(flScalar&x, flScalar& y) const;

        virtual flScalar areaAndCentroid(flScalar& x, flScalar& y) const;

    };
}

#endif	/* FL_TERM_H */

