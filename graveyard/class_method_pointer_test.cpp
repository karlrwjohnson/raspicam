// Q: Can I pass a pointer to an object's method?
// A: Yes, but the syntax is weird.

#include <iostream>
#include <string>

using namespace std;


class MyClass
{
  public:

	/* The first trick is getting the syntax of the type specification right.
	   It's easiest to use a typedef so you only have to declare the type once.
	   It looks like an ordinary function pointer typedef, except with the
	   added "MyClass::*" prefix on the function name.
	*/
	typedef void (MyClass::* my_func_ptr_t)(string);

	void foo (string bar)
	{
		cout << "bar = " << bar << endl;
	}

	void doFoo (my_func_ptr_t fooPtr)
	{
		/* The second trick is properly dereferencing the pointer. "this->"
		   dereferences the current object, "*fooPtr" dereferences the function,
		   and the two together are wrapped in parens so the entire unit is the
		   function being called.
		*/
		(this->*fooPtr)("PANCAKES ARE AWESOME");
	}

	void sendFooToDoFoo ()
	{
		/* The third trick is taking a reference to the function properly.
		   It's necessary to refer to the base class.
		*/
		doFoo(&MyClass::foo);
	}
};

int main (int argc, char *args[])
{
	MyClass myObject;
	myObject.sendFooToDoFoo();
}

