#pragma once
#include <memory>
#include <iostream>
using namespace std;

//继承enable_shared_from_this，可以返回this指针的shared_ptr
class AC :public enable_shared_from_this<AC>
{
public:

	AC(int id) :
		m_id(id)
	{

	}
	virtual ~AC() {
		cout << "A::~A(),id=" << m_id << endl;
	}

	shared_ptr<AC> getSelf() {
		return shared_from_this();
	}

	shared_ptr<AC> getSelf2() {//样写是错误的，会导致计数错误，不会关联到原来的shared_ptr
		return shared_ptr<AC>(this);
	}

	int getId() {
		return m_id;
	}
private:
	int m_id;
};
void test_shared_ptr()
{

	shared_ptr<int> p1;
	p1.reset(new int(12));

	auto p2 = make_shared<AC>(1);
	p2.reset(new AC(2));//p2指向新的对象，原来的对象被释放

	cout << "p2.use_count()=" << p2.use_count() << endl;
	shared_ptr<AC> p3(new AC(3), [p2](AC* a)//删除器： 匿名函数新增捕获参数p2
		{
			cout << "delete " << a->getId() << ",p2.id=" << p2->getId() << endl;
			delete a;
		});
	auto p4 = shared_ptr<int>(new int[10], [](int* a)//构造动态数组的shared_ptr，需要自定义删除器
		{
			cout << "delete " << *a << endl;
			delete [] a;
		});
	//给p4数组赋值，从1到10
	for (int i = 0; i < 10; i++)
	{
		p4.get()[i] = i + 1;
	}

	cout<<p4.get()[5]<<endl;

	auto p5 = p2->getSelf();//返回this指针的shared_ptr，有计数加1


}