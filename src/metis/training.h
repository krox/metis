#pragma once

#include "metis/engine.h"
#include "metis/evaluator.h"
#include <vector>

namespace metis {
void run_training(Engine &engine, LinearEvaluator const &eval);
}