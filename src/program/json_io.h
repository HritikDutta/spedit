#pragma once

#include <string>
#include "containers/darray.h"
#include "animation.h"
#include "engine/ui.h"

void OutputToJSONFile(const std::string& fullpath, const std::string& filename, const gn::darray<Animation>& animations);
bool LoadFromJSONFile(const std::string& jsonfile, std::string& fullpath, std::string& filename, gn::darray<Animation>& animations, UI::Image& image, bool imageLoaded);