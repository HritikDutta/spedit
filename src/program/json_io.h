#pragma once

#include <string>
#include "containers/darray.h"
#include "animation.h"
#include "engine/ui.h"
#include "context.h"

void OutputToJSONFile(const Context& context);
bool LoadFromJSONFile(const std::string& jsonfile, Context& context);