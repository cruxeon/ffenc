#include "ffenc.h"

#include <iostream>
using namespace std;


int main(int argc, char** argv)
{
	bool ret;
	FILE* f;
	const void* data;
	uint32_t size;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	Encoder* enc = new Encoder();
	char buf[] = "hello";
	

	f = fopen("out.mp4", "wb");
	if (!f)
	{
		fprintf(stderr, "Could not open file\n");
		return 1;
	}

	enc->initialize();
	ret = enc->configure();
	if (!ret) return 1;
	
	/* Encode 10 seconds worth of video */
	for (int i = 0; i < 250; i++)
	{
		//Create RAW Data and send as argument to encodeFrame

		ret = enc->encodeFrame(buf, sizeof(buf) - 1);
		if (!ret) return 1;

		if (enc->dataAvailable())
		{
			if (enc->lockBitstream(&data, &size))
			{
				fwrite(data, 1, size, f);
				enc->unlockBitstream();
			}
		}
	}
	
	/* add sequence end code to have a real mpeg file */
	fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	enc->shutdown();
	system("pause");

    return 0;
}