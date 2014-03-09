// Example of passing Plain-Old-Data structures to STL data structure constructors

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

int main (int argc, char* args[])
{

/// Using vectors (ArrayLists):

	vector<string> mystrs ({
		"apple",
		"bananna",
		"cherry",
		"date",
		"elderberry",
		"fig",
		"grape"
	});

	for_each(
		mystrs.begin(),
		mystrs.end(),

		[] (string i) {
			cout << i << endl;
		}
	);

//// Using maps:

	map<int, string> mymap({
		{1, "apple"},
		{3, "bananna"},
		{14, "cherry"},
		{5, "date"}
	});

	for_each(
		mymap.begin(),
		mymap.end(),
		
		[] (pair<int, string> i)
		{
			cout << i.first << ": " << i.second << endl;
		}
	);
}
