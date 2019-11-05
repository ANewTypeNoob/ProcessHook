#include <stdio.h>
#include <Windows.h>
#include <locale>
#include "resource.h"
#include "InjectModule.h"

bool isMessageBoxMonitored = false;
bool isSumMonitored = false;
char* hMapView = NULL;

BOOL CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;

	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_MESSAGE_BOX_MONITOR:
			isMessageBoxMonitored = !isMessageBoxMonitored;
			if (isMessageBoxMonitored) {
				*hMapView = HOOK_MESSAGE_BOX;
				SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_MESSAGE_BOX_MONITOR), L"Close Monitor");
			}
			else {
				*hMapView = UNHOOK_MESSAGE_BOX;
				SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_MESSAGE_BOX_MONITOR), L"Open Monitor");
			}
			return TRUE;

		case IDC_BUTTON_SUM_MONITOR:
			isSumMonitored = !isSumMonitored;
			if (isSumMonitored) {
				*hMapView = HOOK_SUM;
				SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_SUM_MONITOR), L"Close Monitor");
			}
			else {
				*hMapView = UNHOOK_SUM;
				SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_SUM_MONITOR), L"Open Monitor");
			}
			return TRUE;

		case IDC_BUTTON_MESSAGE_BOX_CALL:
			*hMapView = CALL_MESSAGE_BOX;
			return TRUE;

		case IDC_BUTTON_SUM_CALL:
			*hMapView = CALL_SUM;
			return TRUE;
		}
	}
	return FALSE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	// ���ñ����ַ���
	setlocale(LC_ALL, "");

	// ͨ���ڴ�д�뷽ʽע��ģ�飬ʵ��ģ������
	injectModule();

	// ����FileMapping����
	HANDLE hMapObject = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 10, TEXT("shared"));
	// ��FileMapping����ӳ�䵽�Լ��Ľ���
	hMapView = (char*)MapViewOfFile(hMapObject, FILE_MAP_WRITE, 0, 0, 0);

	DialogBox(hInstance, (LPCWSTR)IDD_DIALOG_MAIN, NULL, DialogProc);
	return 0;
}