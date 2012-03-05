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
#include "FuzzyEngine.h"

#include "MamdaniRule.h"
#include "FuzzyExceptions.h"
#include "defs.h"
#include "StrOp.h"

#include "TriangularTerm.h"
#include "ShoulderTerm.h"
#include <iosfwd>

#include "FuzzyException.h"

namespace fl {

    FuzzyEngine::FuzzyEngine() : _name(""),
    _fuzzy_op(&FuzzyOperator::DefaultFuzzyOperator()),
    _hedge_set(new HedgeSet) {

    }

    FuzzyEngine::FuzzyEngine(const std::string& name) : _name(name),
    _fuzzy_op(&FuzzyOperator::DefaultFuzzyOperator()),
    _hedge_set(new HedgeSet) {

    }

    FuzzyEngine::FuzzyEngine(const std::string& name, FuzzyOperator& fuzzy_operator) : _name(name),
    _fuzzy_op(&fuzzy_operator),
    _hedge_set(new HedgeSet) {

    }

    FuzzyEngine::~FuzzyEngine() {
        reset();
        delete _hedge_set;
    }

    void FuzzyEngine::setName(const std::string& name) {
        this->_name = name;
    }

    std::string FuzzyEngine::name() const {
        return this->_name;
    }

    void FuzzyEngine::process(bool clear) {
        if (clear) {
            for (int i = 0; i < numberOfOutputLVars(); ++i) {
                outputLVar(i)->output().clear();
            }
        }
        for (int i = 0; i < numberOfRuleBlocks(); ++i) {
            ruleBlock(i)->fireRules();
        }
    }

    void FuzzyEngine::process(int ruleblock, bool clear) {
        if (clear) {
            for (int i = 0; i < numberOfOutputLVars(); ++i) {
                outputLVar(i)->output().clear();
            }
        }
        ruleBlock(ruleblock)->fireRules();
    }

    void FuzzyEngine::reset() {
        for (int i = numberOfRuleBlocks() - 1; i >= 0; --i) {
            delete removeRuleBlock(i);
        }
        for (int i = numberOfInputLVars() - 1; i >= 0; --i) {
            delete removeInputLVar(i);
        }
        for (int i = numberOfOutputLVars() - 1; i >= 0; --i) {
            delete removeOutputLVar(i);
        }
        for (int i = hedgeSet().numberOfHedges() - 1; i >= 0; --i) {
            delete hedgeSet().remove(i);
        }

    }

    void FuzzyEngine::addInputLVar(InputLVar* lvar) {
        _input_lvars.push_back(lvar);
    }

    InputLVar* FuzzyEngine::inputLVar(int index) const {
        return _input_lvars[index];
    }

    InputLVar* FuzzyEngine::inputLVar(const std::string& name) const {
        for (int i = 0; i < numberOfInputLVars(); ++i) {
            if (inputLVar(i)->name() == name) {
                return inputLVar(i);
            }
        }
        return NULL;
    }

    InputLVar* FuzzyEngine::removeInputLVar(int index) {
        InputLVar* result = inputLVar(index);
        _input_lvars.erase(_input_lvars.begin() + index);
        return result;
    }

    InputLVar* FuzzyEngine::removeInputLVar(const std::string& name) {
        int index = indexOfInputLVar(name);
        return index == -1 ? NULL : removeInputLVar(index);
    }

    int FuzzyEngine::indexOfInputLVar(const std::string& name) const {
        for (int i = 0; i < numberOfInputLVars(); ++i) {
            if (inputLVar(i)->name() == name) {
                return i;
            }
        }
        return -1;
    }

    int FuzzyEngine::numberOfInputLVars() const {
        return _input_lvars.size();
    }

    void FuzzyEngine::addOutputLVar(OutputLVar* lvar) {
        _output_lvars.push_back(lvar);
    }

    OutputLVar* FuzzyEngine::outputLVar(int index) const {
        return _output_lvars[index];
    }

    OutputLVar* FuzzyEngine::outputLVar(const std::string& name) const {
        for (int i = 0; i < numberOfOutputLVars(); ++i) {
            if (outputLVar(i)->name() == name) {
                return outputLVar(i);
            }
        }
        return NULL;
    }

    OutputLVar* FuzzyEngine::removeOutputLVar(int index) {
        OutputLVar* result = outputLVar(index);
        _output_lvars.erase(_output_lvars.begin() + index);
        return result;
    }

    OutputLVar* FuzzyEngine::removeOutputLVar(const std::string& name) {
        int index = indexOfOutputLVar(name);
        return index == -1 ? NULL : removeOutputLVar(index);
    }

    int FuzzyEngine::indexOfOutputLVar(const std::string& name) const {
        for (int i = 0; i < numberOfOutputLVars(); ++i) {
            if (outputLVar(i)->name() == name) {
                return i;
            }
        }
        return -1;
    }

    int FuzzyEngine::numberOfOutputLVars() const {
        return _output_lvars.size();
    }

    HedgeSet& FuzzyEngine::hedgeSet() const {
        return *this->_hedge_set;
    }

    void FuzzyEngine::setFuzzyOperator(FuzzyOperator& fuzzy_operator) {
        this->_fuzzy_op = &fuzzy_operator;
    }

    FL_INLINE FuzzyOperator& FuzzyEngine::fuzzyOperator() const {
        return *this->_fuzzy_op;
    }

    //RULES

    void FuzzyEngine::addRuleBlock(RuleBlock* ruleblock) {
        this->_rule_blocks.push_back(ruleblock);
    }

