// Demonstrates what happens when istringstream can't parse an integer.

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

using namespace std;

int main (int argc, char * args[])
{
	if(argc < 2) {
		cout << "Usage: " << args[0] << " $number" << endl;
		return 1;
	}

	try {
		int myint;
		istringstream iss(args[1]);
		iss >> myint;
		cout << "Successfully parsed value " << myint << endl;
		return 0;
	}
	catch (runtime_error e)
	{
		cerr << "Caught runtime error:" << endl << e.what() << endl;
		return 1;
	}
}

