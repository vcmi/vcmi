/*   Copyright 2010 Juan Rada-Vilela

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */
#include "test.h"
#include "FuzzyLite.h"
#include <limits>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "FunctionTerm.h"
namespace fl {

    void Test::SimpleMamdani() {
        FuzzyOperator& op = FuzzyOperator::DefaultFuzzyOperator();
        FuzzyEngine engine("simple-mamdani", op);
        engine.hedgeSet().add(new fl::HedgeNot);
        engine.hedgeSet().add(new fl::HedgeSomewhat);
        engine.hedgeSet().add(new fl::HedgeVery);
        fl::InputLVar* energy = new fl::InputLVar("Energy");
        energy->addTerm(new fl::ShoulderTerm("LOW", 0.25, 0.5, true));
        energy->addTerm(new fl::TriangularTerm("MEDIUM", 0.25, 0.75));
        energy->addTerm(new fl::ShoulderTerm("HIGH", 0.50, 0.75, false));
        engine.addInputLVar(energy);

        fl::OutputLVar* health = new fl::OutputLVar("Health");
        health->addTerm(new fl::TriangularTerm("BAD", 0.0, 0.50));
        health->addTerm(new fl::TriangularTerm("REGULAR", 0.25, 0.75));
        health->addTerm(new fl::TriangularTerm("GOOD", 0.50, 1.00));
        engine.addOutputLVar(health);
        fl::RuleBlock* block = new fl::RuleBlock();
        block->addRule(new fl::MamdaniRule("if Energy is LOW then Health is BAD", engine));
        block->addRule(new fl::MamdaniRule("if Energy is MEDIUM then Health is REGULAR", engine));
        block->addRule(new fl::MamdaniRule("if Energy is HIGH then Health is GOOD", engine));
        engine.addRuleBlock(block);

        for (fl::flScalar in = 0.0; in < 1.1; in += 0.1) {
            energy->setInput(in);
            engine.process();
            fl::flScalar out = health->output().defuzzify();
            (void)out; //Just to avoid warning when building
            FL_LOG("Energy=" << in);
            FL_LOG("Energy is " << energy->fuzzify(in));
            FL_LOG("Health=" << out);
            FL_LOG("Health is " << health->fuzzify(out));
            FL_LOG("--");
        }
    }

    void Test::ComplexMamdani() {
        FuzzyOperator& op = FuzzyOperator::DefaultFuzzyOperator();
        FuzzyEngine engine("complex-mamdani", op);

        engine.hedgeSet().add(new fl::HedgeNot);
        engine.hedgeSet().add(new fl::HedgeSomewhat);
        engine.hedgeSet().add(new fl::HedgeVery);



        fl::InputLVar* energy = new fl::InputLVar("Energy");
        energy->addTerm(new fl::TriangularTerm("LOW", 0.0, 0.50));
        energy->addTerm(new fl::TriangularTerm("MEDIUM", 0.25, 0.75));
        energy->addTerm(new fl::TriangularTerm("HIGH", 0.50, 1.00));
        engine.addInputLVar(energy);

        fl::InputLVar* distance = new fl::InputLVar("Distance");
        distance->addTerm(new fl::TriangularTerm("NEAR", 0, 500));
        distance->addTerm(new fl::TriangularTerm("FAR", 250, 750));
        distance->addTerm(new fl::TriangularTerm("FAR_AWAY", 500, 1000));
        engine.addInputLVar(distance);

        fl::OutputLVar* power = new fl::OutputLVar("Power");
        power->addTerm(new fl::ShoulderTerm("LOW", 0.25, 0.5, true));
        power->addTerm(new fl::TriangularTerm("MEDIUM", 0.25, 0.75));
        power->addTerm(new fl::ShoulderTerm("HIGH", 0.50, 0.75, false));
        engine.addOutputLVar(power);

        fl::RuleBlock* block = new fl::RuleBlock();
        block->addRule(new fl::MamdaniRule("if Energy is LOW and Distance is FAR_AWAY then Power is LOW", engine));
        block->addRule(new fl::MamdaniRule("if Energy is LOW and Distance is FAR then Power is very MEDIUM", engine));
        block->addRule(new fl::MamdaniRule("if Energy is LOW and Distance is NEAR then Power is HIGH", engine));
        block->addRule(new fl::MamdaniRule("if Energy is MEDIUM and Distance is FAR_AWAY then Power is LOW with 0.8", engine));
        block->addRule(new fl::MamdaniRule("if Energy is MEDIUM and Distance is FAR then Power is MEDIUM with 1.0", engine));
        block->addRule(new fl::MamdaniRule("if Energy is MEDIUM and Distance is NEAR then Power is HIGH with 0.3", engine));
        block->addRule(new fl::MamdaniRule("if Energy is HIGH and Distance is FAR_AWAY then Power is LOW with 0.43333", engine));
        block->addRule(new fl::MamdaniRule("if Energy is HIGH and Distance is FAR then Power is MEDIUM", engine));
        block->addRule(new fl::MamdaniRule("if Energy is HIGH and Distance is NEAR then Power is HIGH", engine));
        engine.addRuleBlock(block);

        for (int i = 0; i < block->numberOfRules(); ++i) {
            FL_LOG(block->rule(i)->toString());
        }

        return;
        for (fl::flScalar in = 0.0; in < 1.1; in += 0.1) {
            energy->setInput(in);
            distance->setInput(500);
            engine.process();
            fl::flScalar out = power->output().defuzzify();
            (void)out; //Just to avoid warning when building
            FL_LOG("Energy=" << in);
            FL_LOG("Energy is " << power->fuzzify(in));
            FL_LOG("Health=" << out);
            FL_LOG("Health is " << energy->fuzzify(out));
            FL_LOG("--");
        }
    }

