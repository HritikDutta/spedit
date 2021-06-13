#include "file_dialog.h"

#ifdef DEBUG
#include <iostream>
#endif
#include <string>
#include <windows.h>

std::string OpenFileDialog()
{
    char filter[] = "All Files (*.*)\0*.*\0PNG (*.PNG)\0*.PNG\0JPEG (*.JPG, *.JPEG)\0*.JPG;*.JPEG\0\0";
    char fnameBuffer[MAX_PATH] = "";

    OPENFILENAME ofn {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile   = fnameBuffer;
    ofn.nMaxFile    = MAX_PATH;

    if (GetOpenFileName(&ofn))
        return std::move(std::string(fnameBuffer));

#   ifdef DEBUG
    std::cout << "Couldn't open file\n";
#   endif

    return "";
}