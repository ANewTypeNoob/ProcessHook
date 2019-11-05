#include <stdio.h>
#include <Windows.h>
#include "PETools.h"

PIMAGE_DOS_HEADER getDosHeader(void* pFileBuffer) {
	return (PIMAGE_DOS_HEADER)pFileBuffer;
}
PIMAGE_NT_HEADERS getNTHeader(void* pFileBuffer) {
	return (PIMAGE_NT_HEADERS)((size_t)pFileBuffer + getDosHeader(pFileBuffer)->e_lfanew);
}
PIMAGE_FILE_HEADER getFileHeader(void* pFileBuffer) {
	return (PIMAGE_FILE_HEADER)((size_t)getNTHeader(pFileBuffer) + 4);
}
PIMAGE_OPTIONAL_HEADER32 getOptionalHeader32(void* pFileBuffer) {
	return (PIMAGE_OPTIONAL_HEADER32)((size_t)getFileHeader(pFileBuffer) + IMAGE_SIZEOF_FILE_HEADER);
}
PIMAGE_SECTION_HEADER getFirstSectionHeader(void* pFileBuffer) {
	return (PIMAGE_SECTION_HEADER)((size_t)getOptionalHeader32(pFileBuffer) + getFileHeader(pFileBuffer)->SizeOfOptionalHeader);
}
PIMAGE_SECTION_HEADER getLastSectionHeader(void* pFileBuffer) {
	return getFirstSectionHeader(pFileBuffer) + getFileHeader(pFileBuffer)->NumberOfSections - 1;
}

void dbgPrintf(const wchar_t* format, ...) {
	wchar_t strBuffer[100];
	va_list vlArgs;
	va_start(vlArgs, format);
	vswprintf_s(strBuffer, 100, format, vlArgs);
	va_end(vlArgs);

	OutputDebugString(strBuffer);
}

void* rvaToFa(void* pFileBuffer, size_t rva) {
	if (!pFileBuffer) {
		printf("pFileBufferΪNULL\n");
		return NULL;
	}

	PIMAGE_FILE_HEADER pFileHeader = getFileHeader(pFileBuffer);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = getOptionalHeader32(pFileBuffer);
	PIMAGE_SECTION_HEADER pFirstSectionHeader = getFirstSectionHeader(pFileBuffer);

	// rva��PEͷ�ڲ�
	if (rva < pOptionalHeader32->SizeOfHeaders) {
		return (void*)((size_t)pFileBuffer + rva);
	}

	// rva��PE�������ڲ�
	PIMAGE_SECTION_HEADER pNextSectionHeader = pFirstSectionHeader;
	for (size_t i = 0; i < pFileHeader->NumberOfSections; i++) {
		if (rva >= pNextSectionHeader->VirtualAddress && rva < pNextSectionHeader->VirtualAddress + pNextSectionHeader->SizeOfRawData) {
			return (void*)((size_t)pFileBuffer + rva - pNextSectionHeader->VirtualAddress + pNextSectionHeader->PointerToRawData);
		}
		pNextSectionHeader++;
	}

	printf("rvaת��ʧ��\n");
	return NULL;
}

void* getDataDirectory(void* pFileBuffer, size_t index) {
	size_t dataDirectoryRva = getOptionalHeader32(pFileBuffer)->DataDirectory[index].VirtualAddress;
	return dataDirectoryRva ? rvaToFa(pFileBuffer, dataDirectoryRva) : NULL;
}

size_t readFile(const wchar_t* pFilePath, void** ppFileBuffer) {
	// ���У��
	if (!pFilePath) {
		wprintf(L"pFilePathΪNULL\n");
		return 0;
	}

	// ���ļ�
	FILE* pFile = NULL;
	_wfopen_s(&pFile, pFilePath, L"rb");
	if (!pFile) {
		wprintf(L"���ļ�ʧ��\n");
		return 0;
	}

	// �����ļ���С
	fseek(pFile, 0, SEEK_END);
	size_t fileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	// �����ڴ�
	void* pFileBuffer = malloc(fileSize);
	if (!pFileBuffer) {
		fclose(pFile);
		wprintf(L"�����ڴ�ʧ��\n");
		return 0;
	}
	memset(pFileBuffer, 0, fileSize);

	// ��ȡ�ļ�
	if (!fread(pFileBuffer, fileSize, 1, pFile)) {
		fclose(pFile);
		free(pFileBuffer);
		wprintf(L"��ȡ�ļ�ʧ��\n");
		return 0;
	}

	// �洢pFileBuffer
	*ppFileBuffer = pFileBuffer;

	// �ر��ļ�
	fclose(pFile);

	return fileSize;
}

