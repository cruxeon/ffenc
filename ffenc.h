#include <omega.h>
#include <omegaToolkit.h>


extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}

#ifdef OMEGA_OS_WIN
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C"
#endif

using namespace omega;
using namespace omegaToolkit;


class Encoder : public IEncoder
{
public:
	Encoder();

	bool initialize();
	bool configure(int width = 352, int height = 288, int fps = 25, int quality = 100);
	//bool encodeFrame(RenderTarget* source, char* sei_data = NULL, size_t lenData = 0);
	bool encodeFrame(RenderTarget* source);
	bool dataAvailable();
	bool lockBitstream(const void** stptr, uint32_t* bytes);
	void unlockBitstream();
	void shutdown();

private:
	int frameCount;
	int got_output;

	char* sei_msg;

	struct SwsContext* myImgConvertCtx;
	PixelData* pixels;
	uint8_t* myBuffer;

	AVCodec* codec;
	AVCodecContext* c;
	AVFrame *frame;
	AVFrame *frameRGB;
	AVPacket packet;
};