    void Test::SimpleTakagiSugeno() {
        FuzzyOperator& op = FuzzyOperator::DefaultFuzzyOperator();
        op.setDefuzzifier(new TakagiSugenoDefuzzifier);
        FuzzyEngine engine("takagi-sugeno", op);


        fl::InputLVar* x = new fl::InputLVar("x");
        x->addTerm(new fl::TriangularTerm("NEAR_1", 0, 2));
        x->addTerm(new fl::TriangularTerm("NEAR_2", 1, 3));
        x->addTerm(new fl::TriangularTerm("NEAR_3", 2, 4));
        x->addTerm(new fl::TriangularTerm("NEAR_4", 3, 5));
        x->addTerm(new fl::TriangularTerm("NEAR_5", 4, 6));
        x->addTerm(new fl::TriangularTerm("NEAR_6", 5, 7));
        x->addTerm(new fl::TriangularTerm("NEAR_7", 6, 8));
        x->addTerm(new fl::TriangularTerm("NEAR_8", 7, 9));
        x->addTerm(new fl::TriangularTerm("NEAR_9", 8, 10));
        engine.addInputLVar(x);

        fl::OutputLVar* f_x = new fl::OutputLVar("f_x");
        f_x->addTerm(new fl::FunctionTerm("function", "(sin x) / x", 0, 10));
        engine.addOutputLVar(f_x);

        fl::RuleBlock* block = new fl::RuleBlock();
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_1 then f_x=0.84", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_2 then f_x=0.45", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_3 then f_x=0.04", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_4 then f_x=-0.18", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_5 then f_x=-0.19", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_6 then f_x=-0.04", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_7 then f_x=0.09", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_8 then f_x=0.12", engine));
        block->addRule(new fl::TakagiSugenoRule("if x is NEAR_9 then f_x=0.04", engine));

        engine.addRuleBlock(block);

        int n = 40;
        flScalar mse = 0;
        for (fl::flScalar in = x->minimum(); in < x->maximum() ;
                in += (x->minimum() + x->maximum()) / n) {
            x->setInput(in);
            engine.process();
            flScalar expected = f_x->term(0)->membership(in);
            flScalar obtained = f_x->output().defuzzify();
            flScalar se = (expected - obtained) * (expected - obtained);
            mse += isnan(se) ? 0 : se;
            FL_LOG("x=" << in << "\texpected_out=" << expected << "\tobtained_out=" << obtained
                    << "\tse=" << se);
        }
        FL_LOG("MSE=" << mse / n);
    }

    void Test::SimplePendulum() {
        FuzzyOperator& op = FuzzyOperator::DefaultFuzzyOperator();
        FuzzyEngine engine("pendulum-3d",op);

        InputLVar* anglex = new InputLVar("AngleX");
        std::vector<std::string> labels;
        labels.push_back("NEAR_0");
        labels.push_back("NEAR_45");
        labels.push_back("NEAR_90");
        labels.push_back("NEAR_135");
        labels.push_back("NEAR_180");
        anglex->createTerms(5, LinguisticTerm::MF_SHOULDER, 0, 180, labels);
        engine.addInputLVar(anglex);

        InputLVar* anglez = new InputLVar("AngleZ");
        labels.clear();
        labels.push_back("NEAR_0");
        labels.push_back("NEAR_45");
        labels.push_back("NEAR_90");
        labels.push_back("NEAR_135");
        labels.push_back("NEAR_180");
        anglez->createTerms(5, LinguisticTerm::MF_SHOULDER, 0, 180, labels);
        engine.addInputLVar(anglez);

        OutputLVar* forcex = new OutputLVar("ForceX");
        labels.clear();
        labels.push_back("NL");
        labels.push_back("NS");
        labels.push_back("ZR");
        labels.push_back("PS");
        labels.push_back("PL");
        forcex->createTerms(5, LinguisticTerm::MF_TRIANGULAR, -1, 1, labels);
        engine.addOutputLVar(forcex);

        OutputLVar* forcez = new OutputLVar("ForceZ");
        labels.clear();
        labels.push_back("NL");
        labels.push_back("NS");
        labels.push_back("ZR");
        labels.push_back("PS");
        labels.push_back("PL");
        forcez->createTerms(5, LinguisticTerm::MF_TRIANGULAR, -1, 1, labels);
        engine.addOutputLVar(forcez);

        RuleBlock* ruleblock = new RuleBlock("Rules");
        ruleblock->addRule(new MamdaniRule("if AngleX is NEAR_180 then ForceX is NL", engine));
        ruleblock->addRule(new MamdaniRule("if AngleX is NEAR_135 then ForceX is NS", engine));
        ruleblock->addRule(new MamdaniRule("if AngleX is NEAR_90 then ForceX is ZR", engine));
        ruleblock->addRule(new MamdaniRule("if AngleX is NEAR_45 then ForceX is PS", engine));
        ruleblock->addRule(new MamdaniRule("if AngleX is NEAR_0 then ForceX is PL", engine));

        ruleblock->addRule(new MamdaniRule("if AngleZ is NEAR_180 then ForceZ is NL", engine));
        ruleblock->addRule(new MamdaniRule("if AngleZ is NEAR_135 then ForceZ is NS", engine));
        ruleblock->addRule(new MamdaniRule("if AngleZ is NEAR_90 then ForceZ is ZR", engine));
        ruleblock->addRule(new MamdaniRule("if AngleZ is NEAR_45 then ForceZ is PS", engine));
        ruleblock->addRule(new MamdaniRule("if AngleZ is NEAR_0 then ForceZ is PL", engine));
        engine.addRuleBlock(ruleblock);

        FL_LOG(engine.toString());
        for (int i = 0; i < 180; i += 20) {
            engine.setInput("AngleX", i);
            engine.process();
            FL_LOG("angle=" << i << "\tforce=" << engine.output("ForceX"));
        }
    }

    void Test::main(int args, char** argv) {
        FL_LOG("Starting in 2 second");
        FL_LOG("Example: Simple Mamdani");
        FL_LOG("=======================");
#ifdef _MSC_VER
        //Sleep(2);
#else
        sleep(2);
#endif
        SimpleMamdani();
        FL_LOG("=======================\n");

        FL_LOG("Starting in 2 second");
        FL_LOG("Example: Complex Mamdani");
        FL_LOG("========================");
#ifdef _MSC_VER
        //Sleep(2);
#else
        sleep(2);
#endif
        ComplexMamdani();
        FL_LOG("=======================\n");

        FL_LOG("Starting in 2 second");
        FL_LOG("Example: Simple Pendulum");
        FL_LOG("========================");
#ifdef _MSC_VER
        //Sleep(2);
#else
        sleep(2);
#endif

        SimplePendulum();
        FL_LOG("=======================\n");

        FL_LOG("Starting in 2 second");
        FL_LOG("Example: Simple Takagi-Sugeno");
        FL_LOG("========================");
#ifdef _MSC_VER
        //Sleep(2);
#else
        sleep(2);
#endif

        SimpleTakagiSugeno();
        FL_LOG("=======================\n");

        FL_LOG("For further examples build the GUI...");
    }

}

