// nv_gpufan_static_pub.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
#include "nv_gpufan_static_pub.h"


int main(int argc, TCHAR* argv[]) {
	//errlog.open();
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		//errlog_w(GetLastError());
		return GetLastError();
	}

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
	DWORD Status = E_FAIL;

	// Register our service control handler with the SCM
	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (g_StatusHandle == NULL) {
		//errlog_w("");
		return;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
		//errlog_w("");
		OutputDebugString(
			L"ServiceMain: SetServiceStatus returned error");
	}

	/*
	* Perform tasks necessary to start the service here
	*/

	// Create a service stop event to wait on later
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL) {
		// Error creating event
		// Tell service controller we are stopped and exit
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
			//errlog_w("");
			OutputDebugString(
				L"ServiceMain: SetServiceStatus returned error");
		}
		return;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
		//errlog_w("");
		OutputDebugString(
			L"ServiceMain: SetServiceStatus returned error");
	}
	//Logrotate
	/*
	* ...
	*/

	//}
	//File open
	


	// API Load and init
	hmod = LoadLibrary(L"nvapi64.dll");
	if (hmod == NULL) { return; }
	nvs = NvAPI_Initialize();
	if (nvs != NVAPI_OK) { //errlog_w(NvAPI_GetErrorMessage(nvs, nvss)); 
		return; 
	}

	//API prep
	NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");
	NvAPI_GPU_SetCoolerLevels = (NvAPI_GPU_SetCoolerLevels_t)(*NvAPI_QueryInterface)(0x891FA0AE);
	fan_speed.Version = NvGPUCoolerLevels_VER;
	fan_speed.NvLevel[0].Policy = 1;
	thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
	NvAPI_EnumPhysicalGPUs(&phys, &cnt);


	// Start a thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	// Wait until our worker thread exits signaling that the service needs to stop
	if (hThread == NULL) { return; }
	WaitForSingleObject(hThread, INFINITE);


	/*
	* Perform any cleanup tasks
	*/


	CloseHandle(g_ServiceStopEvent);

	// Tell the service controller we are stopped
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		
		OutputDebugString(
			L"ServiceMain: SetServiceStatus returned error");
	}


	// API unload
	nvs = NvAPI_Unload();
	if (nvs != NVAPI_OK) { //errlog_w(NvAPI_GetErrorMessage(nvs, nvss)); 
	}

	if (FreeLibrary(hmod) == NULL) { //errlog_w(GetLastError()); return;
	}
	return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		* Perform tasks necessary to stop the service here
		*/

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			
			OutputDebugString(
				L"ServiceCtrlHandler: SetServiceStatus returned error");
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}
}

static void change_fan_speed(int a) {
	fan_speed.NvLevel[0].Level = a;
	(*NvAPI_GPU_SetCoolerLevels)(phys, 0, fan_speed);
}

static int mk_log_mk_change() {
	NvAPI_GPU_GetThermalSettings(phys, 0, &thermal);
	NvAPI_GPU_GetTachReading(phys, &fan);
	auto current_temp = thermal.sensor[0].currentTemp;

	if (current_temp <= 55) { change_fan_speed(0); return 0; }
	if (current_temp <= 60 && fan <= 30) { change_fan_speed(30);  return 0; }
	if (current_temp <= 70 && fan != 60) { change_fan_speed(60); return 1; }
	if (current_temp >= 71 && fan != 100) { change_fan_speed(100);  return 2; }
	//else just log
	//wlog(now(), current_temp, fan);
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		/*
		* Perform main service function here
		*/

		switch (mk_log_mk_change()) {
		case 0:
			Sleep(30000);
		case 1:
			Sleep(15000);
		case 2:
			Sleep(1000);
		}
	}

	return ERROR_SUCCESS;
}
