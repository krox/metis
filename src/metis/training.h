#pragma once

#include "CLI/CLI.hpp"
#include "metis/engine.h"
#include "metis/evaluator.h"
#include <vector>

namespace metis {

void setup_train_command(CLI::App &app);

void run_training(Engine &engine, LinearEvaluator &eval);
} // namespace metis