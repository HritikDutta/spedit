#include "json_io.h"

#ifdef DEBUG
#include <iostream>
#endif

#include <string>
#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/fileio.h"
#include "animation.h"
#include "json/parser.h"

constexpr char fileFormatStart[] =
"{\n"
"    \"directory\": \"%s\",\n"
"    \"file\": \"%s\",\n"
"    \"animations\": ["
;

constexpr char animationFormatStart[] =
"        {\n"
"            \"name\": \"%s\",\n"
"            \"loopType\": \"%s\",\n"
"            \"frameRate\": %f,\n"
"            \"frames\": ["
;

constexpr char frameFormat[] =
"                {\n"
"                    \"x\": %d,\n"
"                    \"y\": %d,\n"
"                    \"w\": %d,\n"
"                    \"h\": %d,\n"
"                    \"pivot_x\": %f,\n"
"                    \"pivot_y\": %f\n"
"                }"
;

constexpr char animationFormatEnd[] =
"]\n"
"        }"
;

constexpr char fileFormatEnd[] =
"]\n"
"}"
;

std::string EscapedString(const std::string& str)
{
    std::string res;

    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] == '\\')
            res.push_back('\\');
        
        res.push_back(str[i]);
    }

    return std::move(res);
}

void OutputToJSONFile(const std::string& fullpath, const std::string& filename, const gn::darray<Animation>& animations)
{
#   ifdef DEBUG
    std::cout << "Outputing file for " << filename << std::endl;
#   endif

    size_t lastSlash = fullpath.find_last_of('\\');
    std::string directory = fullpath.substr(0, lastSlash);

    size_t lastDot = fullpath.find_last_of('.');
    std::string outfileName = fullpath.substr(0, lastDot) + ".json";

    FILE* outfile = fopen(outfileName.c_str(), "wb");
    ASSERT(outfile);

    fprintf(outfile, fileFormatStart, EscapedString(directory).c_str(), filename.c_str());

    for (int i = 0; i < animations.size(); i++)
    {
        if (i > 0)
            fprintf(outfile, ",\n");
        else
            fprintf(outfile, "\n");

        const Animation& animation = animations[i];
        fprintf(outfile, animationFormatStart, animation.name.c_str(), animation.GetLoopTypeName(), animation.frameRate);

        for (int j = 0; j < animation.frames.size(); j++)
        {
            if (j > 0)
                fprintf(outfile, ",\n");
            else
                fprintf(outfile, "\n");
            
            const AnimationFrame& frame = animation.frames[j];
            fprintf(outfile, frameFormat, (int) frame.topLeft.x, (int) frame.topLeft.y, (int) frame.size.x, (int) frame.size.y, frame.pivot.x, frame.pivot.y);
        }

        if (animation.frames.size() > 0)
            fprintf(outfile, "\n            ");

        fprintf(outfile, animationFormatEnd);
    }

    if (animations.size() > 0)
        fprintf(outfile, "\n    ");

    fprintf(outfile, fileFormatEnd);

    fclose(outfile);
}

bool LoadFromJSONFile(const std::string& jsonfile, std::string& fullpath, std::string& filename, gn::darray<Animation>& animations, UI::Image& image, bool imageLoaded)
{
    std::string json = LoadFile(jsonfile);

    json::Document document;
    if (!json::ParseFile(json, document))
        return false;

    auto docObject = document.start().object();

    std::string directory = docObject["directory"].string();
    filename = docObject["file"].string();
    fullpath = directory + '\\' + filename;

    UI::Image temp;
    if (!temp.Load(fullpath))
        return false;

    animations.clear();

    image = temp;

    for (auto& animObject : docObject["animations"].array())
    {
        auto& name = animObject["name"].string();
        Animation& animation = animations.emplace_back(name);

        auto& loopTypeName = animObject["loopType"].string();
        if (loopTypeName == "None")
            animation.loopType = Animation::LoopType::NONE;
        else if (loopTypeName == "Cycle")
            animation.loopType = Animation::LoopType::CYCLE;
        else // if (loopTypeName == "Ping Pong")
            animation.loopType = Animation::LoopType::PING_PONG;

        animation.frameRate = animObject["frameRate"].float64();

        for (auto& frameObject : animObject["frames"].array())
        {
            AnimationFrame& frame = animation.frames.emplace_back();

            frame.topLeft.x = frameObject["x"].int64();
            frame.topLeft.y = frameObject["y"].int64();
            frame.size.x = frameObject["w"].int64();
            frame.size.y = frameObject["h"].int64();

            frame.pivot.x = frameObject["pivot_x"].float64();
            frame.pivot.y = frameObject["pivot_y"].float64();
        }
    }

    return true;
}