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

#ifndef FL_OUTPUTVARIABLE_H
#define FL_OUTPUTVARIABLE_H

#include "fl/variable/Variable.h"

namespace fl {
    class Accumulated;
    class Defuzzifier;

    class FL_API OutputVariable : public Variable {
    private:
        void copyFrom(const OutputVariable& other);
    protected:
        FL_unique_ptr<Accumulated> _fuzzyOutput;
        FL_unique_ptr<Defuzzifier> _defuzzifier;
        scalar _outputValue;
        scalar _previousOutputValue;
        scalar _defaultValue;
        bool _lockOutputValueInRange;
        bool _lockPreviousOutputValue;

    public:
        OutputVariable(const std::string& name = "",
                scalar minimum = -fl::inf, scalar maximum = fl::inf);
        OutputVariable(const OutputVariable& other);
        OutputVariable& operator=(const OutputVariable& other);
        virtual ~OutputVariable() FL_IOVERRIDE;
        FL_DEFAULT_MOVE(OutputVariable)

        virtual Accumulated* fuzzyOutput() const;

        virtual void setName(const std::string& name) FL_IOVERRIDE;

        virtual void setMinimum(scalar minimum) FL_IOVERRIDE;
        virtual void setMaximum(scalar maximum) FL_IOVERRIDE;

        virtual void setDefuzzifier(Defuzzifier* defuzzifier);
        virtual Defuzzifier* getDefuzzifier() const;

        virtual void setOutputValue(scalar outputValue);
        virtual scalar getOutputValue() const;

        virtual void setPreviousOutputValue(scalar defuzzifiedValue);
        virtual scalar getPreviousOutputValue() const;

        virtual void setDefaultValue(scalar defaultValue);
        virtual scalar getDefaultValue() const;

        virtual void setLockOutputValueInRange(bool lockOutputValueInRange);
        virtual bool isLockedOutputValueInRange() const;

        virtual void setLockPreviousOutputValue(bool lockPreviousOutputValue);
        virtual bool isLockedPreviousOutputValue() const;

        virtual void defuzzify();

        virtual std::string fuzzyOutputValue() const;

        virtual void clear();

        virtual std::string toString() const FL_IOVERRIDE;

    };

}
#endif /* FL_OUTPUTVARIABLE_H */
