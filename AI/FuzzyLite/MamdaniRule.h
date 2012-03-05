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
 * File:   MamdaniRule.h
 * Author: jcrada
 *
 * Created on November 5, 2009, 11:15 AM
 */

#ifndef FL_MAMDANIRULE_H
#define	FL_MAMDANIRULE_H

#include "FuzzyRule.h"

namespace fl {

    class MamdaniRule : public FuzzyRule {
    public:

        MamdaniRule();
        MamdaniRule(const std::string& rule, const FuzzyEngine& engine);
        virtual ~MamdaniRule();

        virtual void parse(const std::string& rule, const FuzzyEngine& engine) throw (ParsingException);

    };
}

#endif	/* FL_MAMDANIRULE_H */

