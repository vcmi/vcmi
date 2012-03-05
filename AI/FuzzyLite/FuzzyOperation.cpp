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

#include "FuzzyOperation.h"
#include <math.h>

namespace fl {

    FuzzyOperation::~FuzzyOperation() {
    }

    //TNORMS

    flScalar FuzzyOperation::Min(flScalar mu1, flScalar mu2) {
        return mu1 < mu2 ? mu1 : mu2;
    }

    flScalar FuzzyOperation::AlgebraicProduct(flScalar mu1, flScalar mu2) {
        return mu1 * mu2;
    }

    flScalar FuzzyOperation::BoundedDiff(flScalar mu1, flScalar mu2) {
        return Max(0, mu1 + mu2 -1);
    }

    //SNORMS

    flScalar FuzzyOperation::Max(flScalar mu1, flScalar mu2) {
        return mu1 > mu2 ? mu1 : mu2;
    }

    flScalar FuzzyOperation::AlgebraicSum(flScalar mu1, flScalar mu2) {
        return mu1 + mu2 - (mu1 * mu2);
    }

    flScalar FuzzyOperation::BoundedSum(flScalar mu1, flScalar mu2) {
        return Min(1, mu1 + mu2);
    }

    //Other operators

    flScalar FuzzyOperation::Absolute(flScalar value) {
        return value < flScalar(0.0) ? value * flScalar(-1.0) : value;
    }

    bool FuzzyOperation::IsEq(flScalar x1, flScalar x2, flScalar epsilon) {
        //http://stackoverflow.com/questions/17333/most-effective-way-for-float-and-double-comparison
        //        return fabs(x1 - x2) < std::numeric_limits<flScalar>::epsilon() ;
        return fabs(x1 - x2) < epsilon;
    }

    bool FuzzyOperation::IsLEq(flScalar x1, flScalar x2, flScalar epsilon) {
        return IsEq(x1, x2, epsilon) ? true : x1 < x2;
    }

    bool FuzzyOperation::IsGEq(flScalar x1, flScalar x2, flScalar epsilon) {
        return IsEq(x1, x2,epsilon) ? true : x1 > x2;
    }

    flScalar FuzzyOperation::Scale(flScalar src_min, flScalar src_max, flScalar value,
            flScalar target_min, flScalar target_max) {
        //Courtesy of Rubén Parma :)
        return (target_max - target_min) / (src_max - src_min) * (value - src_min)
                + target_min;
    }
}
