/*
    Copyright 2016-2017 StapleButter

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Config.h"
#include <string>
#ifdef _WIN32
#define NTDDI_VERSION		0x06000000 // GROSS FUCKING HACK
#include <windows.h>
//#include <knownfolders.h> // FUCK THAT SHIT
extern "C" const GUID DECLSPEC_SELECTANY FOLDERID_RoamingAppData = {0x3eb685db, 0x65f9, 0x4cf6, {0xa0, 0x3a, 0xe3, 0xef, 0x65, 0x72, 0x9f, 0x3d}};
#include <shlobj.h>
#elif !defined(__SWITCH__)
#include <glib.h>
#endif


namespace Config
{

int KeyMapping[12];
int JoyMapping[12];

int WindowWidth;
int WindowHeight;

int ScreenRotation;
int ScreenGap;
int ScreenLayout;
int ScreenSizing;
int ScreenFilter;

int DirectBoot;

int Threaded3D;

int SocketBindAnyAddr;

typedef struct
{
    char Name[16];
    int Type;
    void* Value;
    int DefaultInt;
    char* DefaultStr;
    int StrLength;

} ConfigEntry;

ConfigEntry ConfigFile[] =
{
    {"Key_A",      0, &KeyMapping[0],   32, NULL, 0},
    {"Key_B",      0, &KeyMapping[1],   31, NULL, 0},
    {"Key_Select", 0, &KeyMapping[2],   57, NULL, 0},
    {"Key_Start",  0, &KeyMapping[3],   28, NULL, 0},
    {"Key_Right",  0, &KeyMapping[4],  333, NULL, 0},
    {"Key_Left",   0, &KeyMapping[5],  331, NULL, 0},
    {"Key_Up",     0, &KeyMapping[6],  328, NULL, 0},
    {"Key_Down",   0, &KeyMapping[7],  336, NULL, 0},
    {"Key_R",      0, &KeyMapping[8],   54, NULL, 0},
    {"Key_L",      0, &KeyMapping[9],   42, NULL, 0},
    {"Key_X",      0, &KeyMapping[10],  17, NULL, 0},
    {"Key_Y",      0, &KeyMapping[11],  30, NULL, 0},

    {"Joy_A",      0, &JoyMapping[0],  -1, NULL, 0},
    {"Joy_B",      0, &JoyMapping[1],  -1, NULL, 0},
    {"Joy_Select", 0, &JoyMapping[2],  -1, NULL, 0},
    {"Joy_Start",  0, &JoyMapping[3],  -1, NULL, 0},
    {"Joy_Right",  0, &JoyMapping[4],  -1, NULL, 0},
    {"Joy_Left",   0, &JoyMapping[5],  -1, NULL, 0},
    {"Joy_Up",     0, &JoyMapping[6],  -1, NULL, 0},
    {"Joy_Down",   0, &JoyMapping[7],  -1, NULL, 0},
    {"Joy_R",      0, &JoyMapping[8],  -1, NULL, 0},
    {"Joy_L",      0, &JoyMapping[9],  -1, NULL, 0},
    {"Joy_X",      0, &JoyMapping[10], -1, NULL, 0},
    {"Joy_Y",      0, &JoyMapping[11], -1, NULL, 0},

    {"WindowWidth",  0, &WindowWidth,  256, NULL, 0},
    {"WindowHeight", 0, &WindowHeight, 384, NULL, 0},

    {"ScreenRotation", 0, &ScreenRotation, 0, NULL, 0},
    {"ScreenGap",      0, &ScreenGap,      0, NULL, 0},
    {"ScreenLayout",   0, &ScreenLayout,   0, NULL, 0},
    {"ScreenSizing",   0, &ScreenSizing,   0, NULL, 0},
    {"ScreenFilter",   0, &ScreenFilter,   1, NULL, 0},

    {"DirectBoot", 0, &DirectBoot, 1, NULL, 0},

    {"Threaded3D", 0, &Threaded3D, 1, NULL, 0},

    {"SockBindAnyAddr", 0, &SocketBindAnyAddr, 0, NULL, 0},

    {"", -1, NULL, 0, NULL, 0}
};

FILE* GetConfigFile(const char* fileName, const char* permissions)
{
    // Locations are application directory, and XDG_CONFIG_HOME/melonds or AppData/MelonDS on windows

    FILE* f;

    // First check application directory
    f = fopen(fileName, permissions);
    if (f) return f;
#ifdef _WIN32
    // Now check AppData
    PWSTR appDataPath = NULL;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath);
    if (!appDataPath)
        return NULL;
    CoTaskMemRealloc(appDataPath, (wcslen(appDataPath)+9+strlen(fileName)+1)*sizeof(WCHAR));

    // this will be more than enough
    WCHAR fatperm[4];
    fatperm[0] = permissions[0];
    fatperm[1] = permissions[1];
    fatperm[2] = permissions[2];
    fatperm[3] = 0;

    f = _wfopen(appDataPath, fatperm);
    CoTaskMemFree(appDataPath);
    if (f) return f;
#elif defined(__SWITCH__)
    std::string path = std::string("sdmc:/switch/melonds/") + fileName;
    f = fopen(path.c_str(), permissions);
    if (f) return f;
#else
    // Now check XDG_CONFIG_HOME
    // TODO: check for memory leak there
    std::string path = std::string(g_get_user_config_dir()) + "/melonds/" + fileName;
    f = fopen(path.c_str(), permissions);
    if (f) return f;
#endif

    return NULL;

}

bool HasConfigFile(const char* fileName)
{
    FILE* f = GetConfigFile(fileName, "rb");
    if (f)
    {
        fclose(f);
        return true;
    }
    else
        return false;
}

void Load()
{
    ConfigEntry* entry = &ConfigFile[0];
    for (;;)
    {
        if (!entry->Value) break;

        if (entry->Type == 0)
            *(int*)entry->Value = entry->DefaultInt;
        else
            strncpy((char*)entry->Value, entry->DefaultStr, entry->StrLength);

        entry++;
    }

    FILE* f = Config::GetConfigFile("melonDS.ini", "r");
    if (!f) return;

    char linebuf[1024];
    char entryname[16];
    char entryval[1024];
    while (!feof(f))
    {
        fgets(linebuf, 1024, f);
        int ret = sscanf(linebuf, "%15[A-Za-z_0-9]=%[^\t\n]", entryname, entryval);
        if (ret < 2) continue;

        ConfigEntry* entry = &ConfigFile[0];
        for (;;)
        {
            if (!entry->Value) break;

            if (!strncmp(entry->Name, entryname, 15))
            {
                if (entry->Type == 0)
                    *(int*)entry->Value = strtol(entryval, NULL, 10);
                else
                    strncpy((char*)entry->Value, entryval, entry->StrLength);

                break;
            }

            entry++;
        }
    }

    fclose(f);
}

void Save()
{
    FILE* f = Config::GetConfigFile("melonDS.ini", "w");
    if (!f) return;

    ConfigEntry* entry = &ConfigFile[0];
    for (;;)
    {
        if (!entry->Value) break;

        if (entry->Type == 0)
            fprintf(f, "%s=%d\n", entry->Name, *(int*)entry->Value);
        else
            fprintf(f, "%s=%s\n", entry->Name, entry->Value);

        entry++;
    }

    fclose(f);
}


}
