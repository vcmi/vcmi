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

#include "MamdaniConsequent.h"
#include "FuzzyRule.h"
#include <stdlib.h>

#include "StrOp.h"
namespace fl {

    MamdaniConsequent::MamdaniConsequent() : _term(NULL) {

    }

    MamdaniConsequent::~MamdaniConsequent() {

    }

    void MamdaniConsequent::setTerm(const LinguisticTerm* term) {
        this->_term = term;
    }

    const LinguisticTerm* MamdaniConsequent::term() const {
        return this->_term;
    }

    void MamdaniConsequent::addHedge(Hedge* hedge) {
        this->_hedges.push_back(hedge);
    }

    Hedge* MamdaniConsequent::hedge(int index) const {
        return _hedges[index];
    }

    int MamdaniConsequent::numberOfHedges() const {
        return _hedges.size();
    }

    void MamdaniConsequent::execute(flScalar degree) {
        LinguisticTerm* rule_output = term()->clone();
        flScalar modulation = degree * weight();
        for (int i = 0; i < numberOfHedges(); ++i) {
            modulation = hedge(i)->hedge(modulation);
        }
        rule_output->setModulation(modulation);
        outputLVar()->output().addTerm(*rule_output);
        delete rule_output;
    }

    std::string MamdaniConsequent::toString() const {
        std::stringstream ss;
        ss << outputLVar()->name() << " " << FuzzyRule::FR_IS;
        for (int i = 0; i < numberOfHedges(); ++i) {
            ss << " " << hedge(i)->name();
        }
        ss << " " << term()->name();
        if (weight() < flScalar(1.0)) {
            ss << " " + FuzzyRule::FR_WITH + " " << weight();
        }
        return ss.str();
    }

    void MamdaniConsequent::parse(const std::string& consequent,
            const FuzzyEngine& engine) throw (ParsingException) {
        std::stringstream ss(consequent);
        std::string token;

        enum e_state {
            S_LVAR = 1, S_IS, S_HEDGE, S_TERM, S_WITH, S_WEIGHT, S_END
        };
        e_state current_state = S_LVAR;

        OutputLVar* output = NULL;
        LinguisticTerm* term = NULL;
        std::vector<Hedge*> hedges;
        Hedge* hedge = NULL;
        while (ss >> token) {
            switch (current_state) {
                case S_LVAR:
                    output = engine.outputLVar(token);
                    if (!output) {
                        throw ParsingException(FL_AT, "Output variable <" +
                                token + "> not registered in fuzzy engine");
                    }
                    current_state = S_IS;
                    break;
                case S_IS:
                    if (token == FuzzyRule::FR_IS) {
                        current_state = S_HEDGE;
                    } else {
                        throw ParsingException(FL_AT, "<" + FuzzyRule::FR_IS + "> expected but found <" +
                                token + ">");
                    }
                    break;
                case S_HEDGE:
                    hedge = engine.hedgeSet().get(token);
                    if (hedge) {
                        hedges.push_back(hedge); //And check for more hedges
                        break;
                    }
                    //intentional fall-through if a term follows
                case S_TERM:
                    term = output->term(token);
                    if (!term) {
                        throw ParsingException(FL_AT, "Term <" + token +
                                "> not found in output variable <" + output->name() + ">");
                    }
                    setOutputLVar(output);
                    setTerm(term);
                    for (size_t i = 0; i < hedges.size(); ++i) {
                        addHedge(hedges[i]);
                    }
                    setWeight(1.0);
                    current_state = S_WITH;
                    break;
                case S_WITH:
                    if (token != FuzzyRule::FR_WITH) {
                        throw ParsingException(FL_AT, "<" + FuzzyRule::FR_WITH + "> expected but found <"
                                + token + ">");
                    }
                    current_state = S_WEIGHT;
                    break;
                case S_WEIGHT:
                    setWeight(atof(token.c_str()));
                    if (weight() > 1.0){
                        throw ParsingException(FL_AT, "Weight [0.0, 1.0] expected but found <" +
                                StrOp::ScalarToString(weight()) + ">");
                    }
                    current_state = S_END;
                    break;
                case S_END:
                    throw ParsingException(FL_AT, "End of line expected but found <" + token + ">");
            }
        }
        if (current_state == S_WEIGHT){
            throw ParsingException(FL_AT, "Weight [0.0, 1.0] expected after <" + FuzzyRule::FR_WITH +
                    "> but found nothing");
        }
    }
}
