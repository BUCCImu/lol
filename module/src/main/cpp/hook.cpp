#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cinttypes>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_android.h"
#include "KittyMemory/KittyMemory.h"
#include "KittyMemory/MemoryPatch.h"
#include "KittyMemory/KittyScanner.h"
#include "KittyMemory/KittyUtils.h"
#include "Includes/Dobby/dobbyForHooks.h"
#include "Include/Unity.h"
#include "Misc.h"
#include "hook.h"
#include "Include/Roboto-Regular.h"
#include <iostream>
#include <chrono>
#include "Include/Quaternion.h"
#include "Rect.h"
#include <fstream>
#include <limits>
#include <thread>
#define GamePackageName OBFUSCATE("com.igenesoft.hide")

int glHeight, glWidth;
//dear future self, if this game ever updat


/*
 *
 * COMPILE SETTINGS:
 * - BIGSEX: EXPERIMENTAL FEATURES, NOT FINAL AND SHOULDN'T BE BUILT UNTIL WORKING FINE
 *
 */

uintptr_t find_pattern(uint8_t* start, const size_t length, const char* pattern) {
    const char* pat = pattern;
    uint8_t* first_match = 0;
    for (auto current_byte = start; current_byte < (start + length); ++current_byte) {
        if (*pat == '?' || *current_byte == strtoul(pat, NULL, 16)) {
            if (!first_match)
                first_match = current_byte;
            if (!pat[2])
                return (uintptr_t)first_match;
            pat += *(uint16_t*)pat == 16191 || *pat != '?' ? 3 : 2;
        }
        else if (first_match) {
            current_byte = first_match;
            pat = pattern;
            first_match = 0;
        }
    } return 0;
}

struct lib_info{
    void* start_address;
    void* end_address;
    intptr_t size;
    std::string name;
};

lib_info find_library(const char* module_name) {
    lib_info library_info{};
    char line[512], mod_name[64];

    FILE* fp = fopen("/proc/self/maps", "rt");
    if (fp != nullptr) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                sscanf(line, "%lx-%lx %*s %*s %*s %*d %s",
                       (long unsigned *)&library_info.start_address,
                       (long unsigned*)&library_info.end_address, mod_name);

                library_info.size = reinterpret_cast<uintptr_t>(library_info.end_address) -
                                    reinterpret_cast<uintptr_t>(library_info.start_address);

                if (library_info.name.empty()) {
                    library_info.name = mod_name;
                }

                break;
            }
        }
        fclose(fp);
    }

    return library_info;
}


uintptr_t find_pattern_in_module(const char* lib_name, const char* pattern) {
    lib_info lib_info = find_library(lib_name);
    return find_pattern((uint8_t*)lib_info.start_address, lib_info.size, pattern);
}

#define PHOOK(pattern, ptr, orig) hook((const char*)find_pattern_in_module(OBFUSCATE("libil2cpp.so"), OBFUSCATE(pattern)), (void *)ptr, (void **)&orig)



bool destroy, setMaster, setMasterL, selectMap, changeMapL, destroyL, disconnectMe, reviveAuto, cloneMe, noSkin;

bool (*SetMasterClient) (void* masterClientPlayer);
bool (*CloseConnection) (void* kickPlayer);
void* (*get_LocalPlayer) ();
void (*DestroyPlayerObjects)(void *player);
void (*Disconnect)(void *player);
void (*ReviveHider) ();
void (*RemoveRPCs)(void *player);
monoArray<void**> *(*PhotonNetwork_playerListothers)();


