#include <stdio.h>
#include <Windows.h>
#include <locale>
#include "PETools.h"
#include "InlineHook.h"

#define INSTRUCTION_NUM 9

typedef int (*pSum)(int, int);

struct Register {
	size_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
};

/************************************
	sum�����ڱ�ע������е�rva(���±��뱻ע��������Ҫ�޸Ĵ�ֵ)
************************************/
size_t sumRva = 0x12A30;
void* sum = NULL;

char oldInstruction[INSTRUCTION_NUM] = { 0 };

size_t arg1 = 0;
size_t arg2 = 0;
Register reg = { 0 };
size_t retAddr = 0;

_declspec(naked) void hookPlus() {
	__asm {
		// �����ֳ�
		pushad
		pushfd
		// ����Ĵ���
		mov reg.eax, eax
		mov reg.ecx, ecx
		mov reg.edx, edx
		mov reg.ebx, ebx
		// �������
		mov eax, DWORD PTR SS : [esp + 0x28]
		mov arg1, eax
		mov eax, DWORD PTR SS : [esp + 0x2C]
		mov arg2, eax
	}

	printf("***************************Inline Hook***************************\n");
	printf("args: %d %d\n", arg1, arg2);
	printf("regs: %d %d %d %d\n", reg.eax, reg.ecx, reg.edx, reg.ebx);
	printf("***************************Inline Hook***************************\n");

	__asm {
		// �ָ��ֳ�
		popfd
		popad
		// ִ�б����ǵ�ָ��
		push ebp
		mov ebp, esp
		sub esp, 0C0h
		// ���ص���hook�ĺ���
		jmp retAddr
	}
}

void initInlineHook() {
	sum = (void*)(sumRva + (size_t)GetModuleHandle(NULL));
	retAddr = (size_t)sum + INSTRUCTION_NUM;

	// �޸�hook��ַ���ڴ��д����
	DWORD oldProtext = 0;
	VirtualProtect(sum, INSTRUCTION_NUM, PAGE_EXECUTE_READWRITE, &oldProtext);

	// �����ָ��
	memcpy(oldInstruction, sum, INSTRUCTION_NUM);
}

void setInlineHook() {
	// ����jmp
	memset(sum, 0x90, INSTRUCTION_NUM);
	*((char*)sum) = 0xE9;
	*((size_t*)((size_t)sum + 1)) = (size_t)hookPlus - (size_t)sum - 5;
}

void unsetInlineHook() {
	memcpy(sum, oldInstruction, INSTRUCTION_NUM);
}

void callSum() {
	((pSum)sum)(1, 2);
}