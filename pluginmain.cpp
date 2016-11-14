#include <windows.h>
#include "pluginsdk\_plugins.h"
#include "pluginsdk\_exports.h" // modified _exports.h to use _dbg_addrinfoget export
#include "pluginsdk\bridgemain.h"
#ifdef _WIN64
#pragma comment(lib, "pluginsdk\\x64dbg.lib")
#pragma comment(lib, "pluginsdk\\x64bridge.lib")
#else
#pragma comment(lib, "pluginsdk\\x32dbg.lib")
#pragma comment(lib, "pluginsdk\\x32bridge.lib")
#endif

#ifndef DLL_EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)
#endif //DLL_EXPORT

#define plugin_name "ExtraInfo"
#define plugin_version 1

int pluginHandle;
HWND hwndDlg;

int compareFunc(const void* a, const void* b)
{
    XREF_RECORD* A, *B;
    A = (XREF_RECORD*)a;
    B = (XREF_RECORD*)b;
    if(A->type > B->type)
        return 1;
    else if(A->type < B->type)
        return -1;
    else if(A->addr > B->addr)
        return 1;
    else if(A->addr < B->addr)
        return -1;
    else
        return 0;
}

void cbPlugin(CBTYPE cbType, LPVOID generic_param)
{
    PLUG_CB_SELCHANGED* param = (PLUG_CB_SELCHANGED*)generic_param;
    if(param->hWindow == GUI_DISASSEMBLY)
    {
        XREF_INFO info;
        if(DbgXrefGet(param->VA, &info) && info.refcount > 0)
        {
            std::string output;
            std::qsort(info.references, info.refcount, sizeof(info.references[0]), &compareFunc);
            int t = -1;
            for(int i = 0; i < info.refcount; i++)
            {
                if(t != info.references[i].type)
                {
                    if(t != -1)
                        output += ",";
                    switch(info.references[i].type)
                    {
                    case XREF_JMP:
                        output += "Jump from ";
                        break;
                    case XREF_CALL:
                        output += "Call from ";
                        break;
                    default:
                        output += "Reference from ";
                        break;
                    }
                    t = info.references[i].type;
                }
                ADDRINFO label;
                label.flags = flaglabel;
                _dbg_addrinfoget(info.references[i].addr, SEG_DEFAULT, &label);
                output += label.label;
                if(i != info.refcount - 1)
                    output += ",";
            }
            GuiAddInfoLine(output.c_str());
        }
    }
}

DLL_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = plugin_version;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strcpy(initStruct->pluginName, plugin_name);
    pluginHandle = initStruct->pluginHandle;
    return true;
}

DLL_EXPORT bool plugstop()
{
    return true;
}

DLL_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct)
{
    hwndDlg = setupStruct->hwndDlg;
    _plugin_registercallback(pluginHandle, CB_SELCHANGED, &cbPlugin);
}

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);
    return TRUE;
}
