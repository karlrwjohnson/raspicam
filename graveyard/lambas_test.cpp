
#include <iostream>
#include <sstream>

#include <unordered_set>
#include <functional>
#include <algorithm> // for_each

using namespace std;

class MyClass
{
public:

	unordered_set< function<void(string)> > myFuncs;

	MyClass() {}

	void
	doIt()
	{
		auto myfunc = [](string str)
		{
			cout << "Anonymous function declared at " << __FUNCTION__ << ", line " << __LINE__
			     << ": str = " << str << endl;
		};
		myfunc("What is this black magic?");

		myFuncs.insert( myfunc);

		for_each(myFuncs.begin(), myFuncs.end(), [](function<void(string)> f) {
			f("Hello World!");
		});
	}
};

int main(int argc, char* args[])
{
	MyClass myObject;
	myObject.doIt();
}
