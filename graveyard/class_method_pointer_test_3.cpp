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

using namespace std;

typedef void (*genericFnPtr) (string);

class ParentClass
{
  public:

	typedef void (ParentClass::* my_func_ptr_t)(string);

	void doFoo (my_func_ptr_t funcPtr)
	{
		(this->*funcPtr)("WAFFLES ARE AWESOME TOO");
	}

};

class ChildClass: public ParentClass
{
  public:
	void sendFooToDoFoo ()
	{
		doFoo((my_func_ptr_t) &ChildClass::foo);
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

	void* ptr = reinterpret_cast<void*>(&ChildClass::foo);
	printf("ptr  = 0x%p\n", ptr);

	void* ptr2 = reinterpret_cast<void*>(&ChildClass::sendFooToDoFoo);
	printf("ptr2 = 0x%p\n", ptr2);

	printf("Size of MFP (foo) = %lu\n", sizeof(&ChildClass::foo));
	printf("Size of MFP (sendFooToDoFoo) = %lu\n", sizeof(&ChildClass::sendFooToDoFoo));

	uint8_t* fnPtr = reinterpret_cast<uint8_t*>(ptr); //reinterpret_cast<uint8_t*>(&ChildClass::foo);
	printf("&foo            = 0x");
	for(int i = 0; i < sizeof(&ChildClass::foo); i++)
		printf(" %02hhx", fnPtr[i]);
	printf("\n");

	uint8_t* fnPtr2 = reinterpret_cast<uint8_t*>(ptr2); //reinterpret_cast<uint8_t*>(&ChildClass::foo);
	printf("&sendFooToDoFoo = 0x");
	for(int i = 0; i < sizeof(&ChildClass::sendFooToDoFoo); i++)
		printf(" %02hhx", fnPtr2[i]);
	printf("\n");
}

