// Q: What does it look like, to have a set of function pointers?
// A: It works. Really well.

#include <iostream>
#include <set>
#include <string>

using namespace std;

typedef void (*fn_ptr)(string);

void fn1 (string param)
{
	cout << __FUNCTION__ << ": param = " << param << endl;
}

void fn2 (string param)
{
	cout << __FUNCTION__ << ": param = " << param << endl;
}

void fn3 (string param)
{
	cout << __FUNCTION__ << ": param = " << param << endl;
}



int main (int argc, char *args[])
{
	set<fn_ptr> fns;

	cout << "Calling all functions in fns..." << endl;
	for (set<fn_ptr>::iterator it = fns.begin(); it != fns.end(); it++) {
		(*it)("\"Test 0\"");
	}
	cout << endl;

	fns.insert(fn2);
	fns.insert(fn1);

	cout << "Calling all functions in fns..." << endl;
	for (set<fn_ptr>::iterator it = fns.begin(); it != fns.end(); it++) {
		(*it)("\"Test 1\"");
	}
	cout << endl;

	fns.insert(fn3);

	cout << "Calling all functions in fns..." << endl;
	for (set<fn_ptr>::iterator it = fns.begin(); it != fns.end(); it++) {
		(*it)("\"Test 2\"");
	}
	cout << endl;

	fns.insert(fn1);

	cout << "Calling all functions in fns..." << endl;
	for (set<fn_ptr>::iterator it = fns.begin(); it != fns.end(); it++) {
		(*it)("\"Test 3\"");
	}
	cout << endl;

	fns.erase(fn2);

	cout << "Calling all functions in fns..." << endl;
	for (set<fn_ptr>::iterator it = fns.begin(); it != fns.end(); it++) {
		(*it)("\"Test 4\"");
	}
	cout << endl;

	fns.erase(fn2);

	cout << "Calling all functions in fns..." << endl;
	for (set<fn_ptr>::iterator it = fns.begin(); it != fns.end(); it++) {
		(*it)("\"Test \"");
	}
	cout << endl;
}