size_t copyFileBufferToImageBuffer(void* pFileBuffer, void** ppImageBuffer) {
	if (!pFileBuffer) {
		printf("pFileBufferΪNULL\n");
		return 0;
	}

	PIMAGE_FILE_HEADER pFileHeader = getFileHeader(pFileBuffer);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = getOptionalHeader32(pFileBuffer);
	PIMAGE_SECTION_HEADER pFirstSectionHeader = getFirstSectionHeader(pFileBuffer);

	// �����ڴ�
	void* pImageBuffer = malloc(pOptionalHeader32->SizeOfImage);
	if (!pImageBuffer) {
		printf("�����ڴ�ʧ��\n");
		return 0;
	}
	memset(pImageBuffer, 0, pOptionalHeader32->SizeOfImage);

	// ����ͷ��
	memcpy(pImageBuffer, pFileBuffer, pOptionalHeader32->SizeOfHeaders);

	// ���Ƹ�����
	PIMAGE_SECTION_HEADER pNextSectionHeader = pFirstSectionHeader;
	for (size_t i = 0; i < pFileHeader->NumberOfSections; i++) {
		void* dst = (void*)((size_t)pImageBuffer + pNextSectionHeader->VirtualAddress);
		void* src = (void*)((size_t)pFileBuffer + pNextSectionHeader->PointerToRawData);
		size_t size = pNextSectionHeader->SizeOfRawData;
		memcpy(dst, src, size);
		pNextSectionHeader++;
	}

	*ppImageBuffer = pImageBuffer;

	return pOptionalHeader32->SizeOfImage;
}

bool repairRelocationImage(void* pImageBuffer, size_t newImageBase) {
	if (!pImageBuffer) {
		printf("pImageBufferΪNULL\n");
		return false;
	}

	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = getOptionalHeader32(pImageBuffer);
	size_t dataDirectoryRva = getOptionalHeader32(pImageBuffer)->DataDirectory[5].VirtualAddress;
	PIMAGE_BASE_RELOCATION pBaseRelocation = (PIMAGE_BASE_RELOCATION)((size_t)pImageBuffer + dataDirectoryRva);

	if (!pBaseRelocation) {
		printf("û���ض�λ��\n");
		return false;
	}

	while (pBaseRelocation->VirtualAddress) {
		unsigned short* t = (unsigned short*)((size_t)pBaseRelocation + 8);
		for (size_t i = 0; i < (pBaseRelocation->SizeOfBlock - 8) / 2; i++) {
			size_t rva = (*t) & 0xFFF;
			if (rva && (((*t) >> 12) == 3)) {
				size_t* fa = (size_t*)((size_t)pImageBuffer + rva + pBaseRelocation->VirtualAddress);
				*fa = *fa - pOptionalHeader32->ImageBase + newImageBase;
			}
			t++;
		}
		pBaseRelocation = (PIMAGE_BASE_RELOCATION)((size_t)pBaseRelocation + pBaseRelocation->SizeOfBlock);
	}

	pOptionalHeader32->ImageBase = newImageBase;

	return true;
}

bool repairIatImage(void* pImageBuffer) {
	if (!pImageBuffer) {
		printf("pImageBufferΪNULL\n");
		return false;
	}

	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = getOptionalHeader32(pImageBuffer);
	size_t dataDirectoryRva = pOptionalHeader32->DataDirectory[1].VirtualAddress;
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((size_t)pImageBuffer + dataDirectoryRva);
	if (!pImportDescriptor) {
		printf("û�е�����\n");
		return false;
	}

	while (pImportDescriptor->OriginalFirstThunk) {
		// ����ģ��
		HMODULE hModule = LoadLibraryA((char*)((size_t)pImageBuffer + pImportDescriptor->Name));

		size_t* originalThunk = (size_t*)((size_t)pImageBuffer + pImportDescriptor->OriginalFirstThunk);
		size_t* thunk = (size_t*)((size_t)pImageBuffer + pImportDescriptor->FirstThunk);
		while (*originalThunk) {
			if ((*originalThunk) & 0x80000000) {
				*thunk = (size_t)GetProcAddress(hModule, (LPCSTR)((*originalThunk) & 0x7FFFFFFF));
			}
			else {
				*thunk = (size_t)GetProcAddress(hModule, ((PIMAGE_IMPORT_BY_NAME)((size_t)pImageBuffer + *originalThunk))->Name);
			}
			originalThunk++;
			thunk++;
		}

		pImportDescriptor++;
	}

	return true;
}