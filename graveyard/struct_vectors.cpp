#include <vector>
#include <iostream>

using namespace std;

struct foo_s
{
	int a;
	int b;
	int c;
};

int main(int argc, char *args[])
{
	vector<struct foo_s> myvec;
	myvec.push_back({ 3, 4, 55 });
	myvec.push_back({ 20,10,17 });
	cout << "myvec[0].a = " << myvec[0].a << endl
	     << "myvec[0].b = " << myvec[0].b << endl
	     << "myvec[0].c = " << myvec[0].c << endl
	     << "myvec[1].a = " << myvec[1].a << endl
	     << "myvec[1].b = " << myvec[1].b << endl
	     << "myvec[1].c = " << myvec[1].c << endl
	;
}
