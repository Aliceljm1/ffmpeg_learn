#pragma once
#include <memory>
#include <iostream>
using namespace std;

//�̳�enable_shared_from_this�����Է���thisָ���shared_ptr
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

	shared_ptr<AC> getSelf2() {//��д�Ǵ���ģ��ᵼ�¼������󣬲��������ԭ����shared_ptr
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
	p2.reset(new AC(2));//p2ָ���µĶ���ԭ���Ķ����ͷ�

	cout << "p2.use_count()=" << p2.use_count() << endl;
	shared_ptr<AC> p3(new AC(3), [p2](AC* a)//ɾ������ �������������������p2
		{
			cout << "delete " << a->getId() << ",p2.id=" << p2->getId() << endl;
			delete a;
		});
	auto p4 = shared_ptr<int>(new int[10], [](int* a)//���춯̬�����shared_ptr����Ҫ�Զ���ɾ����
		{
			cout << "delete " << *a << endl;
			delete [] a;
		});
	//��p4���鸳ֵ����1��10
	for (int i = 0; i < 10; i++)
	{
		p4.get()[i] = i + 1;
	}

	cout<<p4.get()[5]<<endl;

	auto p5 = p2->getSelf();//����thisָ���shared_ptr���м�����1


}