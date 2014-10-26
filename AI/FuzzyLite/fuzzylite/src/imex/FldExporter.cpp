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

#include "fl/imex/FldExporter.h"

#include "fl/Engine.h"
#include "fl/Operation.h"
#include "fl/variable/Variable.h"
#include "fl/variable/InputVariable.h"
#include "fl/variable/OutputVariable.h"

#include <cmath>
#include <fstream>
#include <vector>

namespace fl {

    FldExporter::FldExporter(const std::string& separator) : Exporter(),
    _separator(separator), _exportHeaders(true),
    _exportInputValues(true), _exportOutputValues(true) {

    }

    FldExporter::~FldExporter() {
    }

    std::string FldExporter::name() const {
        return "FldExporter";
    }

    void FldExporter::setSeparator(const std::string& separator) {
        this->_separator = separator;
    }

    std::string FldExporter::getSeparator() const {
        return this->_separator;
    }

    void FldExporter::setExportHeader(bool exportHeaders) {
        this->_exportHeaders = exportHeaders;
    }

    bool FldExporter::exportsHeader() const {
        return this->_exportHeaders;
    }

    void FldExporter::setExportInputValues(bool exportInputValues) {
        this->_exportInputValues = exportInputValues;
    }

    bool FldExporter::exportsInputValues() const {
        return this->_exportInputValues;
    }

    void FldExporter::setExportOutputValues(bool exportOutputValues) {
        this->_exportOutputValues = exportOutputValues;
    }

    bool FldExporter::exportsOutputValues() const {
        return this->_exportOutputValues;
    }

    std::string FldExporter::header(const Engine* engine) const {
        std::vector<std::string> result;
        if (_exportInputValues) {
            for (int i = 0; i < engine->numberOfInputVariables(); ++i) {
                InputVariable* inputVariable = engine->getInputVariable(i);
                result.push_back("@InputVariable: " + inputVariable->getName() + ";");
            }
        }
        if (_exportOutputValues) {
            for (int i = 0; i < engine->numberOfOutputVariables(); ++i) {
                OutputVariable* outputVariable = engine->getOutputVariable(i);
                result.push_back("@OutputVariable: " + outputVariable->getName() + ";");
            }
        }
        return "#@Engine: " + engine->getName() + ";\n#" + Op::join(result, _separator);
    }

    std::string FldExporter::toString(const Engine* engine) const {
        return toString(const_cast<Engine*> (engine), 1024);
    }

    std::string FldExporter::toString(Engine* engine, int maximumNumberOfResults) const {
        std::ostringstream result;
        write(engine, result, maximumNumberOfResults);
        return result.str();
    }

    void FldExporter::toFile(const std::string& path, Engine* engine, int maximumNumberOfResults) const {
        std::ofstream writer(path.c_str());
        if (not writer.is_open()) {
            throw fl::Exception("[file error] file <" + path + "> could not be created", FL_AT);
        }
        write(engine, writer, maximumNumberOfResults);
        writer.close();
    }

    std::string FldExporter::toString(Engine* engine, const std::string& inputData) const {
        std::ostringstream writer;
        if (_exportHeaders) writer << header(engine) << "\n";
        std::istringstream reader(inputData);
        std::string line;
        int lineNumber = 0;
        while (std::getline(reader, line)) {
            ++lineNumber;
            line = Op::trim(line);
            if (not line.empty() and line.at(0) == '#') continue; //comments are ignored, blank lines are retained
            std::vector<scalar> inputValues = parse(line);
            write(engine, writer, inputValues);
            writer.flush();
        }
        return writer.str();
    }

