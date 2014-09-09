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

#ifndef FL_FACTORYMANAGER_H
#define FL_FACTORYMANAGER_H

#include "fl/fuzzylite.h"

namespace fl {
    class TNormFactory;
    class SNormFactory;
    class DefuzzifierFactory;
    class TermFactory;
    class HedgeFactory;
    class FunctionFactory;

    class FL_API FactoryManager {
    protected:
        static FactoryManager _instance;

        FL_unique_ptr<TNormFactory> _tnorm;
        FL_unique_ptr<SNormFactory> _snorm;
        FL_unique_ptr<DefuzzifierFactory> _defuzzifier;
        FL_unique_ptr<TermFactory> _term;
        FL_unique_ptr<HedgeFactory> _hedge;
        FL_unique_ptr<FunctionFactory> _function;

        FactoryManager();
        FactoryManager(TNormFactory* tnorm, SNormFactory* snorm,
                DefuzzifierFactory* defuzzifier, TermFactory* term,
                HedgeFactory* hedge, FunctionFactory* function);
        FactoryManager(const FactoryManager& other);
        FactoryManager& operator=(const FactoryManager& other);
        FL_DEFAULT_MOVE(FactoryManager)
        virtual ~FactoryManager();

    public:
        static FactoryManager* instance();

        virtual void setTnorm(TNormFactory* tnorm);
        virtual TNormFactory* tnorm() const;

        virtual void setSnorm(SNormFactory* snorm);
        virtual SNormFactory* snorm() const;

        virtual void setDefuzzifier(DefuzzifierFactory* defuzzifier);
        virtual DefuzzifierFactory* defuzzifier() const;

        virtual void setTerm(TermFactory* term);
        virtual TermFactory* term() const;

        virtual void setHedge(HedgeFactory* hedge);
        virtual HedgeFactory* hedge() const;

        virtual void setFunction(FunctionFactory* function);
        virtual FunctionFactory* function() const;
    };
}
#endif  /* FL_FACTORYMANAGER_H */