    RuleBlock* FuzzyEngine::removeRuleBlock(int index) {
        RuleBlock* result = ruleBlock(index);
        this->_rule_blocks.erase(this->_rule_blocks.begin() + index);
        return result;
    }

    RuleBlock* FuzzyEngine::ruleBlock(int index) const {
        return this->_rule_blocks[index];
    }

    RuleBlock* FuzzyEngine::ruleBlock(const std::string& name) const {
        for (int i = 0; i < numberOfRuleBlocks(); ++i) {
            if (ruleBlock(i)->name() == name) {
                return ruleBlock(i);
            }
        }
        return NULL;
    }

    int FuzzyEngine::numberOfRuleBlocks() const {
        return this->_rule_blocks.size();
    }

    FL_INLINE void FuzzyEngine::setInput(const std::string& input_lvar, flScalar value) {
        InputLVar* input = inputLVar(input_lvar);
        if (!input) {
            throw InvalidArgumentException(FL_AT, "Input variable <" + input_lvar + "> not registered in fuzzy engine");
        }
        input->setInput(value);
    }

    FL_INLINE flScalar FuzzyEngine::output(const std::string& output_lvar) const {
        OutputLVar* output = outputLVar(output_lvar);
        if (!output) {
            throw InvalidArgumentException(FL_AT, "Output variable <" + output_lvar + "> not registered in fuzzy engine");
        }
        return output->output().defuzzify();
    }

    std::string FuzzyEngine::fuzzyOutput(const std::string& output_lvar) const {
        flScalar defuzzified = output(output_lvar);
        OutputLVar* output = outputLVar(output_lvar);
        if (!output) {
            throw InvalidArgumentException(FL_AT, "Output variable <" + output_lvar + "> not registered in fuzzy engine");
        }
        return output->fuzzify(defuzzified);
    }

    //    void FuzzyEngine::fromString(const std::string& fcl) {
    //
    //        enum eState {
    //            WAITING = 0, VAR_INPUT, FUZZIFY, DEFUZZIFY, RULE_BLOCK,
    //        };
    //        eState state = WAITING;
    //        std::string line;
    //        std::string token;
    //        std::istringstream is(fcl);
    //        std::string current_name;
    //        while (!is.eof()) {
    //            std::getline(is, line);
    //            StrOp::FindReplace(line, "(", " ( ");
    //            StrOp::FindReplace(line, ")", " ) ");
    //            while (StrOp::FindReplace(line, "  ", " ") > 0) {
    //                //do nothing;
    //            }
    //            std::vector<std::string> tokens = StrOp::Tokenize(line, " ");
    //            if (tokens.size() == 0) continue;
    //            InputLVar* input = NULL;
    //            OutputLVar* output = NULL;
    //            switch (state) {
    //                case WAITING:
    //                    current_name = tokens.size() > 0 ? tokens[1] : "";
    //                    if (tokens[0] == "FUNCTION_BLOCK") setName(current_name);
    //                    else if (tokens[0] == "VAR_INPUT") state = VAR_INPUT;
    //                    else if (tokens[0] == "FUZZIFY") state = FUZZIFY;
    //                    else if (tokens[0] == "DEFUZZIFY") state = DEFUZZIFY;
    //                    else if (tokens[0] == "RULEBLOCK") state = RULE_BLOCK;
    //                    break;
    //                case VAR_INPUT:
    //                    if (tokens[0] == "END_VAR") state = WAITING;
    //                    else addInputLVar(new InputLVar(tokens[0]));
    //                    break;
    //                case FUZZIFY:
    //                    if (tokens[0] == "TERM") {
    //                        input = inputLVar(current_name);
    //                        ParsingException::Assert(FL_AT, input, "Input <" << current_name << "> not registered");
    //
    //                    } else if (tokens[0] == "END_FUZZIFY") state = WAITING;
    //                    break;
    //                case DEFUZZIFY:
    //                case RULE_BLOCK:
    //            }
    //        }
    //    }

    std::string FuzzyEngine::toString() const {
        std::stringstream ss;
        ss << "FUNCTION_BLOCK " << name() << "\n\n";
        ss << "VAR_INPUT\n";
        for (int i = 0; i < numberOfInputLVars(); ++i) {
            ss << inputLVar(i)->name() << ": REAL;\n";
        }
        ss << "END_VAR\n\n";

        for (int i = 0; i < numberOfInputLVars(); ++i) {
            ss << "FUZZIFY " << inputLVar(i)->name() << "\n";
            for (int j = 0; j < inputLVar(i)->numberOfTerms(); ++j) {
                ss << inputLVar(i)->term(j)->toString() << ";\n";
            }
            ss << "END_FUZZIFY\n\n";
        }

        ss << "VAR_OUTPUT\n";
        for (int i = 0; i < numberOfOutputLVars(); ++i) {
            ss << outputLVar(i)->name() << ": REAL\n";
        }
        ss << "END_VAR\n\n";

        for (int i = 0; i < numberOfOutputLVars(); ++i) {
            ss << "DEFUZZIFY " << outputLVar(i)->name() << "\n";
            for (int j = 0; j < outputLVar(i)->numberOfTerms(); ++j) {
                ss << outputLVar(i)->term(j)->toString() << ";\n";
            }
            ss << "END_DEFUZZIFY\n\n";
        }

        for (int i = 0; i < numberOfRuleBlocks(); ++i) {
            ss << ruleBlock(i)->toString() << "\n\n";
        }

        ss << "END_FUNCTION_BLOCK";
        return ss.str();
    }


}
