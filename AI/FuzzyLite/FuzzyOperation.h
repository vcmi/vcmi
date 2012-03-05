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
 * File:   FuzzyOperation.h
 * Author: jcrada
 *
 * Created on April 6, 2010, 6:42 PM
 */

#ifndef FL_FUZZYOPERATION_H
#define	FL_FUZZYOPERATION_H

#include "flScalar.h"
#include <string>
namespace fl {

    class FuzzyOperation {
    public:

        virtual ~FuzzyOperation();
        virtual std::string name() const = 0;
        virtual flScalar execute(flScalar mu1, flScalar mu2) const = 0;

        //TNORMS
        static flScalar Min(flScalar mu1, flScalar mu2);
        static flScalar AlgebraicProduct(flScalar mu1, flScalar mu2);
        static flScalar BoundedDiff(flScalar mu1, flScalar mu2);
        //SNORMS
        static flScalar Max(flScalar mu1, flScalar mu2);
        static flScalar AlgebraicSum(flScalar mu1, flScalar mu2);
        static flScalar BoundedSum(flScalar mu1, flScalar mu2);

        //Other operators
        static flScalar Absolute(flScalar value);
        static bool IsEq(flScalar x1, flScalar x2, flScalar epsilon = FL_EPSILON);
        static bool IsLEq(flScalar x1, flScalar x2, flScalar epsilon = FL_EPSILON);
        static bool IsGEq(flScalar x1, flScalar x2, flScalar epsilon = FL_EPSILON);

        static flScalar Scale(flScalar src_min, flScalar src_max, flScalar value,
                flScalar target_min, flScalar target_max);

    };
}

#endif	/* FL_FUZZYOPERATION_H */

