#include <windows.h>
#include "pluginsdk\_plugins.h"
#include "pluginsdk\_exports.h" // modified _exports.h to use _dbg_addrinfoget export
#include "pluginsdk\bridgemain.h"
#include "utf8\utf8.h"
#include "resource.h"
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

#define plugin_version 1

int pluginHandle;
HWND hwndDlg;
HINSTANCE hModule;

std::string LoadUTF8String(int index)
{
    wchar_t p[512];
    int len;
    memset(p, 0, sizeof(p));
    if((len = LoadString(hModule, index, (LPWSTR)p, 512)) == 0)
    {
        return "";
    }
    else
    {
        std::wstring utf16line(p, len);
        std::string utf8line;
        utf8::utf16to8(utf16line.begin(), utf16line.end(), std::back_inserter(utf8line));
        return utf8line;
    }
}

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
        info.refcount = 0;
        info.references = nullptr;
        if(DbgXrefGet(param->VA, &info) && info.refcount > 0)
        {
            std::string output;
            std::qsort(info.references, info.refcount, sizeof(info.references[0]), &compareFunc);
            int t = XREF_NONE;
            duint i;
            for(i = 0; i < info.refcount && i < 10; i++)
            {
                if(t != info.references[i].type)
                {
                    switch(info.references[i].type)
                    {
                    case XREF_JMP:
                        output += LoadUTF8String(IDS_JMPFROM);
                        break;
                    case XREF_CALL:
                        output += LoadUTF8String(IDS_CALLFROM);
                        break;
                    default:
                        output += LoadUTF8String(IDS_REFFROM);
                        break;
                    }
                    t = info.references[i].type;
                }
                ADDRINFO label;
                label.flags = flaglabel;
                _dbg_addrinfoget(info.references[i].addr, SEG_DEFAULT, &label);
                if(label.label[0] != '\0')
                    output += label.label;
                else{
                    char temp[18];
                    sprintf_s(temp, "%p", info.references[i].addr);
                    output += temp;
                }
                if(i != info.refcount - 1)
                    output += ",";
            }
            if(info.refcount > 10)
                output += " ...";
            GuiAddInfoLine(output.c_str());
            if(info.references)
                BridgeFree(info.references);
        }
    }
}

DLL_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = plugin_version;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strcpy_s(initStruct->pluginName, LoadUTF8String(IDS_PLUGINAME).c_str());
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
    {
        hModule = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
