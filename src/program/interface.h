#pragma once

#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/application.h"
#include "animation.h"
#include "context.h"

void ShowNewAnimationDialog();
void HideNewAnimationDialog();
bool RenderNewAnimationDialog(Application& app, const UI::Font& font, Context& context);

// Returns size of the panel
Vector2 RenderAnimationInfo(Application& app, const UI::Font& font, Context& context, const Vector3& topLeft);

void RenderFrameInfo(Application& app, const UI::Font& font, Context& context);

void ResetAnimations(gn::darray<Animation>& animations);