void Pointers() {
    SetMasterClient = (bool(*)(void*)) (g_il2cppBaseMap.startAddress + string2Offset("0xDD9480"));//search SetMasterClient in strings
    CloseConnection = (bool(*)(void*)) (g_il2cppBaseMap.startAddress + string2Offset("0xDD9200"));
    get_LocalPlayer = (void*(*)()) (g_il2cppBaseMap.startAddress + string2Offset("0xDD4838"));//get_LocalPlayer
    DestroyPlayerObjects = (void (*)(void *)) (g_il2cppBaseMap.startAddress + string2Offset("0xDDF0C4"));//search DestroyPlayerObjects in strings
    Disconnect = (void (*)(void *)) (g_il2cppBaseMap.startAddress + string2Offset("0xDD8828"));
    ReviveHider =  (void(*)()) (g_il2cppBaseMap.startAddress + string2Offset("0x1AB28AC"));
    RemoveRPCs = (void (*)(void *)) (g_il2cppBaseMap.startAddress + string2Offset("0xDDF9D0"));
    PhotonNetwork_playerListothers = (monoArray<void **> *(*)()) (g_il2cppBaseMap.startAddress + string2Offset("0xDD4B7C"));//same steps as LocalPlayer, its the 2nd array, so below the first array method you see
}



void (*old_GameManager)(void *obj);
void GameManager(void *obj) {
    if (obj != nullptr) {
        if(destroy){
            auto photonplayers = PhotonNetwork_playerListothers();
            SetMasterClient(get_LocalPlayer());
            for (int i = 0; i < photonplayers->getLength(); ++i) {
                auto photonplayer = photonplayers->getPointer()[i];
                DestroyPlayerObjects(photonplayer);
                CloseConnection(photonplayer);
                RemoveRPCs(photonplayer);
            }
            destroy = false;
        }
        if(destroyL){
            auto photonplayers = PhotonNetwork_playerListothers();
            SetMasterClient(get_LocalPlayer());
            for (int i = 0; i < photonplayers->getLength(); ++i) {
                auto photonplayer = photonplayers->getPointer()[i];
                DestroyPlayerObjects(photonplayer);
                CloseConnection(photonplayer);
                RemoveRPCs(photonplayer);
            }
        }
        if(setMaster){
            SetMasterClient(get_LocalPlayer());
            setMaster = false;
        }
        if(setMasterL){
            SetMasterClient(get_LocalPlayer());
        }
        if(disconnectMe){
            Disconnect(get_LocalPlayer());
            disconnectMe = false;
        }
        if(reviveAuto){
            for(int i = 0; i<100; ++i){
                ReviveHider();
            }
                reviveAuto = false;
        }

    }
    old_GameManager(obj);
}


int isGame(JNIEnv *env, jstring appDataDir) {
    if (!appDataDir)
        return 0;
    const char *app_data_dir = env->GetStringUTFChars(appDataDir, nullptr);
    int user = 0;
    static char package_name[256];
    if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &user, package_name) != 2) {
        if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) != 1) {
            package_name[0] = '\0';
            LOGW(OBFUSCATE("can't parse %s"), app_data_dir);
            return 0;
        }
    }
    if (strcmp(package_name, GamePackageName) == 0) {
        LOGI(OBFUSCATE("detect game: %s"), package_name);
        game_data_dir = new char[strlen(app_data_dir) + 1];
        strcpy(game_data_dir, app_data_dir);
        env->ReleaseStringUTFChars(appDataDir, app_data_dir);
        return 1;
    } else {
        env->ReleaseStringUTFChars(appDataDir, app_data_dir);
        return 0;
    }
}


bool setupimg;

HOOKAF(void, Input, void *thiz, void *ex_ab, void *ex_ac) {
    origInput(thiz, ex_ab, ex_ac);
    ImGui_ImplAndroid_HandleInputEvent((AInputEvent *)thiz);
    return;
}

void Hooks() {
        HOOK("0x1AB7D5C", GameManager, old_GameManager);
}

