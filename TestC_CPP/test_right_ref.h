#pragma once
#include <memory>
#include <iostream>
using namespace std;

#define SAFE_DELETE(p) if(p){delete p;p=nullptr;}

class AC
{
public:
	AC() :
		m_data(new int(1))
	{
		cout << "A::A(),m_data=" << m_data << endl;
	}

	AC(const AC& temp) {//拷贝构造函数，因为有堆内存，定义此函数避免默认构造函数的浅拷贝。
		m_data = new int(*temp.m_data);
		cout << "A::A(const A &temp),m_data=" << m_data << endl;
	}
	AC(AC&& right_value) {
		//移动构造函数，当右值需要拷贝的时候会调用，这样手动移动数据，避免拷贝构造函数的性能问题
		m_data = right_value.m_data;
		right_value.m_data = nullptr;
		cout << "A::A(A &&temp),m_data=" << m_data << endl;
	}

	AC& operator=(AC&& right_value) {
		//移动赋值函数，用于已存在对象赋值时获取资源。，且src对象是右值类型
		if (this != &right_value) {
			SAFE_DELETE(m_data)
				m_data = right_value.m_data;
			right_value.m_data = nullptr;
		}
		cout << "A::A(A &&right_value) operator= ,m_data=" << m_data << endl;
		return *this;
	}

	~AC()
	{
		cout << "A::~A(),m_data=" << m_data << endl;
		SAFE_DELETE(m_data)
	}

private:
	int* m_data;//堆内存， 如果数据很大，拷贝构造函数会导致性能问题
};

AC GetA(bool flag)
{
	AC a;
	AC b;
	cout << "准备返回对象" << endl;
	if (flag)
		return a;
	return b;
}

//返回一个将亡值，也就是一个临时对象，这个临时对象会被立即销毁，所以可以使用右值引用来接收
AC GetAC_XValue() {
	return  GetA(true);
}


void test_right_ref1() {
	//AC temp = GetA(true);
	//这里因为返回值是一个临时对象,是将亡值，所以会调用一次拷贝构造函数， 如果只有默认构造函数，会浅拷贝(拷贝值，也就是指针地址)。
	// 导致两个对象的m_data指向同一个堆内存，析构时会导致两次释放同一个堆内存，导致程序崩溃
	//原则上如果某个类包含堆内存，那么就需要自定义拷贝构造函数，避免浅拷贝。

	{
		//temp2就是用 &&符号构建的右值引用对象。这就是移动语义
		AC&& temp2 = move(GetAC_XValue());//move()将左值转换为右值，这样就可以调用移动构造函数，让这个临时对象在temp2的生命周期内，不会被销毁
		AC temp3;
		temp3= move(temp2);//调用移动赋值函数，将temp2的资源转移到temp3, 为什么是赋值，因为temp3已经存在了，所以是赋值。
	}
	cout << "end" << endl;


}

