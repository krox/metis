#pragma once

#include "CLI/CLI.hpp"
#include "metis/engine.h"
#include "metis/evaluator.h"
#include <vector>

namespace metis {

void setup_train_command(CLI::App &app);

} // namespace metis