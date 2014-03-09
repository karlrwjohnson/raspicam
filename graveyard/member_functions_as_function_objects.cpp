

#include <string>
#include <iostream>
#include <functional>

using namespace std;


class MyClass
{
	string foo;

  public:
	MyClass(const MyClass &orig)
	{
		cout << "WARNING: I'VE BEEN COPIED!" << endl;
	}

	MyClass (string fooval):
		foo(fooval)
	{ }

	void setFoo(string fooval)
	{
		cout << "setFoo: foo -> " << fooval << endl;
		foo = fooval;
	}

	string getFoo()
	{
		return foo;
	}

	function<void(string)> getSetFoo()
	{
		return bind(&MyClass::setFoo, this, placeholders::_1);
	}

};

void callFunc (function<void(string)> func, string val)
{
	func(val);
}

int main(int argc, char *args[])
{
	MyClass myobj("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	// Somehow pass member function myobj.getFoo to callFunc to indirectly
	// assign "wordbank" to foo.

	// This is the direct route, using lambdas. It's equivalent to what I'm
	// currently doing to pass message handlers for the server:
	callFunc(
		[&](string s){
			myobj.setFoo(s);
		},
		"wordbank"
	);

	cout << "after:  foo = " << myobj.getFoo() << endl;

	// (Reset)
	myobj.setFoo("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	// Is there a better way that lets me somehow wrap std::function directly
	// around a member function?

	// I can get a pointer to a member function this way, but it doesn't
	// have an obvious way of passing in parameters.
	function<string(MyClass&)> getFooWrapped = &MyClass::getFoo;
	cout << "   aside: foo (from wrapped fn) = " << getFooWrapped(myobj) << endl;

	// What's the type?
	// Let's see, setFoo is a void(string). The first layer is obviously to
	// bind to the current class, so that would be the innermost parameter.
	// Maybe it's a void(string(MyClass&))?
	//function<void(string(MyClass&))> setFooWrapped = &MyClass::setFoo;

	// Nope.
	// error: conversion from
	//   ‘void (MyClass::*)(std::string) {aka void (MyClass::*)(std::basic_string<char>)}’
	// to non-scalar type
	//   ‘std::function<void(std::basic_string<char> (*)(MyClass&))>’
	// requested
	//  function<void(string(MyClass&))> setFooWrapped = &MyClass::setFoo;
	//                                                             ^

	// This doesn't work either:
	//function<void(string)(MyClass&)> setFooWrapped = &MyClass::setFoo;

	// Maybe this?
	function<void(MyClass&, string)> setFooWrapped = &MyClass::setFoo;

	// Okay, that works! Kinda adds an extra parameter to the front, though.
	// I'd much prefer the chain-looking prototype, though. Then I could bind
	// it with the first call and then pass it elsewhere for the follow-up.

	// Let's test it anyway:
	setFooWrapped(myobj, "wordbank");

	cout << "after:  foo = " << myobj.getFoo() << endl;

	// (Reset)
	myobj.setFoo("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	// Hey, what's this about binding?
	function<void(string)> setFooWrappedAndBound = bind(setFooWrapped, myobj, placeholders::_1);
cout << "---" << endl;
	setFooWrappedAndBound("wordbank");

	// Well, *that* doesn't work. It makes copies of the original object!
	cout << "after:  foo = " << myobj.getFoo() << endl;

	// (Reset)
	myobj.setFoo("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	auto setFoo_2 = bind(&MyClass::setFoo, myobj, placeholders::_1);
	setFoo_2("wordbank");

	// Nope, still copied.
	cout << "after:  foo = " << myobj.getFoo() << endl;


	// (Reset)
	myobj.setFoo("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	// Okay, so I need to add the &reference operator, and THEN it works.
	auto setFoo_3 = bind(&MyClass::setFoo, &myobj, placeholders::_1);
	setFoo_3("wordbank");

	cout << "after:  foo = " << myobj.getFoo() << endl;

	// One more reset -- this time, to see if I can call callFunc() with this thing!
	myobj.setFoo("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	callFunc(
		bind(&MyClass::setFoo, &myobj, placeholders::_1),
		"wordbank"
	);

	cout << "after:  foo = " << myobj.getFoo() << endl;

	// I lied. Let's let the object bind its own methods!
	myobj.setFoo("walrus");
	cout << "before: foo = " << myobj.getFoo() << endl;

	callFunc(myobj.getSetFoo(), "wordbank");

	cout << "after:  foo = " << myobj.getFoo() << endl;

	// Yep, it works!
}
