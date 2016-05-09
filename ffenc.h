extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}

class Encoder//: public IEncoder
{
public:
	Encoder();

	bool initialize();
	bool configure(int width = 352, int height = 288, int fps = 25);
	bool encodeFrame(/*RenderTarget* source, */char* sei_data = NULL, size_t lenData = 0);
	bool dataAvailable();
	bool lockBitstream(const void** stptr, uint32_t* bytes);
	void unlockBitstream();
	void shutdown();

private:
	int frameCount;
	int got_output;

	char* sei_msg;

	//struct SwsContext* myImgConvertCtx;
	//PixelData* pixels;
	//uint8_t* myBuffer;

	AVCodec* codec;
	AVCodecContext* c;
	AVFrame *frame;
	//AVFrame *frameRGB;
	AVPacket packet;
};