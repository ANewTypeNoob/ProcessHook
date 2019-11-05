#include <stdio.h>
#include <Windows.h>
#include "InjectModule.h"
#include "PETools.h"
#include "IATHook.h"
#include "InlineHook.h"

/************************************
	��ע�����ĵ�ַ(����Debugģʽ�±���ͨ���ĳ��������ȷHook)
************************************/
wchar_t pFilePath[] = L"C:/Users/12269/source/repos/Test/Debug/Test.exe";

// Զ��ע�������ڴ�
DWORD entry(void* pImageBuffer) {
	// �޸�IAT��
	repairIatImage(pImageBuffer);
	// ��ʼ��IAT Hook
	initIATHook();
	// ��ʼ��Inline Hook
	initInlineHook();

	//����FileMapping����					
	HANDLE hMapObject = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 10, TEXT("shared"));
	//��FileMapping����ӳ�䵽�Լ��Ľ���					
	HANDLE hMapView = MapViewOfFile(hMapObject, FILE_MAP_WRITE, 0, 0, 0);

	while (true) {
		switch (*(char*)hMapView) {
		case HOOK_MESSAGE_BOX:
			setIATHook();
			break;

		case UNHOOK_MESSAGE_BOX:
			unSetIATHook();
			break;

		case HOOK_SUM:
			setInlineHook();
			break;

		case UNHOOK_SUM:
			unsetInlineHook();
			break;

		case CALL_MESSAGE_BOX:
			callMessageBox();
			break;

		case CALL_SUM:
			callSum();
			break;
		}

		*(char*)hMapView = 0;
		Sleep(100);
	}

	return 0;
}

void injectModule() {
	// ��ȡImageBase
	HMODULE imageBase = GetModuleHandle(NULL);
	// ��ȡSizeOfImage
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = getOptionalHeader32((void*)imageBase);
	size_t sizeOfImage = pOptionalHeader32->SizeOfImage;

	// �����������ڴ�
	void* pImageBuffer = malloc(sizeOfImage);
	memcpy(pImageBuffer, (void*)imageBase, sizeOfImage);

	// ����Ҫע��Ľ���
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);
	CreateProcess(pFilePath, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

	// ����Զ�̽����ڴ�
	size_t newImageBase = (size_t)VirtualAllocEx(pi.hProcess, NULL, sizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// �޸��ض�λ��
	repairRelocationImage(pImageBuffer, newImageBase);

	// д��Զ�̽���
	WriteProcessMemory(pi.hProcess, (void*)newImageBase, pImageBuffer, sizeOfImage, NULL);

	// ����Զ���̣߳����Ϊentry������ΪnewImageBase
	CreateRemoteThread(pi.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)((size_t)entry - (size_t)imageBase + newImageBase), (void*)newImageBase, 0, NULL);

	free(pImageBuffer);
}