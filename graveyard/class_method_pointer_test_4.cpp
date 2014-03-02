// The previous test didn't model what I actually was planning on doing.

// Here, I need to pass a pointer to a function defined in the child class to
// the parent, which calls it.

// The only real trick is that you need to cast the pointer to a member function
// of the parent instead of a member function of the child. It's not exactly
// a safe cast, but so long as only the child is passing pointers to its own
// methods we're fine.

#include <iostream>
#include <string>
#include <memory>

#include <stdio.h>

#include <stdint.h> // uint8_t

#include <functional> // lambdas!

using namespace std;

typedef void (*genericFnPtr) (string);

class ParentClass
{
  public:

	//typedef void (ParentClass::* my_func_ptr_t)(string);

	typedef function< void(string) > my_func_t;
	typedef shared_ptr< my_func_t > my_func_ptr_t;

	void doFoo (my_func_t& func)
	{
		func("WHAT ABOUT CREPES GUYS?");
	}

};

class ChildClass: public ParentClass
{
  public:

	string mystr;

	my_func_t lambdaFoo = [this](string bar)
	{
		cout << "{lambdaFoo}: bar = " << bar << endl;
		cout << "{lambdaFoo}: mystr = " << mystr << endl;
	};

	void sendFooToDoFoo ()
	{
		//function<void(&ChildClass)> fooWrapper = 
		//doFoo(fooWrapper);
		mystr = "This is a state variable";
		doFoo(lambdaFoo);
		mystr = "This is another state variable";
		doFoo(lambdaFoo);
	}

	void foo (string bar)
	{
		cout << "bar = " << bar << endl;
	}

};

int main (int argc, char *args[])
{
	ChildClass myObject;
	myObject.sendFooToDoFoo();
}

