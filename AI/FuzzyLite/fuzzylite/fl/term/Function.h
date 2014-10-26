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

#ifndef FL_FUNCTION_H
#define FL_FUNCTION_H

#include "fl/term/Term.h"

#include <map>
#include <string>

namespace fl {

    class Engine;

    class FL_API Function : public Term {
        /****************************
         * Parsing Elements
         ****************************/

    public:
        typedef scalar(*Unary)(scalar);
        typedef scalar(*Binary)(scalar, scalar);

        struct FL_API Element {

            enum Type {
                OPERATOR, FUNCTION
            };
            std::string name;
            std::string description;
            Type type;
            Unary unary;
            Binary binary;
            int arity;
            int precedence; //Operator
            int associativity;
            Element(const std::string& name, const std::string& description, Type type);
            Element(const std::string& name, const std::string& description,
                    Type type, Unary unary, int precedence = 0, int associativity = -1);
            Element(const std::string& name, const std::string& description,
                    Type type, Binary binary, int precedence = 0, int associativity = -1);
            virtual ~Element();
            FL_DEFAULT_COPY_AND_MOVE(Element)

            virtual bool isOperator() const;
            virtual bool isFunction() const;

            virtual Element* clone() const;

            virtual std::string toString() const;

        };

        /**************************
         * Tree elements, wrap Elements into Nodes.
         **************************/

        struct FL_API Node {
            FL_unique_ptr<Element> element;
            FL_unique_ptr<Node> left;
            FL_unique_ptr<Node> right;
            std::string variable;
            scalar value;

            Node(Element* element, Node* left = fl::null, Node* right = fl::null);
            Node(const std::string& variable);
            Node(scalar value);
            Node(const Node& source);
            Node& operator=(const Node& rhs);
            virtual ~Node();
            FL_DEFAULT_MOVE(Node)

            virtual scalar evaluate(const std::map<std::string, scalar>*
                    variables = fl::null) const;

            virtual Node* clone() const;

            virtual std::string toString() const;
            virtual std::string toPrefix(const Node* node = fl::null) const;
            virtual std::string toInfix(const Node* node = fl::null) const;
            virtual std::string toPostfix(const Node* node = fl::null) const;
        private:
            void copyFrom(const Node& source);
        };




        /******************************
         * Term
         ******************************/

    protected:
        FL_unique_ptr<Node> _root;
        std::string _formula;
        const Engine* _engine;
    public:
        mutable std::map<std::string, scalar> variables;
        Function(const std::string& name = "",
                const std::string& formula = "", const Engine* engine = fl::null);
        Function(const Function& other);
        Function& operator=(const Function& other);
        virtual ~Function() FL_IOVERRIDE;
        FL_DEFAULT_MOVE(Function)

        static Function* create(const std::string& name,
                const std::string& formula,
                const Engine* engine = fl::null); // throw (fl::Exception);

        virtual scalar membership(scalar x) const FL_IOVERRIDE;

        virtual scalar evaluate(const std::map<std::string, scalar>* variables) const;

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        virtual void setFormula(const std::string& formula);
        virtual std::string getFormula() const;

        virtual void setEngine(const Engine* engine);
        virtual const Engine* getEngine() const;

        virtual Node* root() const;

        virtual bool isLoaded() const;
        virtual void unload();
        virtual void load(); // throw (fl::Exception);
        virtual void load(const std::string& formula); // throw (fl::Exception);
        virtual void load(const std::string& formula, const Engine* engine); // throw (fl::Exception);

        virtual Node* parse(const std::string& formula); // throw (fl::Exception);

        virtual std::string toPostfix(const std::string& formula) const; //throw (fl::Exception);

        virtual std::string space(const std::string& formula) const;

        virtual Function* clone() const FL_IOVERRIDE;

        static Term* constructor();

        static void main();

    };

}

#endif  /* FL_FUNCTION_H */

