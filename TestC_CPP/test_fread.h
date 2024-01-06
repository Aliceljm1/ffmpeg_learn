#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>


//连续读取数据，本质是缓冲区指针的移动
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
		// 读取剩余的50个字节，移动写入缓冲区的指针
		size_t n2 = fread(buffer + first_read, sizeof(char), 50, file);
		fclose(file);
	}
} 