    void FldExporter::toFile(const std::string& path, Engine* engine, const std::string& inputData) const {
        std::ofstream writer(path.c_str());
        if (not writer.is_open()) {
            throw fl::Exception("[file error] file <" + path + "> could not be created", FL_AT);
        }
        if (_exportHeaders) writer << header(engine) << "\n";
        std::istringstream reader(inputData);
        std::string line;
        int lineNumber = 0;
        while (std::getline(reader, line)) {
            ++lineNumber;
            line = Op::trim(line);
            if (not line.empty() and line.at(0) == '#') continue; //comments are ignored, blank lines are retained
            std::vector<scalar> inputValues = parse(line);
            write(engine, writer, inputValues);
            writer.flush();
        }
        writer.close();
    }

    std::vector<scalar> FldExporter::parse(const std::string& x) const {
        std::vector<scalar> inputValues;
        if (not (x.empty() or x.at(0) == '#')) {
            std::istringstream tokenizer(x);
            std::string token;
            while (tokenizer >> token)
                inputValues.push_back(fl::Op::toScalar(token));
        }
        return inputValues;
    }

    void FldExporter::write(Engine* engine, std::ostream& writer, int maximum) const {
        if (_exportHeaders) writer << header(engine) << "\n";

        int resolution = -1 + (int) std::max(1.0, std::pow(
                maximum, 1.0 / engine->numberOfInputVariables()));
        std::vector<int> sampleValues, minSampleValues, maxSampleValues;
        for (int i = 0; i < engine->numberOfInputVariables(); ++i) {
            sampleValues.push_back(0);
            minSampleValues.push_back(0);
            maxSampleValues.push_back(resolution);
        }

        engine->restart();

        bool overflow = false;
        std::vector<scalar> inputValues(engine->numberOfInputVariables());
        while (not overflow) {
            for (int i = 0; i < engine->numberOfInputVariables(); ++i) {
                InputVariable* inputVariable = engine->getInputVariable(i);
                inputValues.at(i) = inputVariable->getMinimum()
                        + sampleValues.at(i) * inputVariable->range() / std::max(1, resolution);
            }
            write(engine, writer, inputValues);
            overflow = Op::increment(sampleValues, minSampleValues, maxSampleValues);
        }
    }

    void FldExporter::write(Engine* engine, std::ostream& writer, std::istream& reader) const {
        if (_exportHeaders) writer << header(engine) << "\n";

        engine->restart();

        std::string line;
        int lineNumber = 0;
        while (std::getline(reader, line)) {
            ++lineNumber;
            try {
                std::vector<scalar> inputValues = parse(Op::trim(line));
                write(engine, writer, inputValues);
            } catch (fl::Exception& ex) {
                ex.append(" at line <" + Op::str(lineNumber) + ">");
                throw;
            }
        }
    }

    void FldExporter::write(Engine* engine, std::ostream& writer, const std::vector<scalar>& inputValues) const {
        if (inputValues.empty()) {
            writer << "\n";
            return;
        }
        if (int(inputValues.size()) < engine->numberOfInputVariables()) {
            std::ostringstream ex;
            ex << "[export error] engine has <" << engine->numberOfInputVariables() << "> "
                    "input variables, but input data provides <" << inputValues.size() << "> values";
            throw fl::Exception(ex.str(), FL_AT);
        }

        std::vector<std::string> values;
        for (int i = 0; i < engine->numberOfInputVariables(); ++i) {
            InputVariable* inputVariable = engine->getInputVariable(i);
            scalar inputValue = inputVariable->isEnabled() ? inputValues.at(i) : fl::nan;
            inputVariable->setInputValue(inputValue);
            if (_exportInputValues) values.push_back(Op::str(inputValue));
        }

        engine->process();

        for (int i = 0; i < engine->numberOfOutputVariables(); ++i) {
            OutputVariable* outputVariable = engine->getOutputVariable(i);
            outputVariable->defuzzify();
            if (_exportOutputValues)
                values.push_back(Op::str(outputVariable->getOutputValue()));
        }

        writer << Op::join(values, _separator) << "\n";
    }

    FldExporter* FldExporter::clone() const {
        return new FldExporter(*this);
    }

}
