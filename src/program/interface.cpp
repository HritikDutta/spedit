#include <sstream>
#include <string>
#include "math/types.h"
#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/application.h"
#include "animation.h"
#include "colors.h"
#include "context.h"

static bool showRenameAnimationDialogue = false;
static bool showNewAnimationDialogue    = false;
static bool showSplitSheetDialogue = false;
static s32  numAnimations = 1;
static UI::ID textInputID = GenUIID();

static std::string defaultName = "animation_0";

void ShowNewAnimationDialog()
{
    showNewAnimationDialogue = true;
    UI::SetActive(textInputID);
}

void HideNewAnimationDialog()
{
    showNewAnimationDialogue = false;
}

// This is a bit hackey
bool RenderNewAnimationDialog(Application& app, const UI::Font& font, Context& context)
{
    if (!showNewAnimationDialogue)
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
        if (app.GetKeyDown(KEY(ESCAPE)) ||
            UI::RenderTextButton(app, GenUIID(), "Cancel", font, Vector2(10.0f, 5.0f), topLeft))
        {
            std::stringstream ss;
            ss << "animation_" << numAnimations - 1;
            defaultName = ss.str();
            showNewAnimationDialogue = false;
        }

        std::string_view text = "Add";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        topLeft.x = ((app.refScreenWidth + totalWidth) / 2.0f) - size.x - 20.0f - hgap;
        if (app.GetKeyDown(KEY(ENTER)) || app.GetKeyDown(KEY(KP_ENTER)) ||
            UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), topLeft))
        {
            if (defaultName.length() <= 0)
                return false;

            context.animations.emplace_back(defaultName);

            std::stringstream ss;
            ss << "animation_" << numAnimations;
            defaultName = ss.str();
            numAnimations++;

            result = true;
            showNewAnimationDialogue = false;
        }
    }
    
    return result;
}

// This is a bit hackey
static void RenderRenameAnimationDialog(Application& app, const UI::Font& font, Context& context)
{
    static bool copiedName = false;
    static std::string tempName;

    if (!copiedName)
    {
        tempName = context.CurrentAnimation().name;
        copiedName = true;
    }

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
        std::string_view text = "Rename Animation";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        Vector3 topLeft((app.refScreenWidth - size.x) / 2.0f, height, 0.0f);
        UI::RenderText(app, text, font, white, topLeft);

        height += size.y + vgap;
    }

    {   // Text Box
        f32 width = totalWidth - 40.0f;
        Vector3 topLeft((app.refScreenWidth - width) / 2.0f, height, 0.0f);
        UI::RenderTextInput(app, textInputID, tempName, font, Vector2(10.0f, 5.0f), topLeft, width);

        height += font.fontHeight + 20.0f + vgap;
    }

    {   // Buttons
        Vector3 topLeft(rect.topLeft.x + hgap, height, 0.0f);
        if (app.GetKeyDown(KEY(ESCAPE)) ||
            UI::RenderTextButton(app, GenUIID(), "Cancel", font, Vector2(10.0f, 5.0f), topLeft))
        {
            showRenameAnimationDialogue = false;
            copiedName = false;
        }

        std::string_view text = "Confirm";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        topLeft.x = ((app.refScreenWidth + totalWidth) / 2.0f) - size.x - 20.0f - hgap;
        if (app.GetKeyDown(KEY(ENTER)) || app.GetKeyDown(KEY(KP_ENTER)) ||
            UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), topLeft))
        {
            if (tempName.length() >= 0)
                context.CurrentAnimation().name = tempName;

            showRenameAnimationDialogue = false;
            copiedName = false;
        }
    }
}

static bool FrameNotEmpty(const Context& context, const AnimationFrame& frame)
{
    u32 yStart = frame.topLeft.y;
    u32 xStart = frame.topLeft.x;
    u32 yEnd   = frame.topLeft.y + frame.size.y;
    u32 xEnd   = frame.topLeft.x + frame.size.x;

    for (u32 y = yStart; y < yEnd; y++)
    {
        for (u32 x = xStart; x < xEnd; x++)
        {
            u32 pixelIndex = y * context.image.width + x;
            if (context.image.pixels[4 * pixelIndex + 3] != 0)
                return true;
        }
    }

    return false;
}

static void SplitSheet(Context& context, Vector2 frameSize)
{
    if (frameSize.x <= 0.0f || frameSize.y <= 0.0f)
        return;
    
    context.CurrentAnimation().frames.clear();
    context.selectedFrameIndex = -1;

    f32 yEnd = context.image.height - frameSize.y;
    f32 xEnd = context.image.width - frameSize.x;
    for (f32 y = 0.0f; y <= yEnd; y += frameSize.y)
    {
        for (f32 x = 0.0f; x <= xEnd; x += frameSize.x)
        {
            AnimationFrame frame;
            frame.topLeft = Vector2(x, y);
            frame.size = frameSize;

            if (FrameNotEmpty(context, frame))
                context.CurrentAnimation().frames.emplace_back(frame);
        }
    }
}

