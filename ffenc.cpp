#include "ffenc.h"

/* Notes: */
//	1> Keeping structure of encodeFrame, lockBitstream, unlockBitstream
//	1> Assuming the above three functions are called in order in succession
//	1> Malloc'ing packet data in encodeFrame and free'ing it in unlockBitstream
//	1> Use sent and received counters like llenc instead?? (supports threading??)
//
//	2> Need to flush stdout and get delayed frames ??
//	2> If yes when? Perhaps shutdown(); ??
//



/* Library entry point */
DLL_EXPORT IEncoder* createEncoder()
{
	return new Encoder();
}


Encoder::Encoder()
{
	frameCount = 0;
	got_output = 0;
	c = NULL;
	addSeiData = false;
}


bool Encoder::initialize()
{	
	avcodec_register_all();
	return true;
}


bool Encoder::configure(int width, int height, int fps, int quality)
{
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		return false;
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		return false;
	}

	/* sample parameters */
	c->bit_rate = 400000;
	c->width = width;
	c->height = height;
	c->time_base = {1, fps};	/* frames per second */
	c->gop_size = 10;			/* emit one intra frame every ten frames */
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	av_opt_set(c, "preset", "slow", 0);
	av_opt_set(c, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);

	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		return false;
	}

	
	pixels = new PixelData(PixelData::FormatRgb, c->width, c->height);
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, c->width, c->height);
	myBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));


	myImgConvertCtx = sws_getContext(
		c->width, c->height, AV_PIX_FMT_RGB24,
		c->width, c->height,
		c->pix_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	

	return true;
}


void Encoder::addSei(char* sei_data, size_t lenData)
{
	/***************************** SEI NAL unit *****************************/
	/* \x00\x00\x01	- NAL unit identifier									*/
	/* \x06			- Indicating NAL unit of type 6 ie. SEI					*/
	/* \x05			- Indicating SEI of type 5 ie. User Data Unregistered	*/
	/* \x00			- Indicating size of SEI payload (1 byte)				*/
	/* <sei data>															*/
	/* \x80			- Indicates end of SEI NAL unit							*/
	/************************************************************************/

	if (lenData < 0 || lenData >= 255)
	{
		fprintf(stderr, "Invalid SEI size (Must be between 0 and 254)\n");
		addSeiData = false;
		return;
	}

	char sei_prefix_string_[] = "\x00\x00\x01\x06\x05\x00";		//last value is size -- setting it to zero default (reset in next line)
	sei_prefix_string_[5] = lenData;							// 0 < lenData < 255
	char sei_suffix_string_[] = "\x80";

	int lenPrefix = sizeof(sei_prefix_string_) - 1;
	int lenSuffix = sizeof(sei_suffix_string_) - 1;
	int sei_pkt_size = lenPrefix + lenData + lenSuffix;

	
	sei_msg = (char *)malloc(sei_pkt_size);
	memcpy(sei_msg, sei_prefix_string_, lenPrefix);
	memcpy(sei_msg + lenPrefix, sei_data, lenData);
	memcpy(sei_msg + lenPrefix + lenData, sei_suffix_string_, lenSuffix);

	addSeiData = true;
}


bool Encoder::encodeFrame(RenderTarget* source)
{	
	oassert(source != NULL);

	
	frameRGB = av_frame_alloc();
	if (!frameRGB) {
		fprintf(stderr, "Could not allocate video frameRGB\n");
		return false;
	}

	avpicture_fill((AVPicture *)frameRGB, myBuffer, AV_PIX_FMT_RGB24, c->width, c->height);
	//avpicture_fill((AVPicture *)frameRGB, NULL, AV_PIX_FMT_RGB24, c->width, c->height);

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		return false;
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;
	
	/* the image can be allocated by any means and av_image_alloc() is just the most convenient way if av_malloc() is to be used */
	if (av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32) < 0)
	{
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		return false;
	}


	PixelData* pixBuf = new PixelData(PixelData::FormatRgb, c->width, c->height);
	source->setReadbackTarget(pixBuf);
	
	source->readback();
	pixels = source->getOffscreenColorTarget();
	
	if (pixels != NULL)
	{
		byte* out = pixels->map();
		
		int p = pixels->getPitch();
		int h = c->height;
		for (int y = 0; y < h; y++)
		{
			memcpy(myBuffer + y * p, out + (h - 1 - y) * p, p);
		}
		//frameRGB->data[0] = out;
		pixels->unmap();
		pixels->setDirty();
		 
	}
	
	// scale from frameRGB (RGB data taken from pixels) into frame (YUV data) for h264 encoding
	sws_scale(myImgConvertCtx, frameRGB->data, frameRGB->linesize, 0, c->height, frame->data, frame->linesize);
	
	

	/* Temp Dummy Data */
	/*
	int i = frameCount + 1;
	for (int y = 0; y < c->height; y++) {
		for (int x = 0; x < c->width; x++) {
			frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
		}
	}
	// Cb and Cr
	for (int y = 0; y < c->height / 2; y++) {
		for (int x = 0; x < c->width / 2; x++) {
			frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
			frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
		}
	}
	*/
	
	frameCount++;
	frame->pts = frameCount;


	// COPY SEI NAL INFORMATION TO THE CORRECT LOCATION IN PIPELINE
	// This is a hack for libx264; done due to absense of API to access required structure
	if (addSeiData)
	{
		memcpy((char *)c->priv_data + 1192, sei_msg, sizeof(uint8_t *));
		memcpy((char *)c->priv_data + 1192 + sizeof(uint8_t *), &sei_pkt_size, sizeof(int));
		addSeiData = false;
	}
	

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	if (avcodec_encode_video2(c, &packet, frame, &got_output) < 0)
	{
		fprintf(stderr, "Error encoding frame\n");
		return false;
	}

	av_frame_free(&frameRGB);
	av_freep(&frame->data[0]);
	av_frame_free(&frame);

	return true;
}


bool Encoder::dataAvailable()
{
	if (got_output && packet.data != NULL) return true;
	return false;
}


bool Encoder::lockBitstream(const void** stptr, uint32_t* bytes)
{
	if (dataAvailable())
	{
		if (got_output == 0 || packet.data == NULL) 
		{
			fprintf(stderr, "WTF\n");			// WHY DOES THIS HAPPEN??? 
			return false;
		}
		*bytes = (uint32_t)packet.size;
		*stptr = packet.data;
		return true;
	}
	return false;
}


void Encoder::unlockBitstream()
{
	got_output = 0;
}


void Encoder::shutdown()
{
	av_packet_unref(&packet);	//where to do this?? is it even required?
	avcodec_close(c);
	av_free(c);
}
