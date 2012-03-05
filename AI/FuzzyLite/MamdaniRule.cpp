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
#include "MamdaniRule.h"

#include "defs.h"
#include "StrOp.h"
#include "FuzzyOperator.h"

#include <sstream>
#include <stack>

#include "DescriptiveAntecedent.h"
#include "MamdaniConsequent.h"
namespace fl {

    MamdaniRule::MamdaniRule()
    : FuzzyRule() {

    }

    MamdaniRule::MamdaniRule(const std::string& rule, const FuzzyEngine& engine)
    : FuzzyRule() {
        try{
            parse(rule,engine);
        }catch(ParsingException& e){
            FL_LOG(e.toString());
        }
    }

    MamdaniRule::~MamdaniRule() {

    }

    void MamdaniRule::parse(const std::string& rule, const FuzzyEngine& engine) throw (ParsingException) {
        std::string str_antecedent, str_consequent;
        ExtractFromRule(rule, str_antecedent, str_consequent);
        DescriptiveAntecedent* obj_antecedent = new DescriptiveAntecedent;
        obj_antecedent->parse(str_antecedent, engine);
        setAntecedent(obj_antecedent);
        std::vector<std::string> consequents = StrOp::SplitByWord(str_consequent, FuzzyRule::FR_AND);
        MamdaniConsequent* obj_consequent = NULL;
        for (size_t i = 0; i < consequents.size(); ++i) {
            obj_consequent = new MamdaniConsequent;
            try {
                obj_consequent->parse(consequents[i], engine);
            } catch (ParsingException& e) {
                delete obj_consequent;
                throw e;
            }
            addConsequent(obj_consequent);
        }
    }


}
