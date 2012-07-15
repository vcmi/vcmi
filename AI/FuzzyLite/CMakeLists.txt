project(FuzzyLite_lib)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR})

set(FuzzyLite_lib_SRCS
        AreaCentroidAlgorithm.cpp
        FuzzyOperation.cpp
        OutputLVar.cpp
        CompoundTerm.cpp
        FuzzyOperator.cpp
        RectangularTerm.cpp
        DescriptiveAntecedent.cpp
        FuzzyOr.cpp
        RuleBlock.cpp
        DiscreteTerm.cpp
        FuzzyRule.cpp
        ShoulderTerm.cpp
        FunctionTerm.cpp
        Hedge.cpp
        SingletonTerm.cpp
        FuzzyAnd.cpp
        HedgeSet.cpp
        StrOp.cpp
        FuzzyAntecedent.cpp
        InfixToPostfix.cpp
        TakagiSugenoConsequent.cpp
        FuzzyConsequent.cpp
        InputLVar.cpp
        TakagiSugenoRule.cpp
        FuzzyDefuzzifier.cpp
        LinguisticTerm.cpp
        TakagiSugenoTerm.cpp
        FuzzyEngine.cpp
        LinguisticVariable.cpp
        test.cpp
        FuzzyException.cpp
        main.cpp
        TrapezoidalTerm.cpp
        FuzzyExceptions.cpp
        MamdaniConsequent.cpp
        TriangularTerm.cpp
        FuzzyModulation.cpp
        MamdaniRule.cpp   
)

add_library(FuzzyLite_lib STATIC ${FuzzyLite_lib_SRCS})

IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET_TARGET_PROPERTIES(FuzzyLite_lib PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")