void Patches() {
    //Toggle
    PATCH_SWITCH("0x1A39F84", "600580D2", selectMap);
    PATCH_SWITCH("0x1413A04", "000080D2C0035FD6", changeMapL);
    PATCH_SWITCH("0xDDD2E0", "21008052", cloneMe);
    PATCH_SWITCH("0x1A7F820", "000080D2", noSkin);
    //PATCH
    PATCH("0x1AB1DA8", "C0035FD6");
    PATCH("0x1B036B4", "C0035FD6");
    PATCH("0x1AB2360", "000080D2");//changeProp
    PATCH("0x1AB2370", "000080D2");//
    PATCH("0x1AB24E8", "21008052");//
    PATCH("0x1AB2ADC", "21008052");//
    PATCH("0x1AB418C", "21008052");//
    PATCH("0x1AD4AAC", "21008052");//
    PATCH("0x1B11F6C", "21008052");//
    PATCH("0x1B223E0", "000080D2");//
    PATCH("0x1B23DF8", "0859202E");//changeProp
    PATCH("0x1AAF448", "0080AFD2C0035FD6");//coin
    PATCH("0x1AB3D74", "1F2003D5");//skinDetection
    PATCH("0x1A3A04C", "1F2003D5");//mapDetection
}

void DrawMenu(){
    static ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    {
        ImGui::Begin(OBFUSCATE("zyCheats Hide.io - BUCCI"));
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
        if (ImGui::BeginTabBar("Menu", tab_bar_flags)) {
            if (ImGui::BeginTabItem(OBFUSCATE("Game"))) {
                if (ImGui::Button(OBFUSCATE("Force MasterClient"))) {
                    setMaster = true;
                }
                ImGui::Checkbox(OBFUSCATE("MasterClient Loop"), &setMasterL);
                if (ImGui::Button(OBFUSCATE("Remove Player"))) {
                    destroy = true;
                }
                ImGui::Checkbox(OBFUSCATE("Remove Player Loop"), &destroyL);
                ImGui::Checkbox(OBFUSCATE("Block World"), &selectMap);
                ImGui::Checkbox(OBFUSCATE("Change Map Loop"), &changeMapL);
                if (ImGui::Button(OBFUSCATE("Disconnect Me"))) {
                    disconnectMe = true;

                    ImGui::EndTabItem();
                }

            }
            if (imGui::BeginTabItem(OBFUSCATE("Player"))) {
                if (ImGui::Button(OBFUSCATE("Revive 100"))) {
                    reviveAuto = true;
                }
                ImGui::Checkbox(OBFUSCATE("Clone"), &cloneMe);
                ImGui::Checkbox(OBFUSCATE("No Skin"), &noSkin);

                ImGui::EndTabItem();
            }
                ImGui::EndTabBar();
            }
            Patches();
        }
    ImGui::End();
}

void SetupImgui() {
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) glWidth, (float) glHeight);
    ImGui_ImplOpenGL3_Init(OBFUSCATE("#version 100"));
    ImGui::StyleColorsDark();

    ImGui::GetStyle().ScaleAllSizes(7.0f);
    io.Fonts->AddFontFromMemoryTTF(Roboto_Regular, 30, 30.0f);
}


EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (!setupimg) {
        SetupImgui();
        setupimg = true;
    }

    Hooks();

    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    DrawMenu();

    ImGui::EndFrame();
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void *hack_thread(void *arg) {
    do {
        sleep(1);
        g_il2cppBaseMap = KittyMemory::getLibraryBaseMap(OBFUSCATE("libil2cpp.so"));
    } while (!g_il2cppBaseMap.isValid());
    Pointers();
    sleep(15);
    auto eglhandle = dlopen(OBFUSCATE("libunity.so"), RTLD_LAZY);
    auto eglSwapBuffers = dlsym(eglhandle, OBFUSCATE("eglSwapBuffers"));
    DobbyHook((void*)eglSwapBuffers,(void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    void *sym_input = DobbySymbolResolver((OBFUSCATE("/system/lib/libinput.so")), (OBFUSCATE("_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE")));
    if (NULL != sym_input) {
        DobbyHook(sym_input,(void*)myInput,(void**)&origInput);
    }
    return nullptr;
}
