
#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>

#include "Thread.h"

using namespace std;

void
randomUSleep(int usec_min, int usec_max)
{
	usleep(usec_min + (usec_max - usec_min) * random() / RAND_MAX);
}

class MyClass
{
public:

	MyClass()
	{
		cout << "MyClass Object created." << endl;
	}

	void doWork(int maxCount)
	{
		for (int i = 1; i <= maxCount; i++)
		{
			cout << __FUNCTION__ << ": " << i << endl;
			randomUSleep(500000, 1000000);
		}
	}

	void startWork(int maxCount)
	{
		pthread_t handle = pthread_create_using_method<MyClass, int>(*this, &MyClass::doWork, maxCount);

		for (int i = 1; i <= maxCount; i++)
		{
			cout << __FUNCTION__ << ": " << i << endl;
			randomUSleep(500000, 800000);
		}

		int err = pthread_join(handle, NULL);
		if (err)
		{
			THROW_ERROR("pthread_join failed: " << strerror(err));
		}
	}

};

int main()
{
	try
	{
		MyClass myObject;
		myObject.startWork(20);
	}
	catch (runtime_error err)
	{
		cerr << "!! Caught runtime error:" << endl << err.what() << endl;
	}
}