static void RenderSplitSheetDialog(Application& app, const UI::Font& font, Context& context)
{
    if (app.GetKeyDown(KEY(ESCAPE)))
    {
        showSplitSheetDialogue = false;
        return;
    }

    constexpr f32 vgap = 10.0f;
    constexpr f32 hgap = 10.0f;

    const f32 totalWidth  = UI::GetRenderedTextSize("Split Sprite Sheet", font).x + 2 * hgap;
    const f32 totalHeight = 3.0f * font.fontHeight + 6.0f * vgap;

    f32 height = vgap;
    UI::Rect rect;

    static f32 numericInputWidth = UI::GetRenderedTextSize("1920", font).x + 20.0f;

    static s32 frameWidth = 0, frameHeight = 0;

    {   // Background
        rect.topLeft = Vector3((app.refScreenWidth - totalWidth) / 2.0f, (app.refScreenHeight - totalHeight) / 2.0f, 0.0f);
        rect.size = Vector2(totalWidth, totalHeight);
        UI::RenderRect(app, rect, grey);

        height += rect.topLeft.y;
    }

    {   // Header
        std::string_view text = "Split Sprite Sheet";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        Vector3 topLeft((app.refScreenWidth - size.x) / 2.0f, height, 0.0f);
        UI::RenderText(app, text, font, white, topLeft);

        height += size.y + vgap;
    }

    {   // Width Input
        f32 x = rect.topLeft.x + hgap;
        Vector2 size;

        {   // Label
            std::string_view label = "Width:\t";
            size = UI::GetRenderedTextSize(label, font);
            UI::RenderText(app, label, font, white, Vector3(x, height, rect.topLeft.z));

            x += size.x + hgap;
        }

        {   // Input
            static std::string text = "0";

            Vector3 inputPos(x, height, rect.topLeft.z);
            UI::RenderNumericInputi(app, GenUIID(), frameWidth, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);
        }

        height += size.y + vgap;
    }

    {   // Height Input
        f32 x = rect.topLeft.x + hgap;
        Vector2 size;

        {   // Label
            std::string_view label = "Height:\t";
            size = UI::GetRenderedTextSize(label, font);
            UI::RenderText(app, label, font, white, Vector3(x, height, rect.topLeft.z));

            x += size.x + hgap;
        }

        {   // Input
            static std::string text = "0";

            Vector3 inputPos(x, height, rect.topLeft.z);
            UI::RenderNumericInputi(app, GenUIID(), frameHeight, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);
        }

        height += size.y + vgap;
    }

    {   // Spit Button
        std::string_view text = "Split";
        Vector2 size = UI::GetRenderedTextSize(text, font);

        f32 x = (app.refScreenWidth - size.x - 20.0f) / 2.0f;
        if (UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), Vector3(x, height, rect.topLeft.z)))
        {
            SplitSheet(context,  Vector2(frameWidth, frameHeight));
            showSplitSheetDialogue = false;
        }

        height += size.y + vgap;
    }
}

Vector2 RenderAnimationInfo(Application& app, const UI::Font& font, Context& context, const Vector3& topLeft)
{
    Animation& animation = context.CurrentAnimation();

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

    {   // Split Button
        std::string_view text = "Split";
        Vector2 size = UI::GetRenderedTextSize(text, font);

        if (UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), Vector3(topLeft.x - 10.0f, height, topLeft.z)))
            showSplitSheetDialogue = true;

        if (showSplitSheetDialogue)
            RenderSplitSheetDialog(app, font, context);

        height += size.y + vgap;
    }

    f32 x = app.refScreenWidth - 10.0f;

    {   // Delete Button
        std::string_view text = "Delete";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        x -= size.x + 20.0f;

        if (UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), Vector3(x, height, topLeft.z)))
        {
            context.animations.erase_at(context.selectedAnimationIndex);
            context.selectedAnimationIndex = -1;
            context.selectedFrameIndex = -1;
        }
    }

    {   // Rename Button
        std::string_view text = "Rename";
        Vector2 size = UI::GetRenderedTextSize(text, font);
        x -= size.x + 20.0f;

        if (UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), Vector3(x - 10.0f, height, topLeft.z)))
            showRenameAnimationDialogue = true;

        if (showRenameAnimationDialogue)
            RenderRenameAnimationDialog(app, font, context);

        height += 2.0f * vgap;
    }

    return Vector2 { 0.0f, height - topLeft.y };
}

