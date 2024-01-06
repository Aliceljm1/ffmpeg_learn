#pragma once


#include <stdio.h>
#include <string.h>

int test_memmove_memcpy() {
    char str[] = "Hello_World";
    //�����ַ���str��str1

    char str_copy[20];
    strcpy_s(str_copy, str);

    printf("Before memmove: %s\n", str);


    // ʹ��memmove�������ַ����е�ǰ5���ַ��ƶ�����6���ַ�����
    memmove(str + 6, str, 5);//str=Hello_Hello
    printf("After memmove: %s\n", str);

    //����str_copy��str
    memcpy(str, str_copy, strlen(str_copy));
    int headszie = 2; //��ͷ����2���ַ��ƶ���β��2���ֽڴ�
    memmove(str + strlen(str) - headszie, str, headszie);

    printf("After memmove: %s\n", str);

    return 0;
}
