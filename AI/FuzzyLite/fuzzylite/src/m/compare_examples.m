%{/*
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
 
 fuzzylite (R) is a registered trademark of FuzzyLite Limited.
 */
}%


function [ engines ] = compare_examples(path, delimiter, hasMetadata)
if (nargin < 2)
    delimiter = ' ';
end
if (nargin < 3)
    hasMetadata = true;
end

examples = {'\mamdani\SimpleDimmer', '\mamdani\matlab\mam21', '\mamdani\matlab\mam22', '\mamdani\matlab\shower', '\mamdani\matlab\tank', '\mamdani\matlab\tank2', '\mamdani\matlab\tipper', '\mamdani\matlab\tipper1', '\mamdani\octave\mamdani_tip_calculator', '\takagi-sugeno\SimpleDimmer', '\takagi-sugeno\matlab\fpeaks', '\takagi-sugeno\matlab\invkine1', '\takagi-sugeno\matlab\invkine2', '\takagi-sugeno\matlab\juggler', '\takagi-sugeno\matlab\membrn1', '\takagi-sugeno\matlab\membrn2', '\takagi-sugeno\matlab\slbb', '\takagi-sugeno\matlab\slcp', '\takagi-sugeno\matlab\slcp1', '\takagi-sugeno\matlab\slcpp1', '\takagi-sugeno\matlab\sltbu_fl', '\takagi-sugeno\matlab\sugeno1', '\takagi-sugeno\matlab\tanksg', '\takagi-sugeno\matlab\tippersg', '\takagi-sugeno\octave\cubic_approximator', '\takagi-sugeno\octave\heart_disease_risk', '\takagi-sugeno\octave\linear_tip_calculator'};
pending = {'\mamdani\octave\investment_portfolio', '\takagi-sugeno\approximation', '\takagi-sugeno\octave\sugeno_tip_calculator', '\tsukamoto\tsukamoto'};
engines = [];
for i = 1 : length(examples)
    fisFile = strcat(path, examples{i}, '.fis')
    fldFile = strcat(path, examples{i}, '.fld');
    engines = [engines compare(fisFile, fldFile, delimiter, hasMetadata)];
    disp(strcat('Five number summary (', num2str(i), '): ', fisFile));
    engines(i).quantiles
end
end

