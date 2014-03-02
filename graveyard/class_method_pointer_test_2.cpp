// Q: Does it still work with a pointer to a child class?
// A: Yep. Gotta love inheritance!

#include <iostream>
#include <string>

using namespace std;


class ParentClass
{
  public:

	typedef void (ParentClass::* my_func_ptr_t)(string);

	void foo (string bar)
	{
		cout << "bar = " << bar << endl;
	}

};

class ChildClass: public ParentClass
{
  public:
	void doFoo (my_func_ptr_t fooPtr)
	{
		(this->*fooPtr)("PANCAKES ARE AWESOME");
	}

	void sendFooToDoFoo ()
	{
		doFoo(&ChildClass::foo);
	}
};

int main (int argc, char *args[])
{
	ChildClass myObject;
	myObject.sendFooToDoFoo();
}

