#pragma once


#include <Windows.h>
#include <iostream> //
#include <fstream> //file io
#include <string>
#include <vector>

#include <nvapi.h>

constexpr auto MAX_COOLER_PER_GPU = 20;

typedef struct
{
	unsigned int Version;
	struct {
		int Level;
		int Policy;
	}NvLevel[MAX_COOLER_PER_GPU];
}NvGPUCoolerLevels_V1;

typedef NvGPUCoolerLevels_V1 NvGPUCoolerLevels;
#define NvGPUCoolerLevels_VER_1   MAKE_NVAPI_VERSION(NvGPUCoolerLevels_V1,1)
#define NvGPUCoolerLevels_VER     NvGPUCoolerLevels_VER_1

typedef int (*NvAPI_QueryInterface_t)(NvU32 offset);
typedef int(*NvAPI_GPU_SetCoolerLevels_t)(NvPhysicalGpuHandle handle, int index, NvGPUCoolerLevels a);
typedef int (*NvAPI_GPU_SetPstates20_t)(NvPhysicalGpuHandle handle, NV_GPU_PERF_PSTATES20_INFO pstates_info);

NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
NvAPI_GPU_SetCoolerLevels_t NvAPI_GPU_SetCoolerLevels = NULL;

HMODULE hmod;
NvAPI_Status nvs;
NvAPI_ShortString nvss;
NvPhysicalGpuHandle phys;
NvU32 cnt;
NvU32 fan;
NV_GPU_THERMAL_SETTINGS thermal;
NvGPUCoolerLevels fan_speed;


VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  const_cast<LPWSTR>(L"gpu_fan") 

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

