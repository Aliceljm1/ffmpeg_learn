#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>


//������ȡ���ݣ������ǻ�����ָ����ƶ�
void test_fread()
{
	FILE* file = fopen("example.txt", "rb");
	if (file != NULL) {
		char buffer[150];
		size_t first_read = fread(buffer, sizeof(char), 100, file);
		if (first_read < 100) {
			fclose(file);
			return;
		}
		// ��ȡʣ���50���ֽڣ��ƶ�д�뻺������ָ��
		size_t n2 = fread(buffer + first_read, sizeof(char), 50, file);
		fclose(file);
	}
} 