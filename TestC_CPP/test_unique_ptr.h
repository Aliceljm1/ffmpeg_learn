#pragma once
#include <memory>
#include <iostream>
using namespace std;

void test_unique_ptr() {

	try {
		unique_ptr<int> p1(new int(10));
		//auto pERROR = p1;//错误，独占型指针不能拷贝
		auto p2 = move(p1);//正确，使用move转移所有权
		cout << *p2 << endl;
		//cout << *p1 << endl;//错误，p1已经转移所有权, p1是nullptr
	}
	catch (const std::exception& e) {
		cout << e.what() << endl;
	}
}