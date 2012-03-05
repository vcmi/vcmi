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
 * File:   MamdaniConsequent.h
 * Author: jcrada
 *
 * Created on March 3, 2010, 12:17 AM
 */

#ifndef FL_MAMDANICONSEQUENT_H
#define	FL_MAMDANICONSEQUENT_H

#include "FuzzyConsequent.h"
#include "LinguisticTerm.h"
#include "Hedge.h"
#include <vector>
#include "FuzzyExceptions.h"
#include "FuzzyEngine.h"

namespace fl {

    class MamdaniConsequent : public FuzzyConsequent {
    private:
        const LinguisticTerm* _term;
        std::vector<Hedge*> _hedges;
    public:
        MamdaniConsequent();
        virtual ~MamdaniConsequent();

        virtual void setTerm(const LinguisticTerm* term);
        virtual const LinguisticTerm* term() const;

        virtual void addHedge(Hedge* hedge);
        virtual Hedge* hedge(int index) const ;
        virtual int numberOfHedges() const;

        //Override
        virtual void execute(flScalar degree);
        virtual std::string toString() const;
        virtual void parse(const std::string& consequent,
                const FuzzyEngine& engine) throw (ParsingException) ;

    };

}

#endif	/* FL_MAMDANICONSEQUENT_H */

