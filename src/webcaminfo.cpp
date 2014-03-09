
#include <cstdio>      // printf
#include <cstdlib>
#include <cstring>
#include <iostream>    // cout
#include <memory>      // shared_ptr()
#include <stdexcept>   // runtime_exception
#include <sstream>     // stringstream
#include <string>      // strings
#include <vector>      // vectors

#include "Log.h"
#include "Webcam.h"

using namespace std;

const string DEFAULT_CAMERA = "/dev/video0";

int
main (int argc, char* args[]) {
	try
	{
		shared_ptr<Webcam> webcam;

		string filename;
		if (argc < 2) {
			filename = DEFAULT_CAMERA;
		} else {
			filename = args[1];
		}

		webcam = shared_ptr<Webcam>(new Webcam(filename));

		webcam->displayInfo();
		
		Webcam::fmtdesc_v supportedFormats = *(webcam->getSupportedFormats());
		MESSAGE("Supported formats:");
		for (Webcam::fmtdesc_v::iterator i = supportedFormats.begin();
		     i != supportedFormats.end();
		     i++)
		{
			MESSAGE(" - Format " << i->index << ": " << i->description << " ( "
			     << (i->flags & V4L2_FMT_FLAG_COMPRESSED ? "compressed " : "")
			     << (i->flags & V4L2_FMT_FLAG_EMULATED ? "emulated " : "")
			     << ")"
			);

			Webcam::resolution_set supportedResolutions = *(webcam->getSupportedResolutions(i->pixelformat));
			for (Webcam::resolution_set::iterator j = supportedResolutions.begin();
			     j != supportedResolutions.end();
			     j++)
			{
				MESSAGE("    - " << j->first << "x" << j->second << "px");
			}
		}
	}
	catch (runtime_error e)
	{
		cerr << "!! Exception thrown: " << e.what() << "\n";
		return 1;
	}

	return 0;
}

