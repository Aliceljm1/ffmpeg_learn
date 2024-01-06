#pragma once


#include <stdio.h>
#include <string.h>

int test_memmove_memcpy() {
    char str[] = "Hello_World";
    //复制字符串str到str1

    char str_copy[20];
    strcpy_s(str_copy, str);

    printf("Before memmove: %s\n", str);


    // 使用memmove函数将字符串中的前5个字符移动到第6个字符后面
    memmove(str + 6, str, 5);//str=Hello_Hello
    printf("After memmove: %s\n", str);

    //拷贝str_copy到str
    memcpy(str, str_copy, strlen(str_copy));
    int headszie = 2; //将头部的2个字符移动到尾部2个字节处
    memmove(str + strlen(str) - headszie, str, headszie);

    printf("After memmove: %s\n", str);

    return 0;
}
