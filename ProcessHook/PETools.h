#pragma once

PIMAGE_DOS_HEADER getDosHeader(void* pFileBuffer);
PIMAGE_NT_HEADERS getNTHeader(void* pFileBuffer);
PIMAGE_FILE_HEADER getFileHeader(void* pFileBuffer);
PIMAGE_OPTIONAL_HEADER32 getOptionalHeader32(void* pFileBuffer);
PIMAGE_SECTION_HEADER getFirstSectionHeader(void* pFileBuffer);
PIMAGE_SECTION_HEADER getLastSectionHeader(void* pFileBuffer);

void dbgPrintf(const wchar_t* format, ...);

// rvaת��Ϊfa
void* rvaToFa(void* pFileBuffer, size_t rva);

// ��ȡ��Ӧ������Ŀ¼��
void* getDataDirectory(void* pFileBuffer, size_t index);

// ���ļ�
size_t readFile(const wchar_t* pFilePath, void** ppFileBuffer);

// ����
size_t copyFileBufferToImageBuffer(void* pFileBuffer, void** ppImageBuffer);

// �޸��������ض�λ��
bool repairRelocationImage(void* pImageBuffer, size_t newImageBase);

// �޸�������IAT��
bool repairIatImage(void* pImageBuffer);

