#include "json_io.h"

#ifdef DEBUG
#include <iostream>
#endif

#include <string>
#include "containers/darray.h"
#include "engine/ui.h"
#include "platform/fileio.h"
#include "animation.h"
#include "json.h"

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
    JSON::Value sheet = JSON::LoadJSONFile(jsonfile);

    std::string directory = sheet["directory"].GetString();
    filename = sheet["file"].GetString();
    fullpath = directory + '\\' + filename;

    UI::Image temp;
    if (!temp.Load(fullpath))
        return false;

    animations.clear();

    image = temp;

    JSON::Value& array = sheet["animations"];
    for (int ai = 0; ai < array.GetArraySize(); ai++)
    {
        JSON::Value& animData = array[ai];

        std::string name = animData["name"].GetString();
        Animation& animation = animations.emplace_back(name);

        std::string loopTypeName = animData["loopType"].GetString();
        if (loopTypeName == "None")
            animation.loopType = Animation::LoopType::NONE;
        else if (loopTypeName == "Cycle")
            animation.loopType = Animation::LoopType::CYCLE;
        else // if (loopTypeName == "Ping Pong")
            animation.loopType = Animation::LoopType::PING_PONG;

        animation.frameRate = animData["frameRate"].GetFloat();

        JSON::Value& frameArray = animData["frames"];
        for (int fi = 0; fi < frameArray.GetArraySize(); fi++)
        {
            JSON::Value& frameData = frameArray[fi];
            AnimationFrame& frame = animation.frames.emplace_back();

            frame.topLeft.x = frameData["x"].GetInt();
            frame.topLeft.y = frameData["y"].GetInt();
            frame.size.x = frameData["w"].GetInt();
            frame.size.y = frameData["h"].GetInt();

            frame.pivot.x = frameData["pivot_x"].GetFloat();
            frame.pivot.y = frameData["pivot_y"].GetFloat();
        }
    }

    sheet.Free();
    return true;
}