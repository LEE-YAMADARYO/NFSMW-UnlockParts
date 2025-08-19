// Copyright(C) 2025 YAMADA RYO
// 此 ASI 插件用于 Need for Speed Most Wanted 2005，它可以让所有外观套件在存档创建之初全部解锁
// 该代码根据 MIT 许可证发布
// 著作权所有
#include "pch.h"
#include <windows.h>
#include <cstdio>
#include <memory>

// 定义原始函数签名的 typedef
typedef char(__cdecl* tGenericUnlockFunc)();

// 指向原始函数的指针
tGenericUnlockFunc originalIsCarPartUnlocked = NULL;
tGenericUnlockFunc originalIsPerfPackageUnlocked = NULL;

// 自定义挂钩函数

// 强制解锁所有 CarPart (外观部件)
char __cdecl MyIsCarPartUnlockedHook() {
    return 1; // 总是返回 True (已解锁)
}

// 强制锁定所有性能部件
char __cdecl MyIsPerfPackageUnlockedHook() {
    return 0; // 总是返回 False (未解锁)
}

// 用于创建跳板钩子的辅助变量，此数组用于存储原始函数被覆盖的字节
BYTE originalBytes_IsCarPartUnlocked[6];
BYTE originalBytes_IsPerfPackageUnlocked[6];

void CreateTrampolineHook(DWORD originalAddress, void* hookFunction, BYTE* pOriginalBytesBuffer, tGenericUnlockFunc* pOriginalFuncPtr) {
    DWORD oldProtect;

    // 复制原始函数的开头字节
    VirtualProtect((LPVOID)originalAddress, sizeof(originalBytes_IsCarPartUnlocked), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(pOriginalBytesBuffer, (void*)originalAddress, sizeof(originalBytes_IsCarPartUnlocked));
    VirtualProtect((LPVOID)originalAddress, sizeof(originalBytes_IsCarPartUnlocked), oldProtect, &oldProtect); // 恢复保护

    // 为原始函数的执行路径创建跳板
    BYTE* trampolineStub = (BYTE*)VirtualAlloc(NULL, sizeof(originalBytes_IsCarPartUnlocked) + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampolineStub) {

        return;
    }

    // 将原始字节复制到跳板中
    memcpy(trampolineStub, pOriginalBytesBuffer, sizeof(originalBytes_IsCarPartUnlocked));

    // 在原始字节后添加一个 JMP 指令，跳回原始函数的其余部分
    trampolineStub[sizeof(originalBytes_IsCarPartUnlocked)] = 0xE9;

    // 计算相对跳转地址：(当前指令的下一条地址即为目标地址)
    *(DWORD*)(trampolineStub + sizeof(originalBytes_IsCarPartUnlocked) + 1) = (originalAddress + sizeof(originalBytes_IsCarPartUnlocked)) - (DWORD)(trampolineStub + sizeof(originalBytes_IsCarPartUnlocked) + 5);

    // 设置原始函数指针指向此跳板
    *pOriginalFuncPtr = (tGenericUnlockFunc)trampolineStub;

    // 在原始函数地址写入 JMP 指令，跳转到挂钩函数
    BYTE jmpCode[5] = { 0xE9 };
    *(DWORD*)(jmpCode + 1) = (DWORD)hookFunction - (originalAddress + 5); // 计算相对地址

    VirtualProtect((LPVOID)originalAddress, sizeof(jmpCode), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy((void*)originalAddress, jmpCode, sizeof(jmpCode));
    VirtualProtect((LPVOID)originalAddress, sizeof(jmpCode), oldProtect, &oldProtect); // 恢复保护
}

// DLL 入口点
extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);

        // 挂钩 IsCarPartUnlocked (外观套件)
        DWORD isCarPartUnlockedAddress = 0x0058A8D0;
        CreateTrampolineHook(isCarPartUnlockedAddress, &MyIsCarPartUnlockedHook, originalBytes_IsCarPartUnlocked, &originalIsCarPartUnlocked);

        // 挂钩 IsPerfPackageUnlocked (性能套件)
        DWORD isPerfPackageUnlockedAddress = 0x0058A960;
        CreateTrampolineHook(isPerfPackageUnlockedAddress, &MyIsPerfPackageUnlockedHook, originalBytes_IsPerfPackageUnlocked, &originalIsPerfPackageUnlocked);

    }
    return TRUE;
}