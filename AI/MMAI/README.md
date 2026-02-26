# Overview

A [VCMI](https://github.com/vcmi/vcmi) AI library which uses pre-trained
models for commanding a hero's army in battle.

During gameplay, MMAI extracts various features from the battlefield,
feeds it to a pre-trained model and executes an action based on the model's
output.

MMAI is also essential during model training, where the collected data is sent
to [`vcmi-gym`](https://github.com/smanolloff/vcmi-gym) (a Reinforcement
Learning environment designed for VCMI).
