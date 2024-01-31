#pragma once
#include <memory>
#include <iostream>
#include <cmath>
using namespace std;


//�������ڹ���shared_ptr ӵ����ָ��Ķ��󲢹������������ڣ��� weak_ptr ��ӵ�ж��󡣲����ƶ����������ڵ�
//���ü�����shared_ptr �Ĵ��ڻ����Ӷ�������ü������� weak_ptr ���ᣬ��������������Ӱ�졣
//��Դ���ʣ�shared_ptr ����ֱ�ӷ��������Ķ��󣬶� weak_ptr ��Ҫ�ȵ���lock()ת��Ϊ shared_ptr ���ܰ�ȫ���ʶ���
//�����ϣ�weak_ptr �� shared_ptr ��һ���۲��ߣ����Թ۲� shared_ptr ���������ڣ����ǲ���Ӱ�����������ڡ���Ϊ�˽��ѭ�����õ����ڴ�й¶����
class Test {
public:
	Test() { cout << "Test Constructor\n"; }
	~Test() { cout << "Test Destructor\n"; }
	void show() { cout << "Test::show()\n"; }
};

int test_weak_ptr() {
	// ����һ�� shared_ptr
	shared_ptr<Test> sp1 = make_shared<Test>();

	// ����һ�� weak_ptr ���۲� sp1
	weak_ptr<Test> wp1 = sp1;

	// ��� wp1 �Ƿ�ָ��һ������
	cout << "wp1 use_count: " << wp1.use_count() << endl;
	cout << "sp1 use_count: " << sp1.use_count() << endl;

	// �� weak_ptr ����Ϊ shared_ptr �����ʶ��� ����۲�Ķ���û�����٣�use_cout>0������������spָ��
	if (shared_ptr<Test> sp2 = wp1.lock()) {//�̰߳�ȫ�ģ���Ϊlock()�ڲ�������
		sp2->show();
		shared_ptr<Test> sp22 = wp1.lock();//ÿ��lock()�����������ü�������Ϊ�������µ�shared_ptr��ǿ����
		cout << "Inside if block: sp2 use_count: " << sp2.use_count() << endl;
	}


	// ����ԭʼ�� shared_ptr
	sp1.reset();

	// �ٴγ��Դ� weak_ptr ��ȡ shared_ptr�� 
	if (shared_ptr<Test> sp3 = wp1.lock()) {
		sp3->show();
	}
	else {
		cout << "wp1 is expired after sp1 is reset.\n";
	}

	if (wp1.expired()) {//�ж�weak_ptr�Ƿ����, �۲����Դ��������
		cout << "wp1 is expired after sp1 is reset.\n";
	}

	if (int a = max(0, -1)) {//if�ж���ֵ���ʽ���ж�����ֵ�ı���
		cout << "a=" << a << endl;
	}

	return 0;
}

