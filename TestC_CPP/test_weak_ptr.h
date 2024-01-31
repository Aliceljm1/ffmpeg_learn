#pragma once
#include <memory>
#include <iostream>
#include <cmath>
using namespace std;


//生命周期管理：shared_ptr 拥有其指向的对象并管理其生命周期，而 weak_ptr 不拥有对象。不控制对象生命周期的
//引用计数：shared_ptr 的存在会增加对象的引用计数，而 weak_ptr 不会，创建和析构都不影响。
//资源访问：shared_ptr 可以直接访问其管理的对象，而 weak_ptr 需要先调用lock()转换为 shared_ptr 才能安全访问对象。
//本质上，weak_ptr 是 shared_ptr 的一个观察者，可以观察 shared_ptr 的生命周期，但是不会影响其生命周期。是为了解决循环引用导致内存泄露问题
class Test {
public:
	Test() { cout << "Test Constructor\n"; }
	~Test() { cout << "Test Destructor\n"; }
	void show() { cout << "Test::show()\n"; }
};

int test_weak_ptr() {
	// 创建一个 shared_ptr
	shared_ptr<Test> sp1 = make_shared<Test>();

	// 创建一个 weak_ptr 来观察 sp1
	weak_ptr<Test> wp1 = sp1;

	// 检查 wp1 是否指向一个对象
	cout << "wp1 use_count: " << wp1.use_count() << endl;
	cout << "sp1 use_count: " << sp1.use_count() << endl;

	// 将 weak_ptr 升级为 shared_ptr 并访问对象， 如果观察的对象没有销毁，use_cout>0可以正常返回sp指针
	if (shared_ptr<Test> sp2 = wp1.lock()) {//线程安全的，因为lock()内部加锁了
		sp2->show();
		shared_ptr<Test> sp22 = wp1.lock();//每次lock()都会增加引用计数，因为产生了新的shared_ptr，强引用
		cout << "Inside if block: sp2 use_count: " << sp2.use_count() << endl;
	}


	// 销毁原始的 shared_ptr
	sp1.reset();

	// 再次尝试从 weak_ptr 获取 shared_ptr， 
	if (shared_ptr<Test> sp3 = wp1.lock()) {
		sp3->show();
	}
	else {
		cout << "wp1 is expired after sp1 is reset.\n";
	}

	if (wp1.expired()) {//判断weak_ptr是否过期, 观察的资源被销毁了
		cout << "wp1 is expired after sp1 is reset.\n";
	}

	if (int a = max(0, -1)) {//if判定赋值表达式，判定被赋值的变量
		cout << "a=" << a << endl;
	}

	return 0;
}

