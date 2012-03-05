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
 * File:   TakagiSugenoRule.h
 * Author: jcrada
 *
 * Created on March 4, 2010, 12:22 AM
 */

#ifndef FL_TAKAGISUGENORULE_H
#define	FL_TAKAGISUGENORULE_H

#include "FuzzyRule.h"

namespace fl {

    class TakagiSugenoRule : public FuzzyRule {
    public:
        TakagiSugenoRule();
        TakagiSugenoRule(const std::string& rule, const FuzzyEngine& engine);
        virtual ~TakagiSugenoRule();

        virtual void parse(const std::string& rule, const FuzzyEngine& engine);
    };
}

#endif	/* FL_TAKAGISUGENORULE_H */

