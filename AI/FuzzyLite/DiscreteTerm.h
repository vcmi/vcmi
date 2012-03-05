/* 
 * File:   DiscreteTerm.h
 * Author: jcrada
 *
 * Created on May 9, 2010, 8:17 PM
 */

#ifndef FL_DISCRETETERM_H
#define	FL_DISCRETETERM_H

#include "LinguisticTerm.h"

namespace fl {

    class DiscreteTerm : public LinguisticTerm {
    private:
        std::vector<flScalar> _x;
        std::vector<flScalar> _y;

    public:
        DiscreteTerm();
        DiscreteTerm(const std::string& name, const std::vector<flScalar>& x = std::vector<flScalar>(),
                const std::vector<flScalar>& y = std::vector<flScalar>());
        DiscreteTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                const std::vector<flScalar>& x = std::vector<flScalar>(),
                const std::vector<flScalar>& y = std::vector<flScalar>());
        virtual ~DiscreteTerm();

        virtual void setXY(int index, flScalar x, flScalar y);
        virtual void setXY(const std::vector<flScalar>&x, const std::vector<flScalar>&y);
        virtual void addXY(flScalar x, flScalar y);
        virtual void remove(int index);
        virtual void clear();
        
        virtual int numberOfCoords() const;

        virtual flScalar minimum() const;
        virtual flScalar maximum() const;

        virtual DiscreteTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;

        virtual eMembershipFunction type() const;

        virtual std::string toString() const;
    };
}

#endif	/* FL_DISCRETETERM_H */

