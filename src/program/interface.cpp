#include <sstream>
#include <string>
#include "math/types.h"
#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/application.h"
#include "animation.h"
#include "colors.h"

static bool showDialogue = false;
static s32  numAnimations = 1;
static UI::ID textInputID = GenUIID();

static std::string defaultName = "animation_0";

void ShowNewAnimationDialog()
{
    showDialogue = true;
    UI::SetActive(textInputID);
}

void HideNewAnimationDialog()
{
    showDialogue = false;
}

// This is a bit hackey
bool RenderNewAnimationDialog(Application& app, const UI::Font& font, gn::darray<Animation>& list)
{
    if (!showDialogue)
        return false;

    bool result = false;

    constexpr f32 vgap = 10.0f;
    constexpr f32 hgap = 10.0f;

    const f32 totalWidth  = 325.0f;
    const f32 totalHeight = 3.0f * font.fontHeight + 6.0f * vgap;

    f32 height = vgap;
    UI::Rect rect;

    {   // Background
        rect.topLeft = Vector3((app.refScreenWidth - totalWidth) / 2.0f, (app.refScreenHeight - totalHeight) / 2.0f, 0.0f);
        rect.size = Vector2(totalWidth, totalHeight);
        UI::RenderRect(app, rect, grey);

        height += rect.topLeft.y;
    }

    {   // Header
        std::string_view text = "New Animation";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        Vector3 topLeft((app.refScreenWidth - size.x) / 2.0f, height, 0.0f);
        UI::RenderText(app, "New Animation", font, white, topLeft);

        height += size.y + vgap;
    }

    {   // Text Box
        f32 width = totalWidth - 40.0f;
        Vector3 topLeft((app.refScreenWidth - width) / 2.0f, height, 0.0f);
        UI::RenderTextInput(app, textInputID, defaultName, font, Vector2(10.0f, 5.0f), topLeft, width);

        height += font.fontHeight + 20.0f + vgap;
    }

    {   // Buttons
        Vector3 topLeft(rect.topLeft.x + hgap, height, 0.0f);
        if (UI::RenderTextButton(app, GenUIID(), "Cancel", font, Vector2(10.0f, 5.0f), topLeft))
        {
            std::stringstream ss;
            ss << "animation_" << numAnimations - 1;
            defaultName = ss.str();
            showDialogue = false;
        }

        std::string_view text = "Add";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        topLeft.x = ((app.refScreenWidth + totalWidth) / 2.0f) - size.x - 20.0f - hgap;
        if (app.GetKeyDown(KEY(ENTER)) || app.GetKeyDown(KEY(KP_ENTER)) ||
            UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), topLeft))
        {
            if (defaultName.length() <= 0)
                return false;

            list.emplace_back(defaultName);

            std::stringstream ss;
            ss << "animation_" << numAnimations;
            defaultName = ss.str();
            numAnimations++;

            result = true;
            showDialogue = false;
        }
    }
    
    return result;
}

Vector2 RenderAnimationInfo(Application& app, const UI::Font& font, gn::darray<Animation>& animations, int& index, const Vector3& topLeft)
{
    Animation& animation = animations[index];

    constexpr f32 vgap = 10.0f;
    constexpr f32 totalWidth = 325.0f;

    f32 height = topLeft.y;

    {   // Loop Type
        std::string_view label = "Loop:\t";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, topLeft + Vector3(0.0f, 5.0f, 0.0f));

        Vector3 typePos = topLeft + Vector3(size.x, 0.0f, 0.0f);
        std::string_view typeText = animation.GetLoopTypeName();
        size = UI::GetRenderedTextSize(typeText, font);
        if (UI::RenderTextButton(app, GenUIID(), typeText, font, Vector2(10.0f, 5.0f), typePos))
        {
            animation.loopType = (Animation::LoopType) (((s32) animation.loopType + 1) % (s32) Animation::LoopType::NUM_TYPES);
        }

        height += size.y + vgap;
    }

    {   // Frame Rate
        std::string_view label = "FPS:\t";
        Vector2 size = UI::GetRenderedTextSize("Loop:\t", font);
        UI::RenderText(app, label, font, white, Vector3(topLeft.x, height + 5.0f, topLeft.z));

        static f32 width = UI::GetRenderedTextSize("Ping Pong", font).x + 20.0f;
        static std::string text;

        Vector3 inputPos(topLeft.x + size.x, height, topLeft.z);
        UI::RenderNumericInputf(app, GenUIID(), animation.frameRate, text, font, Vector2(10.0f, 5.0f), inputPos, width);

        height += size.y + 5.0f + vgap;
    }

    {   // Frames
        char buffer[128];
        sprintf(buffer, "Frames:\t%zd", animation.frames.size());
        Vector2 size = UI::GetRenderedTextSize(buffer, font);
        UI::RenderText(app, buffer, font, white, Vector3(topLeft.x, height, topLeft.z));

        height += size.y + vgap;
    }

    {   // Delete
        std::string_view text = "Delete";
        Vector2 size = UI::GetRenderedTextSize(text, font);

        f32 maxX = app.refScreenWidth - topLeft.x;
        f32 x = topLeft.x + ((maxX - size.x - 20.0f) / 2.0f) - 5.0f;
        if (UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), Vector3(x, height, topLeft.z)))
        {
            animations.erase_at(index);
            index = -1;
        }

        height += vgap;
    }

    return Vector2 { 0.0f, height - topLeft.y };
}

void ResetAnimations(gn::darray<Animation>& animations)
{
    animations.clear();
    numAnimations = 1;
    defaultName = "animation_0";
}