#pragma once
#include <memory>
#include <iostream>
using namespace std;

#define SAFE_DELETE(p) if(p){delete p;p=nullptr;}

/***
* �� C++ �У���ֵ��lvalue��������ֵ��prvalue���ͽ���ֵ��xvalue���Ǳ��ʽ�����ֻ���ֵ���
*��ֵ������ȡ��ַ�ı��ʽ���־ô��ڡ� int x=10; x������ֵ����Ϊ����ȡ��ַ����x�����������ǳ־õġ�exp:++x, x=1,
����ֵ���������򲻶�Ӧ�ڴ洢λ�õ���ʱ���ʽ��exp:56, x++(x����ֵ��x++�Ǵ���ֵ����Ϊx++����ʱ���ʽ������Ӧ�洢λ��)
����ֵ�����������ٵĶ���������Դ���Ա��ƶ���
*
*/

class AC
{
public:
	AC() :
		m_data(new int(1))
	{
		cout << "A::A(),m_data=" << m_data << endl;
	}

	AC(const AC& temp) {//�������캯������Ϊ�ж��ڴ棬����˺�������Ĭ�Ϲ��캯����ǳ������
		m_data = new int(*temp.m_data);
		cout << "A::A(const A &temp),m_data=" << m_data << endl;
	}
	AC(AC&& right_value) {
		//�ƶ����캯��������ֵ��Ҫ������ʱ�����ã������ֶ��ƶ����ݣ����⿽�����캯������������
		m_data = right_value.m_data;
		right_value.m_data = nullptr;
		cout << "A::A(A &&temp),m_data=" << m_data << endl;
	}

	AC& operator=(AC&& right_value) {
		//�ƶ���ֵ�����������Ѵ��ڶ���ֵʱ��ȡ��Դ������src��������ֵ����
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
	int* m_data;//���ڴ棬 ������ݺܴ󣬿������캯���ᵼ����������
};

AC GetA(bool flag)
{
	AC a;
	AC b;
	cout << "׼�����ض���" << endl;
	if (flag)
		return a;
	return b;
}

//����һ������ֵ��Ҳ����һ����ʱ���������ʱ����ᱻ�������٣����Կ���ʹ����ֵ����������
AC GetAC_XValue() {
	return  GetA(true);
}


void test_right_ref1() {
	//AC temp = GetA(true);
	//������Ϊ����ֵ��һ����ʱ����,�ǽ���ֵ�����Ի����һ�ο������캯���� ���ֻ��Ĭ�Ϲ��캯������ǳ����(����ֵ��Ҳ����ָ���ַ)��
	// �������������m_dataָ��ͬһ�����ڴ棬����ʱ�ᵼ�������ͷ�ͬһ�����ڴ棬���³������
	//ԭ�������ĳ����������ڴ棬��ô����Ҫ�Զ��忽�����캯��������ǳ������

	{
		//temp2������ &&���Ź�������ֵ���ö���������ƶ�����
		AC&& temp2 = move(GetAC_XValue());//move()����ֵת��Ϊ��ֵ�������Ϳ��Ե����ƶ����캯�����������ʱ������temp2�����������ڣ����ᱻ����
		AC temp3;
		temp3= move(temp2);//�����ƶ���ֵ��������temp2����Դת�Ƶ�temp3, Ϊʲô�Ǹ�ֵ����Ϊtemp3�Ѿ������ˣ������Ǹ�ֵ��
	}
	cout << "end" << endl;


}

