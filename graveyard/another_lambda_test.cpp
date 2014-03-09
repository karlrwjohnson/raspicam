// Example of assigning lambdas at instantiation time.

#include <string>
#include <iostream>
#include <algorithm>

using namespace std;

class MyClass {

	string foo;

  public:
	const function<void(string)> setFoo;
	const function<string()> getFoo;

	MyClass(string fooval);

};

MyClass::MyClass(string fooval):
	foo(fooval),
	setFoo([this](string fooval){
		foo = fooval;
	}),
	getFoo([this]{
		return foo;
	})
{ }

int main(int argc, char *args[])
{
	MyClass myObj("walrus");
	cout << "foo = " << myObj.getFoo() << endl;
	myObj.setFoo("wordbank");
	cout << "foo = " << myObj.getFoo() << endl;

	([](){
		cout << "Can't believe this is valid." << endl;
	})();

	return 0;
}

