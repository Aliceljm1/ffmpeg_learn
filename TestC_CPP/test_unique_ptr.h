#pragma once
#include <memory>
#include <iostream>
using namespace std;

void test_unique_ptr() {

	try {
		unique_ptr<int> p1(new int(10));
		//auto pERROR = p1;//���󣬶�ռ��ָ�벻�ܿ���
		auto p2 = move(p1);//��ȷ��ʹ��moveת������Ȩ
		cout << *p2 << endl;
		//cout << *p1 << endl;//����p1�Ѿ�ת������Ȩ, p1��nullptr
	}
	catch (const std::exception& e) {
		cout << e.what() << endl;
	}
}