void RenderFrameInfo(Application& app, const UI::Font& font, Context& context)
{
    static bool initialized = false;
    constexpr f32 hgap = 10.0f;
    static f32 numericInputWidth = UI::GetRenderedTextSize("1920", font).x + 20.0f;

    static f32 windowWidth;

    if (!initialized)
    {
        windowWidth = UI::GetRenderedTextSize("X:\t", font).x +
                    numericInputWidth + hgap +
                    UI::GetRenderedTextSize("Y:\t", font).x +
                    numericInputWidth + hgap +
                    UI::GetRenderedTextSize("W:\t", font).x +
                    numericInputWidth + hgap +
                    UI::GetRenderedTextSize("H:\t", font).x +
                    numericInputWidth + hgap +
                    UI::GetRenderedTextSize("Pivot x: ", font).x +
                    numericInputWidth + hgap +
                    UI::GetRenderedTextSize("Pivot Y: ", font).x +
                    numericInputWidth + hgap +
                    UI::GetRenderedTextSize("Delete", font).x + 20.0f
                    ;

        initialized = true;
    }

    AnimationFrame& frame = context.CurrentAnimationFrame();

    static const f32 y = app.refScreenHeight - UI::GetRenderedTextSize("F", font).y - 20.0f;
    f32 x = (app.refScreenWidth - windowWidth) / 2.0f;

    {   // Position X
        std::string_view label = "X:\t";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, Vector3(x, y, 0.0f));

        static std::string text;
        s32 num = frame.topLeft.x;

        Vector3 inputPos(x + size.x, y - 5.0f, 0.0f);
        UI::RenderNumericInputi(app, GenUIID(), num, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);

        frame.topLeft.x = num;

        x += size.x + numericInputWidth + hgap;
    }

    {   // Position Y
        std::string_view label = "Y:\t";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, Vector3(x, y, 0.0f));

        static std::string text;
        s32 num = frame.topLeft.y;

        Vector3 inputPos(x + size.x, y - 5.0f, 0.0f);
        UI::RenderNumericInputi(app, GenUIID(), num, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);


        
        frame.topLeft.y = num;

        x += size.x + numericInputWidth + hgap;
    }

    {   // Width
        std::string_view label = "W:\t";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, Vector3(x, y, 0.0f));

        static std::string text;
        s32 num = frame.size.x;

        Vector3 inputPos(x + size.x, y - 5.0f, 0.0f);
        UI::RenderNumericInputi(app, GenUIID(), num, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);

        frame.size.x = num;

        x += size.x + numericInputWidth + hgap;
    }
    
    {   // Height
        std::string_view label = "H:\t";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, Vector3(x, y, 0.0f));

        static std::string text;
        s32 num = frame.size.y;

        Vector3 inputPos(x + size.x, y - 5.0f, 0.0f);
        UI::RenderNumericInputi(app, GenUIID(), num, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);

        frame.size.y = num;

        x += size.x + numericInputWidth + hgap;
    }

    {   // Pivot X
        std::string_view label = "Pivot X: ";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, Vector3(x, y, 0.0f));

        static std::string text;

        Vector3 inputPos(x + size.x, y - 5.0f, 0.0f);
        UI::RenderNumericInputf(app, GenUIID(), frame.pivot.x, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);
        
        x += size.x + numericInputWidth + hgap;
    }
    
    {   // Pivot Y
        std::string_view label = "Pivot Y: ";
        Vector2 size = UI::GetRenderedTextSize(label, font);
        UI::RenderText(app, label, font, white, Vector3(x, y, 0.0f));

        static std::string text;

        Vector3 inputPos(x + size.x, y - 5.0f, 0.0f);
        UI::RenderNumericInputf(app, GenUIID(), frame.pivot.y, text, font, Vector2(10.0f, 5.0f), inputPos, numericInputWidth);
        
        x += size.x + numericInputWidth + hgap;
    }

    {   // Delete Button
        std::string_view text = "Delete";
        Vector2 size = UI::GetRenderedTextSize(text, font);

        Vector3 position(x, y - 5.0f, 0.0f);
        if (app.GetKeyDown(KEY(DELETE)) || UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), position))
        {
            context.CurrentAnimation().frames.erase_at(context.selectedFrameIndex);
            context.selectedFrameIndex = -1;
        }

        x += size.x + hgap;
    }
}

void ResetAnimations(gn::darray<Animation>& animations)
{
    animations.clear();
    numAnimations = 1;
    defaultName = "animation_0";
}