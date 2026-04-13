#include <windows.h>
#include <iostream>
#include <psapi.h>

uintptr_t FindPattern(const char* module, const char* pattern, const char* mask) {
    HMODULE hModule = GetModuleHandleA(module);
    if (!hModule) return 0;
    MODULEINFO mInfo;
    GetModuleInformation(GetCurrentProcess(), hModule, &mInfo, sizeof(mInfo));
    uintptr_t base = (uintptr_t)mInfo.lpBaseOfDll;
    uintptr_t size = (uintptr_t)mInfo.SizeOfImage;
    size_t patternLength = strlen(mask);
    for (uintptr_t i = 0; i < size - patternLength; i++) {
        bool found = true;
        for (uintptr_t j = 0; j < patternLength; j++) {
            if (mask[j] != '?' && pattern[j] != *(char*)(base + i + j)) {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return 0;
}

void MainThread(HMODULE hModule) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    std::cout << "[!] Injected\n";

    uintptr_t glowSig = FindPattern("client.dll", "\x0F\x11\x05\x00\x00\x00\x00\x83\xC8\x01", "xxx????xxx");
    
    if (glowSig) {
        uintptr_t glowManagerAddr = *(uintptr_t*)(glowSig + 3);

        while (!(GetAsyncKeyState(VK_END) & 1)) {
            uintptr_t currentArray = *(uintptr_t*)(glowManagerAddr);
            int currentCount = *(int*)(glowManagerAddr + 0x4);

            if (currentArray < 0x1000 || currentCount <= 0) continue;

            for (int i = 0; i < currentCount; i++) {
				uintptr_t objAddr = currentArray + (i * 0x38);

				if (IsBadReadPtr((void*)objAddr, 4)) continue;
				uintptr_t entity = *(uintptr_t*)(objAddr);

				if (entity < 0x10000) continue;

				*(float*)(objAddr + 0x4)  = 0.0f; 
				*(float*)(objAddr + 0x8)  = 1.0f; 
				*(float*)(objAddr + 0xC)  = 0.0f; 
				*(float*)(objAddr + 0x10) = 1.0f;
				*(float*)(objAddr + 0x14) = 1.0f; 

				*(bool*)(objAddr + 0x24) = true; 
				*(bool*)(objAddr + 0x25) = false;
				*(bool*)(objAddr + 0x26) = true; 

				*(float*)(objAddr + 0x18) = 1.0f; 
			}
        }
    }
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) 
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
    return TRUE;
}