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

#include "fl/imex/FclImporter.h"

#include "fl/Headers.h"

#include <iostream>
#include <sstream>

namespace fl {

    FclImporter::FclImporter() : Importer() {
    }

    FclImporter::~FclImporter() {
    }

    std::string FclImporter::name() const {
        return "FclImporter";
    }

    Engine* FclImporter::fromString(const std::string& fcl) const {
        FL_unique_ptr<Engine> engine(new Engine);

        std::map<std::string, std::string> tags;
        tags["VAR_INPUT"] = "END_VAR";
        tags["VAR_OUTPUT"] = "END_VAR";
        tags["FUZZIFY"] = "END_FUZZIFY";
        tags["DEFUZZIFY"] = "END_DEFUZZIFY";
        tags["RULEBLOCK"] = "END_RULEBLOCK";
        std::map<std::string, std::string>::const_iterator tagFinder;

        std::string currentTag = "", closingTag = "";
        std::ostringstream block;
        std::istringstream fclReader(fcl);
        std::string line;

        int lineNumber = 0;
        while (std::getline(fclReader, line)) {
            ++lineNumber;
            std::vector<std::string> comments;
            comments = Op::split(line, "//");
            if (comments.size() > 1) {
                line = comments.front();
            }
            comments = Op::split(line, "#");
            if (comments.size() > 1) {
                line = comments.front();
            }
            line = Op::trim(line);
            if (line.empty() or line.at(0) == '%' or line.at(0) == '#'
                    or (line.substr(0, 2) == "//")) {
                continue;
            }
            line = fl::Op::findReplace(line, ";", "");
            std::istringstream tokenizer(line);
            std::string firstToken;
            tokenizer >> firstToken;

            if (firstToken == "FUNCTION_BLOCK") {
                if (tokenizer.rdbuf()->in_avail() > 0) {
                    std::ostringstream name;
                    std::string token;
                    tokenizer >> token;
                    name << token;
                    while (tokenizer >> token) {
                        name << " " << token;
                    }
                    engine->setName(name.str());
                }
                continue;
            }
            if (firstToken == "END_FUNCTION_BLOCK") {
                break;
            }

            if (currentTag.empty()) {
                tagFinder = tags.find(firstToken);
                if (tagFinder == tags.end()) {
                    std::ostringstream ex;
                    ex << "[syntax error] unknown block definition <" << firstToken
                            << "> " << " in line " << lineNumber << ": " << line;
                    throw fl::Exception(ex.str(), FL_AT);
                }
                currentTag = tagFinder->first;
                closingTag = tagFinder->second;
                block.str("");
                block.clear();
                block << line << "\n";
                continue;
            }

            if (not currentTag.empty()) {
                if (firstToken == closingTag) {
                    processBlock(currentTag, block.str(), engine.get());
                    currentTag = "";
                    closingTag = "";
                } else if (tags.find(firstToken) != tags.end()) {
                    //if opening new block without closing the previous one
                    std::ostringstream ex;
                    ex << "[syntax error] expected <" << closingTag << "> before <"
                            << firstToken << "> in line: " << line;
                    throw fl::Exception(ex.str(), FL_AT);
                } else {
                    block << line << "\n";
                }
                continue;
            }
        }

        if (not currentTag.empty()) {
            std::ostringstream ex;
            ex << "[syntax error] ";
            if (block.rdbuf()->in_avail() > 0) {
                ex << "expected <" << closingTag << "> for block:\n" << block.str();
            } else {
                ex << "expected <" << closingTag << ">, but not found";
            }
            throw fl::Exception(ex.str(), FL_AT);
        }
        return engine.release();
    }

    void FclImporter::processBlock(const std::string& tag, const std::string& block, Engine* engine) const {
        if (tag == "VAR_INPUT" or tag == "VAR_OUTPUT") {
            processVar(tag, block, engine);
        } else if (tag == "FUZZIFY") {
            processFuzzify(block, engine);
        } else if (tag == "DEFUZZIFY") {
            processDefuzzify(block, engine);
        } else if (tag == "RULEBLOCK") {
            processRuleBlock(block, engine);
        } else {
            std::ostringstream ex;
            ex << "[syntax error] unexpected tag <" << tag << "> for block:\n" << block;
            throw fl::Exception(ex.str(), FL_AT);
        }
    }

