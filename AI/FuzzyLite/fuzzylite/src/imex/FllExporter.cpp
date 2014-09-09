/*
 Author: Juan Rada-Vilela, Ph.D.
 Copyright (C) 2010-2014 FuzzyLite Limited
 All rights reserved

 This file is part of fuzzylite.

 fuzzylite is free software: you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation, either version 3 of the License, or (at your option)
 any later version.

 fuzzylite is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with fuzzylite.  If not, see <http://www.gnu.org/licenses/>.

 fuzzylite™ is a trademark of FuzzyLite Limited.

 */

#include "fl/imex/FllExporter.h"

#include "fl/Headers.h"

namespace fl {

    FllExporter::FllExporter(const std::string& indent, const std::string& separator)
    : Exporter(), _indent(indent), _separator(separator) {
    }

    FllExporter::~FllExporter() {
    }

    std::string FllExporter::name() const {
        return "FllExporter";
    }

    void FllExporter::setIndent(const std::string& indent) {
        this->_indent = indent;
    }

    std::string FllExporter::getIndent() const {
        return this->_indent;
    }

    void FllExporter::setSeparator(const std::string& separator) {
        this->_separator = separator;
    }

    std::string FllExporter::getSeparator() const {
        return this->_separator;
    }

    std::string FllExporter::toString(const Engine* engine) const {
        std::vector<std::string> result;
        result.push_back("Engine: " + engine->getName());
        result.push_back(toString(engine->inputVariables()));
        result.push_back(toString(engine->outputVariables()));
        result.push_back(toString(engine->ruleBlocks()));
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const std::vector<Variable*>& variables) const {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < variables.size(); ++i) {
            result.push_back(toString(variables.at(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const std::vector<InputVariable*>& variables) const {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < variables.size(); ++i) {
            result.push_back(toString(variables.at(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const std::vector<OutputVariable*>& variables) const {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < variables.size(); ++i) {
            result.push_back(toString(variables.at(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const std::vector<RuleBlock*>& ruleBlocks) const {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < ruleBlocks.size(); ++i) {
            result.push_back(toString(ruleBlocks.at(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const Variable* variable) const {
        std::vector<std::string> result;
        result.push_back("Variable: " + Op::validName(variable->getName()));
        result.push_back(_indent + "enabled: " + (variable->isEnabled() ? "true" : "false"));
        result.push_back(_indent + "range: " + Op::join(2, " ",
                variable->getMinimum(), variable->getMaximum()));
        for (int i = 0; i < variable->numberOfTerms(); ++i) {
            result.push_back(_indent + toString(variable->getTerm(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const InputVariable* inputVariable) const {
        std::vector<std::string> result;
        result.push_back("InputVariable: " + Op::validName(inputVariable->getName()));
        result.push_back(_indent + "enabled: " + (inputVariable->isEnabled() ? "true" : "false"));
        result.push_back(_indent + "range: " + Op::join(2, " ",
                inputVariable->getMinimum(), inputVariable->getMaximum()));
        for (int i = 0; i < inputVariable->numberOfTerms(); ++i) {
            result.push_back(_indent + toString(inputVariable->getTerm(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const OutputVariable* outputVariable) const {
        std::vector<std::string> result;
        result.push_back("OutputVariable: " + Op::validName(outputVariable->getName()));
        result.push_back(_indent + "enabled: " + (outputVariable->isEnabled() ? "true" : "false"));
        result.push_back(_indent + "range: " + Op::join(2, " ",
                outputVariable->getMinimum(), outputVariable->getMaximum()));
        result.push_back(_indent + "accumulation: " +
                toString(outputVariable->fuzzyOutput()->getAccumulation()));
        result.push_back(_indent + "defuzzifier: " +
                toString(outputVariable->getDefuzzifier()));
        result.push_back(_indent + "default: " + Op::str(outputVariable->getDefaultValue()));
        result.push_back(_indent + "lock-previous: " +
                (outputVariable->isLockedPreviousOutputValue() ? "true" : "false"));
        result.push_back(_indent + "lock-range: " +
                (outputVariable->isLockedOutputValueInRange() ? "true" : "false"));
        for (int i = 0; i < outputVariable->numberOfTerms(); ++i) {
            result.push_back(_indent + toString(outputVariable->getTerm(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const RuleBlock* ruleBlock) const {
        std::vector<std::string> result;
        result.push_back("RuleBlock: " + ruleBlock->getName());
        result.push_back(_indent + "enabled: " +
                (ruleBlock->isEnabled() ? "true" : "false"));
        result.push_back(_indent + "conjunction: " + toString(ruleBlock->getConjunction()));
        result.push_back(_indent + "disjunction: " + toString(ruleBlock->getDisjunction()));
        result.push_back(_indent + "activation: " + toString(ruleBlock->getActivation()));
        for (int i = 0; i < ruleBlock->numberOfRules(); ++i) {
            result.push_back(_indent + toString(ruleBlock->getRule(i)));
        }
        return Op::join(result, _separator);
    }

    std::string FllExporter::toString(const Rule* rule) const {
        return "rule: " + rule->getText();
    }

    std::string FllExporter::toString(const Term* term) const {
        return "term: " + Op::validName(term->getName()) + " " + term->className()
                + " " + term->parameters();
    }

    std::string FllExporter::toString(const Norm* norm) const {
        if (norm) return norm->className();
        return "none";
    }

    std::string FllExporter::toString(const Defuzzifier* defuzzifier) const {
        if (not defuzzifier) return "none";
        if (const IntegralDefuzzifier * integralDefuzzifier =
                dynamic_cast<const IntegralDefuzzifier*> (defuzzifier)) {
            return defuzzifier->className() + " " + Op::str<int>(integralDefuzzifier->getResolution());

        } else if (const WeightedDefuzzifier * weightedDefuzzifier =
                dynamic_cast<const WeightedDefuzzifier*> (defuzzifier)) {
            return weightedDefuzzifier->className() + " " + weightedDefuzzifier->getTypeName();
        }
        return defuzzifier->className();
    }

    FllExporter* FllExporter::clone() const {
        return new FllExporter(*this);
    }

}
