#ifdef DEBUG
#include <iostream>
#endif

#include <algorithm>
#include <string_view>
#include <cstring>

#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/application.h"
#include "program/animation.h"
#include "program/background.h"
#include "program/colors.h"
#include "program/file_dialog.h"
#include "program/interface.h"
#include "program/json_io.h"
#include "program/context.h"

UI::Font font;

BackgroundTexture bg;

Vector3 imagePosition, imageStartPos;
f32 scale = 3.0f;

bool isDragging = false;
UI::Rect drawingRect;

f32 maxNameWidth, maxAllowedNameLength = 15;

Context context;

inline bool MouseInRect(Application& app, f32 x, f32 y, f32 w, f32 h)
{
    return app.mouseX >= x && app.mouseX <= x + w &&
           app.mouseY >= y && app.mouseY <= y + h;
}

int main()
{
#   ifdef DEBUG
    Application app("Spedit-Debug", 1028, 720, false);
    app.SetVsync(false);
#   else
    Application app("Spedit", 1920, 1080, false);
    app.SetMaximize(true);
    app.SetVsync(true);
#   endif

    app.SetClearColor(Vector4(0.15f, 0.15f, 0.15f, 1.0f));
    app.SetWindowIcon("res/icons/spedit-icon.png");

    app.onInit = [](Application& app)
    {
        font.Load("res/fonts/Lato-Regular.ttf", 25.0f);

        bg.CreateDefault();
        bg.SetScale(scale);

        imagePosition.x = (app.refScreenWidth -  bg.image.scaledWidth)  / 2.0f;
        imagePosition.y = (app.refScreenHeight - bg.image.scaledHeight) / 2.0f;
        imagePosition.z = 0.1f;

        maxNameWidth = UI::GetRenderedTextSize("Loop:\tPing Pong", font).x + 10.0f;
    };

    app.onUpdate = [](Application& app)
    {
#       ifdef DEBUG
        if (app.GetKeyDown(KEY(ESCAPE)))
        {
            app.Exit();
            return;
        }
#       endif

        {   // Dragging Canvas Controls
            static Vector2 mouseStartPos;

            if (app.GetMouseButtonDown(MOUSE(3)) ||
                (app.GetKey(KEY(SPACE)) && app.GetMouseButtonDown(MOUSE(1))))
            {
                mouseStartPos.x = app.mouseX;
                mouseStartPos.y = app.mouseY;

                imageStartPos = imagePosition;
            } else if (app.GetMouseButton(MOUSE(3)) ||
                       (app.GetKey(KEY(SPACE)) && app.GetMouseButton(MOUSE(1))))
            {
                imagePosition.x = (app.mouseX - mouseStartPos.x) + imageStartPos.x;
                imagePosition.y = (app.mouseY - mouseStartPos.y) + imageStartPos.y;
            }
        }

        if (context.selectedAnimationIndex > -1)
        {   // Dragging Frame Rect
            static Vector2 mouseStartPos;

            if (app.GetKey(KEY(LEFT_CONTROL)) && app.GetMouseButtonDown(MOUSE(1)))
            {
                if (MouseInRect(app, imagePosition.x, imagePosition.y, bg.image.scaledWidth, bg.image.scaledHeight))
                {
                    mouseStartPos.x = std::max((f32) app.mouseX, imagePosition.x);
                    mouseStartPos.y = std::max((f32) app.mouseY, imagePosition.y);
                    isDragging = true;
                    context.selectedFrameIndex = -1;
                }
            } else if (isDragging && app.GetMouseButton(MOUSE(1)))
            {
                f32 x = std::max(std::min(mouseStartPos.x, (f32) app.mouseX), imagePosition.x);
                f32 y = std::max(std::min(mouseStartPos.y, (f32) app.mouseY), imagePosition.y);

                drawingRect.topLeft.x = floorf((x - imagePosition.x) / scale);
                drawingRect.topLeft.y = floorf((y - imagePosition.y) / scale);

                f32 mouseOffsetX = drawingRect.topLeft.x - imagePosition.x;
                f32 mouseOffsetY = drawingRect.topLeft.y - imagePosition.y;

                f32 w = std::min(std::abs((f32) app.mouseX - mouseStartPos.x), bg.image.scaledWidth - mouseOffsetX);
                f32 h = std::min(std::abs((f32) app.mouseY - mouseStartPos.y), bg.image.scaledHeight - mouseOffsetY);

                drawingRect.size.x = roundf(w / scale + 0.35f);
                drawingRect.size.y = roundf(h / scale + 0.35f);
            }

            if (isDragging && app.GetMouseButtonDown(MOUSE(2)))
                isDragging = false;

            if (isDragging && app.GetMouseButtonUp(MOUSE(1)))
            {
                if (drawingRect.size.x > 0.0f && drawingRect.size.y > 0.0f)
                {
                    context.selectedFrameIndex = context.CurrentAnimation().frames.size();
                    
                    AnimationFrame& frame = context.CurrentAnimation().frames.emplace_back();
                    frame.topLeft.x = drawingRect.topLeft.x;
                    frame.topLeft.y = drawingRect.topLeft.y;

                    frame.size.x = drawingRect.size.x;
                    frame.size.y = drawingRect.size.y;

                    frame.pivot.x = frame.pivot.y = 0.5f;
                }

                isDragging = false;
            }
        }
    };

    app.onRender = [](Application& app)
    {
        UI::Begin();

        {   // Render Image
            UI::RenderImage(app, bg.image, imagePosition);

            if (context.imageLoaded)
            {
                UI::RenderImage(app, context.image, imagePosition);

                if (context.selectedAnimationIndex > -1)
                {
                    for (int i = 0; i < context.CurrentAnimation().frames.size(); i++)
                    {
                        AnimationFrame& frame = context.CurrentAnimation().frames[i];
                        UI::Rect displayRect;
                    
                        displayRect.topLeft.x = scale * frame.topLeft.x + imagePosition.x;
                        displayRect.topLeft.y = scale * frame.topLeft.y + imagePosition.y;
                    
                        displayRect.size = scale * frame.size;

                        const Vector4& color = (i == context.selectedFrameIndex) ? orange : green;
                        if (UI::RenderButton(app, GenUIIDWithSec(i), displayRect, color, lgreen, orange))
                            context.selectedFrameIndex = (i == context.selectedFrameIndex) ? -1 : i;
                    }

                    if (isDragging)
                    {   //  Render current frame
                        UI::Rect displayRect;
                    
                        displayRect.topLeft.x = scale * drawingRect.topLeft.x + imagePosition.x;
                        displayRect.topLeft.y = scale * drawingRect.topLeft.y + imagePosition.y;
                    
                        displayRect.size = scale * drawingRect.size;

                        UI::RenderRect(app, displayRect, red);
                    }
                }
            }
        }

        {   // Meta data and Open and Save Options
            char buffer[128];
            sprintf(buffer, "File: %s\nSize: %d x %d px", context.filename.c_str(), context.image.width, context.image.height);
            UI::RenderTextBox(app, buffer, font, white, grey, Vector2(10.0f, 5.0f), Vector3(10.0f, 10.0f, 0.0f));

            f32 height = UI::GetRenderedTextSize(buffer, font).y;

            if (UI::RenderTextButton(app, GenUIID(), "Open", font, { 10.0f, 5.0f }, { 10.0f, height + 30.0f, 0.0f }))
            {
                std::string newpath = std::move(OpenFileDialog());
                bool newImageLoaded = !newpath.empty();

                if (newImageLoaded)
                {
                    // Should use string_view but welp
                    std::string extension = newpath.substr(newpath.find_last_of('.'));

                    if (extension == ".json")
                        context.imageLoadError = !LoadFromJSONFile(newpath, context);
                    else
                    {
                        ResetAnimations(context.animations);

                        context.imageLoadError = false;
                        context.fullpath = std::move(newpath);

                        ResetAnimations(context.animations);
                        
                        UI::Image temp;
                        context.imageLoadError = !temp.Load(context.fullpath.c_str());

                        if (!context.imageLoadError)
                        {
                            if (newImageLoaded)
                                context.image.Free();

                            context.image = temp;
                        }
                    }

                    context.imageLoaded = context.imageLoaded || newImageLoaded;
                
                    if (!context.imageLoadError)
                    {
                        context.selectedFrameIndex = context.selectedAnimationIndex = -1;

                        bg.Free();

                        scale = 3.0f;
                        context.image.SetScale(scale);

                        bg.Create(context.image.width, context.image.height);
                        bg.SetScale(scale);

                        imagePosition.x = (app.refScreenWidth -  context.image.scaledWidth)  / 2.0f;
                        imagePosition.y = (app.refScreenHeight - context.image.scaledHeight) / 2.0f;

                        size_t start = context.fullpath.find_last_of('\\');
                        context.filename = context.fullpath.substr(start + 1);

    #                   ifdef DEBUG
                        std::string windowName = "Spedit-Debug : ";
    #                   else
                        std::string windowName = "Spedit : ";
    #                   endif

                        windowName += context.filename;
                        app.SetWindowTitle(windowName.c_str());

    #                   ifdef DEBUG
                        std::cout << "Opening File: " << context.filename << std::endl;
    #                   endif
                    }

                }
            }

            Vector2 size = UI::GetRenderedTextSize("Open", font);

            if (context.imageLoaded && UI::RenderTextButton(app, GenUIID(), "Save", font, { 10.0f, 5.0f }, { 40.0f + size.x, height + 30.0f, 0.0f }))
                OutputToJSONFile(context);
        }

        if (context.imageLoaded)
        {   // Animation data
            const f32 vgap = 10.0f;
            const f32 hgap = 20.0f;
            f32 height = vgap;

            {   // New Animation Panel
                std::string_view text = "New Animation";
                Vector2 size = UI::GetRenderedTextSize(text, font);
                if (UI::RenderTextButton(app, GenUIID(), text, font, Vector2(10.0f, 5.0f), Vector3(app.refScreenWidth - size.x - 10.0f - hgap, height, 0.0f)))
                {
                    ShowNewAnimationDialog();
                }

                height += size.y + 3.0f * vgap;
            }

            {   // List all animations
                for (int i = 0; i < context.animations.size(); i++)
                {
                    std::string_view name = context.animations[i].name;

                    Vector2 size = UI::GetRenderedTextSize(name, font);
                    Vector3 topLeft(app.refScreenWidth - maxNameWidth - 10.0f - hgap, height, 0.0f);

                    // Render a marker for selected animation
                    if (i == context.selectedAnimationIndex)
                    {
                        static std::string_view marker = "> ";
                        static f32 offset = UI::GetRenderedTextSize(marker, font).x;
                        Vector3 tl = topLeft;
                        tl.x -= offset;
                        tl.y += 5.0f;
                        UI::RenderText(app, marker, font, white, tl);
                    }

                    std::string concatenatedName = context.animations[i].name;
                    if (name.length() > maxAllowedNameLength)
                    {
                        concatenatedName.erase(concatenatedName.begin() + maxAllowedNameLength, concatenatedName.end());
                        concatenatedName.append("...");
                    }

                    if (UI::RenderTextButton(app, GenUIIDWithSec(i), concatenatedName, font, Vector2(10.0f, 5.0f), topLeft))
                    {
                        if (i == context.selectedAnimationIndex)
                            context.selectedAnimationIndex = -1;
                        else
                            context.selectedAnimationIndex = i;

                        context.selectedFrameIndex = -1;
                    }

                    height += size.y + vgap;

                    if (i == context.selectedAnimationIndex)
                    {
                        Vector2 size = RenderAnimationInfo(app, font, context, Vector3(topLeft.x + hgap / 2.0f, height, 0.0f));

                        // If animation gets deleted, then walk back one step
                        if (context.selectedAnimationIndex == -1)
                            i--;

                        height += size.y + vgap;
                    }

                    height += 10.0f;
                }
            }

            if (context.selectedFrameIndex != -1)
            {   // List Animation Frame Data
                RenderFrameInfo(app, font, context);
            }

            if (RenderNewAnimationDialog(app, font, context))
            {
                std::string_view name = context.animations[context.animations.size() - 1].name;
                
                if (name.length() > maxAllowedNameLength)
                {
                    maxNameWidth = UI::GetRenderedTextSize(name.substr(0, maxAllowedNameLength), font).x;
                    maxNameWidth += UI::GetRenderedTextSize("...", font).x;
                }
                else
                {
                    Vector2 size = UI::GetRenderedTextSize(name, font);
                    maxNameWidth = std::max(size.x, maxNameWidth);
                }

                context.selectedAnimationIndex = context.animations.size() - 1;
                context.selectedFrameIndex = -1;
            }
        }

#       ifdef DEBUG
        {   // Frame count
            static u64 frameCount = 0;
            static f64 prevTick = app.time;
            static f64 frameTime = app.deltaTime;

            if (app.time - prevTick >= 1.0)
            {
                frameTime = (app.time - prevTick) / (f64) frameCount;
                frameCount = 0;
                prevTick = app.time;
            }

            static char buffer[256];
            sprintf(buffer, "Frame Time: %fms, Frame Rate: %.0f", frameTime, 1.0 / frameTime);

            Vector3 topLeft(0.0f, app.refScreenHeight - 0.8f * font.fontHeight, 0.0f);
            UI::RenderTextBox(app, buffer, font, white, grey, Vector2(), topLeft);

            frameCount++;
        }
#       else
        if (context.imageLoadError)
        {   // Show Error if image couldn't be loaded
            std::string_view text = "Error loading file...";
            Vector2 size = UI::GetRenderedTextSize(text, font);
            Vector3 topLeft((app.refScreenWidth - size.x) / 2.0f, 10.0f, 0.0f);
            UI::RenderTextBox(app, text, font, white, red, Vector2(10.0f, 5.0f), topLeft);
        }
#       endif

        UI::End();
    };

    app.scrollCallback = [](Application& app, f64 scrollX, f64 scrollY)
    {
        if (isDragging)
            return;
            
        constexpr f32 scrollSpeed = 1.0f;

        f32 relativePosX = (app.mouseX - imagePosition.x) / bg.image.scaledWidth;
        f32 relativePosY = (app.mouseY - imagePosition.y) / bg.image.scaledHeight;

        imagePosition.x = imagePosition.x + (bg.image.scaledWidth * relativePosX);
        imagePosition.y = imagePosition.y + (bg.image.scaledHeight * relativePosY);

        scale += scrollY * scrollSpeed * (0.15f * scale);
        scale = Clamp(scale, 0.5f, 50.0f);

        context.image.SetScale(scale);
        bg.SetScale(scale);

        imagePosition.x = imagePosition.x - (bg.image.scaledWidth * relativePosX);
        imagePosition.y = imagePosition.y - (bg.image.scaledHeight * relativePosY);
    };

    app.Run();

    // OS can do the cleanup
}