    void FclImporter::processVar(const std::string& tag, const std::string& block, Engine* engine)const {
        std::istringstream blockReader(block);
        std::string line;

        std::getline(blockReader, line); //discard first line as it is VAR_INPUT
        while (std::getline(blockReader, line)) {
            std::vector<std::string> token = Op::split(line, ":");
            if (token.size() != 2) {
                std::ostringstream ex;
                ex << "[syntax error] expected property of type (key : value) in line: " << line;
                throw fl::Exception(ex.str(), FL_AT);
            }
            std::string name = fl::Op::validName(token.at(0));
            if (tag == "VAR_INPUT")
                engine->addInputVariable(new InputVariable(name));
            else if (tag == "VAR_OUTPUT")
                engine->addOutputVariable(new OutputVariable(name));
            else {
                std::ostringstream ex;
                ex << "[syntax error] unexpected tag <" << tag << "> in line: " << line;
                throw fl::Exception(ex.str(), FL_AT);
            }
        }
    }

    void FclImporter::processFuzzify(const std::string& block, Engine* engine)const {
        std::istringstream blockReader(block);
        std::string line;

        std::getline(blockReader, line);
        std::string name;
        std::size_t index = line.find_first_of(' ');
        if (index != std::string::npos) {
            name = fl::Op::validName(line.substr(index + 1));
        } else {
            std::ostringstream ex;
            ex << "[syntax error] expected name of input variable in line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }
        if (not engine->hasInputVariable(name)) {
            std::ostringstream ex;
            ex << "[syntax error] engine does not contain "
                    "input variable <" << name << "> from line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        InputVariable* inputVariable = engine->getInputVariable(name);
        while (std::getline(blockReader, line)) {
            std::istringstream ss(line);
            std::string firstToken;
            ss >> firstToken;
            try {
                if (firstToken == "RANGE") {
                    std::pair<scalar, scalar> minmax = parseRange(line);
                    inputVariable->setMinimum(minmax.first);
                    inputVariable->setMaximum(minmax.second);
                } else if (firstToken == "ENABLED") {
                    inputVariable->setEnabled(parseEnabled(line));
                } else if (firstToken == "TERM") {
                    inputVariable->addTerm(parseTerm(line, engine));
                } else throw fl::Exception("[syntax error] unexpected token "
                        "<" + firstToken + ">" + line, FL_AT);
            } catch (fl::Exception& ex) {
                ex.append("At line: <" + line + ">");
                throw;
            }
        }

    }

    void FclImporter::processDefuzzify(const std::string& block, Engine* engine) const {
        std::istringstream blockReader(block);
        std::string line;

        std::getline(blockReader, line);
        std::string name;
        std::size_t index = line.find_first_of(' ');
        if (index != std::string::npos) {
            name = fl::Op::validName(line.substr(index + 1));
        } else {
            std::ostringstream ex;
            ex << "[syntax error] expected an output variable name in line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }
        if (not engine->hasOutputVariable(name)) {
            std::ostringstream ex;
            ex << "[syntax error] output variable <" << name
                    << "> not registered in engine. "
                    << "Line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        OutputVariable* outputVariable = engine->getOutputVariable(name);
        while (std::getline(blockReader, line)) {
            line = fl::Op::trim(line);
            std::istringstream tokenizer(line);
            std::string firstToken;
            tokenizer >> firstToken;
            if (firstToken == "TERM") {
                outputVariable->addTerm(parseTerm(line, engine));
            } else if (firstToken == "METHOD") {
                outputVariable->setDefuzzifier(parseDefuzzifier(line));
            } else if (firstToken == "ACCU") {
                outputVariable->fuzzyOutput()->setAccumulation(parseSNorm(line));
            } else if (firstToken == "DEFAULT") {
                std::pair<scalar, bool> defaultAndLock = parseDefaultValue(line);
                outputVariable->setDefaultValue(defaultAndLock.first);
                outputVariable->setLockPreviousOutputValue(defaultAndLock.second or
                        outputVariable->isLockedPreviousOutputValue());
            } else if (firstToken == "RANGE") {
                std::pair<scalar, scalar> minmax = parseRange(line);
                outputVariable->setMinimum(minmax.first);
                outputVariable->setMaximum(minmax.second);
            } else if (firstToken == "LOCK") {
                std::pair<bool, bool> output_range = parseLocks(line);
                outputVariable->setLockPreviousOutputValue(output_range.first);
                outputVariable->setLockOutputValueInRange(output_range.second);
            } else if (firstToken == "ENABLED") {
                outputVariable->setEnabled(parseEnabled(line));
            } else {
                std::ostringstream ex;
                ex << "[syntax error] unexpected token <" << firstToken <<
                        "> in line: " << line;
                throw fl::Exception(ex.str(), FL_AT);
            }
        }

    }

    void FclImporter::processRuleBlock(const std::string& block, Engine* engine)const {
        std::istringstream blockReader(block);
        std::string line;

        std::string name;
        std::getline(blockReader, line);
        std::size_t index = line.find_last_of(' ');
        if (index != std::string::npos) name = line.substr(index);
        RuleBlock * ruleblock = new RuleBlock(name);
        engine->addRuleBlock(ruleblock);

        while (std::getline(blockReader, line)) {
            std::string firstToken = line.substr(0, line.find_first_of(' '));
            if (firstToken == "AND") {
                ruleblock->setConjunction(parseTNorm(line));
            } else if (firstToken == "OR") {
                ruleblock->setDisjunction(parseSNorm(line));
            } else if (firstToken == "ACT") {
                ruleblock->setActivation(parseTNorm(line));
            } else if (firstToken == "ENABLED") {
                ruleblock->setEnabled(parseEnabled(line));
            } else if (firstToken == "RULE") {
                std::size_t ruleStart = line.find_first_of(':');
                if (ruleStart == std::string::npos) ruleStart = 4; // "RULE".size()
                std::string ruleText = line.substr(ruleStart + 1);
                ruleText = fl::Op::trim(ruleText);
                Rule* rule = new Rule(ruleText);
                try {
                    rule->load(engine);
                } catch (...) {
                    //ignore
                }
                ruleblock->addRule(rule);
            } else {
                std::ostringstream ex;
                ex << "[syntax error] keyword <" << firstToken
                        << "> not recognized in line: " << line;
                throw fl::Exception(ex.str(), FL_AT);
            }
        }
    }

    TNorm* FclImporter::parseTNorm(const std::string& line) const {
        std::vector<std::string> token = Op::split(line, ":");
        if (token.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type (key : value) in line: "
                    << line;
            throw fl::Exception(ex.str(), FL_AT);
        }
        std::string name = Op::trim(token.at(1));
        std::string className = name;
        if (name == "NONE") className = "";
        else if (name == "MIN") className = Minimum().className();
        else if (name == "PROD") className = AlgebraicProduct().className();
        else if (name == "BDIF") className = BoundedDifference().className();
        else if (name == "DPROD") className = DrasticProduct().className();
        else if (name == "EPROD") className = EinsteinProduct().className();
        else if (name == "HPROD") className = HamacherProduct().className();
        else if (name == "NMIN") className = NilpotentMinimum().className();

        return FactoryManager::instance()->tnorm()->constructObject(className);
    }

    SNorm* FclImporter::parseSNorm(const std::string& line) const {
        std::vector<std::string> token = Op::split(line, ":");
        if (token.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type (key : value) in line: "
                    << line;
            throw fl::Exception(ex.str(), FL_AT);
        }
        std::string name = Op::trim(token.at(1));
        std::string className = name;
        if (name == "NONE") className = "";
        else if (name == "MAX") className = Maximum().className();
        else if (name == "ASUM") className = AlgebraicSum().className();
        else if (name == "BSUM") className = BoundedSum().className();
        else if (name == "NSUM") className = NormalizedSum().className();
        else if (name == "DSUM") className = DrasticSum().className();
        else if (name == "ESUM") className = EinsteinSum().className();
        else if (name == "HSUM") className = HamacherSum().className();
        else if (name == "NMAX") className = NilpotentMaximum().className();

        return FactoryManager::instance()->snorm()->constructObject(className);
    }

    Term* FclImporter::parseTerm(const std::string& line, const Engine* engine) const {
        std::ostringstream spacer;
        for (std::size_t i = 0; i < line.size(); ++i) {
            if (line.at(i) == '(' or line.at(i) == ')' or line.at(i) == ',') {
                spacer << " " << line.at(i) << " ";
            } else if (line.at(i) == ':') {
                spacer << " :";
            } else if (line.at(i) == '=') {
                spacer << "= ";
            } else
                spacer << line.at(i);
        }
        std::string spacedLine = spacer.str();

        enum FSM {
            S_KWTERM, S_NAME, S_ASSIGN, S_TERMCLASS, S_PARAMETERS
        };
        FSM state = S_KWTERM;
        std::istringstream tokenizer(spacedLine);
        std::string token;
        std::string name, termClass;
        std::vector<std::string> parameters;
        while (tokenizer >> token) {
            if (state == S_KWTERM and token == "TERM") {
                state = S_NAME;
                continue;
            }
            if (state == S_NAME) {
                name = token;
                state = S_ASSIGN;
                continue;
            }
            if (state == S_ASSIGN and token == ":=") {
                state = S_TERMCLASS;
                continue;
            }
            if (state == S_TERMCLASS) {
                if (fl::Op::isNumeric(token)) {
                    termClass = Constant().className();
                    parameters.push_back(token);
                } else if (token == "(") {
                    termClass = Discrete().className();
                } else {
                    termClass = token;
                }
                state = S_PARAMETERS;
                continue;
            }
            if (state == S_PARAMETERS) {
                if (termClass != Function().className() and
                        (token == "(" or token == ")" or token == ",")) {
                    continue;
                }
                if (token == ";") break;
                parameters.push_back(fl::Op::trim(token));
            }
        }
        if (state <= S_ASSIGN)
            throw fl::Exception("[syntax error] malformed term in line: " + line, FL_AT);

        FL_unique_ptr<Term> term;
        term.reset(FactoryManager::instance()->term()->constructObject(termClass));
        Term::updateReference(term.get(), engine);
        term->setName(fl::Op::validName(name));
        std::string separator;
        if (not dynamic_cast<Function*> (term.get())) {
            separator = " ";
        }
        term->configure(Op::join(parameters, separator)); //remove spaces for text of function
        return term.release();
    }

    Defuzzifier* FclImporter::parseDefuzzifier(const std::string& line) const {
        std::vector<std::string> token = Op::split(line, ":");
        if (token.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type (key : value) in "
                    << "line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        std::string name = fl::Op::trim(token.at(1));
        std::string className = name;
        if (name == "NONE") className = "";
        else if (name == "COG") className = Centroid().className();
        else if (name == "COA") className = Bisector().className();
        else if (name == "LM") className = SmallestOfMaximum().className();
        else if (name == "RM") className = LargestOfMaximum().className();
        else if (name == "MM") className = MeanOfMaximum().className();
        else if (name == "COGS") className = WeightedAverage().className();
        else if (name == "COGSS") className = WeightedSum().className();

        return FactoryManager::instance()->defuzzifier()->constructObject(className);
    }

    std::pair<scalar, bool> FclImporter::parseDefaultValue(const std::string& line) const {
        std::vector<std::string> token = Op::split(line, ":=");
        if (token.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type (key := value) in line: "
                    << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        std::vector<std::string> values = Op::split(token.at(1), "|");

        std::string defaultValue = values.front();
        std::string nc;
        if (values.size() == 2) nc = values.back();

        defaultValue = fl::Op::trim(defaultValue);
        nc = fl::Op::trim(nc);

        scalar value;
        try {
            value = fl::Op::toScalar(defaultValue);
        } catch (...) {
            std::ostringstream ex;
            ex << "[syntax error] expected numeric value, "
                    << "but found <" << defaultValue << "> in line: "
                    << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        bool lockPreviousOutput = (nc == "NC");

        if (not (lockPreviousOutput or nc.empty())) {
            throw fl::Exception("[syntax error] expected keyword <NC>, "
                    "but found <" + nc + "> in line: " + line, FL_AT);
        }

        return std::pair<scalar, bool>(value, lockPreviousOutput);
    }

    std::pair<scalar, scalar> FclImporter::parseRange(const std::string& line) const {
        std::vector<std::string> token = Op::split(line, ":=");
        if (token.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type (key := value) in line: "
                    << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        std::string rangeToken = token.at(1);

        std::ostringstream range;
        for (std::size_t i = 0; i < rangeToken.size(); ++i) {
            char character = rangeToken.at(i);
            if (character == '(' or character == ')' or character == ' ' or character == ';')
                continue;
            range << character;
        }
        token = Op::split(range.str(), "..");
        if (token.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type 'start .. end', "
                    << "but found <" << range.str() << "> in line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }
        scalar minimum, maximum;
        int index;
        try {
            minimum = Op::toScalar(token.at(index = 0));
            maximum = Op::toScalar(token.at(index = 1));
        } catch (std::exception& ex) {
            (void) ex;
            std::ostringstream ss;
            ss << "[syntax error] expected numeric value, but found <" << token.at(index) << "> in "
                    << "line: " << line;
            throw fl::Exception(ss.str(), FL_AT);
        }
        return std::pair<scalar, scalar>(minimum, maximum);
    }

    std::pair<bool, bool> FclImporter::parseLocks(const std::string& line) const {
        std::size_t index = line.find_first_of(":");
        if (index == std::string::npos) {
            throw fl::Exception("[syntax error] expected property of type "
                    "'key : value' in line: " + line, FL_AT);
        }
        bool output, range;
        std::string value = line.substr(index + 1);
        std::vector<std::string> flags = fl::Op::split(value, "|");
        if (flags.size() == 1) {
            std::string flag = fl::Op::trim(flags.front());
            output = (flag == "PREVIOUS");
            range = (flag == "RANGE");
            if (not (output or range)) {
                throw fl::Exception("[syntax error] expected locking flags "
                        "<PREVIOUS|RANGE>, but found <" + flag + "> in line: " + line, FL_AT);
            }
        } else if (flags.size() == 2) {
            std::string flagA = fl::Op::trim(flags.front());
            std::string flagB = fl::Op::trim(flags.back());
            output = (flagA == "PREVIOUS" or flagB == "PREVIOUS");
            range = (flagA == "RANGE" or flagB == "RANGE");
            if (not (output and range)) {
                throw fl::Exception("[syntax error] expected locking flags "
                        "<PREVIOUS|RANGE>, but found "
                        "<" + flags.front() + "|" + flags.back() + "> in line: " + line, FL_AT);
            }
        } else {
            throw fl::Exception("[syntax error] expected locking flags "
                    "<PREVIOUS|RANGE>, but found "
                    "<" + value + "> in line: " + line, FL_AT);
        }
        return std::pair<bool, bool>(output, range);
    }

    bool FclImporter::parseEnabled(const std::string& line) const {
        std::vector<std::string> tokens = Op::split(line, ":");
        if (tokens.size() != 2) {
            std::ostringstream ex;
            ex << "[syntax error] expected property of type (key : value) in "
                    << "line: " << line;
            throw fl::Exception(ex.str(), FL_AT);
        }

        std::string boolean = fl::Op::trim(tokens.at(1));
        if (boolean == "TRUE") return true;
        if (boolean == "FALSE") return false;
        throw fl::Exception("[syntax error] expected boolean <TRUE|FALSE>, but found <" + line + ">", FL_AT);
    }

    FclImporter* FclImporter::clone() const {
        return new FclImporter(*this);
    }


}
