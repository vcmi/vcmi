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
 * File:   FuzzyOr.h
 * Author: jcrada
 *
 * Created on April 6, 2010, 5:13 PM
 */

#ifndef FL_FUZZYOR_H
#define	FL_FUZZYOR_H

#include "FuzzyOperation.h"

namespace fl {

    class FuzzyOrMax : public FuzzyOperation {
    public:
        virtual std::string name() const;
        virtual flScalar execute(flScalar mu1, flScalar mu2) const;
    };

    class FuzzyOrSum : public FuzzyOperation {
    public:
        virtual std::string name() const;
        virtual flScalar execute(flScalar mu1, flScalar mu2) const;
    };

    class FuzzyOrBSum : public FuzzyOperation {
    public:
        virtual std::string name() const;
        virtual flScalar execute(flScalar mu1, flScalar mu2) const;
    };
}

#endif	/* FL_FUZZYOR_H */

