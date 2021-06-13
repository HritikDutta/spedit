#pragma once

#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/application.h"
#include "animation.h"

void ShowNewAnimationDialog();
void HideNewAnimationDialog();
bool RenderNewAnimationDialog(Application& app, const UI::Font& font, gn::darray<Animation>& list);

// Returns size of the panel
Vector2 RenderAnimationInfo(Application& app, const UI::Font& font, gn::darray<Animation>& animations, int& index, const Vector3& topLeft);

void ResetAnimations(gn::darray<Animation>& animations);