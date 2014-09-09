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

#ifndef FL_ENGINE_H
#define FL_ENGINE_H

#include "fl/fuzzylite.h"

#include "fl/defuzzifier/IntegralDefuzzifier.h"

#include <string>
#include <vector>

namespace fl {

    class InputVariable;
    class OutputVariable;
    class Variable;
    class RuleBlock;
    class Hedge;
    class TNorm;
    class SNorm;
    class Defuzzifier;

    class FL_API Engine {
    private:
        void copyFrom(const Engine& source);
    protected:
        std::string _name;
        std::vector<InputVariable*> _inputVariables;
        std::vector<OutputVariable*> _outputVariables;
        std::vector<RuleBlock*> _ruleblocks;

        void updateReferences() const;

    public:
        Engine(const std::string& name = "");
        Engine(const Engine& other);
        Engine& operator=(const Engine& other);
        virtual ~Engine();
        FL_DEFAULT_MOVE(Engine)

        virtual void configure(const std::string& conjunctionT,
                const std::string& disjunctionS,
                const std::string& activationT,
                const std::string& accumulationS,
                const std::string& defuzzifier,
                int resolution = IntegralDefuzzifier::defaultResolution());

        virtual void configure(TNorm* conjunction, SNorm* disjunction,
                TNorm* activation, SNorm* accumulation,
                Defuzzifier* defuzzifier);

        virtual bool isReady(std::string* status = fl::null) const;

        virtual void process();

        virtual void restart();

        virtual void setName(const std::string& name);
        virtual std::string getName() const;

        virtual void setInputValue(const std::string& name, scalar value);
        virtual scalar getOutputValue(const std::string& name);


        virtual std::string toString() const;

        enum Type {
            Mamdani, Larsen, TakagiSugeno,
            Tsukamoto, InverseTsukamoto, Hybrid, Unknown
        };
        virtual Type type(std::string* name = fl::null, std::string* reason = fl::null) const;

        virtual Engine* clone() const;

        virtual std::vector<Variable*> variables() const;

        /**
         * Operations for iterable datatype _inputVariables
         */
        virtual void addInputVariable(InputVariable* inputVariable);
        virtual InputVariable* setInputVariable(InputVariable* inputVariable, int index);
        virtual void insertInputVariable(InputVariable* inputVariable, int index);
        virtual InputVariable* getInputVariable(int index) const;
        virtual InputVariable* getInputVariable(const std::string& name) const;
        virtual InputVariable* removeInputVariable(int index);
        virtual InputVariable* removeInputVariable(const std::string& name);
        virtual bool hasInputVariable(const std::string& name) const;
        virtual int numberOfInputVariables() const;
        virtual const std::vector<InputVariable*>& inputVariables() const;
        virtual void setInputVariables(const std::vector<InputVariable*>& inputVariables);
        virtual std::vector<InputVariable*>& inputVariables();

        /**
         * Operations for iterable datatype _outputVariables
         */
        virtual void addOutputVariable(OutputVariable* outputVariable);
        virtual OutputVariable* setOutputVariable(OutputVariable* outputVariable, int index);
        virtual void insertOutputVariable(OutputVariable* outputVariable, int index);
        virtual OutputVariable* getOutputVariable(int index) const;
        virtual OutputVariable* getOutputVariable(const std::string& name) const;
        virtual bool hasOutputVariable(const std::string& name) const;
        virtual OutputVariable* removeOutputVariable(int index);
        virtual OutputVariable* removeOutputVariable(const std::string& name);
        virtual int numberOfOutputVariables() const;
        virtual const std::vector<OutputVariable*>& outputVariables() const;
        virtual void setOutputVariables(const std::vector<OutputVariable*>& outputVariables);
        virtual std::vector<OutputVariable*>& outputVariables();

        /**
         * Operations for iterable datatype _ruleblocks
         */
        virtual void addRuleBlock(RuleBlock* ruleblock);
        virtual RuleBlock* setRuleBlock(RuleBlock* ruleBlock, int index);
        virtual void insertRuleBlock(RuleBlock* ruleblock, int index);
        virtual RuleBlock* getRuleBlock(int index) const;
        virtual RuleBlock* getRuleBlock(const std::string& name) const;
        virtual bool hasRuleBlock(const std::string& name) const;
        virtual RuleBlock* removeRuleBlock(int index);
        virtual RuleBlock* removeRuleBlock(const std::string& name);
        virtual int numberOfRuleBlocks() const;
        virtual const std::vector<RuleBlock*>& ruleBlocks() const;
        virtual void setRuleBlocks(const std::vector<RuleBlock*>& ruleBlocks);
        virtual std::vector<RuleBlock*>& ruleBlocks();

    };

}
#endif /* FL_ENGINE_H */
