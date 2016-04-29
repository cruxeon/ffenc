//Author: cruxeon
//FFMPEG encoder class to encode raw frames into H.264 stream

#include <omega.h>
#include <omegaToolkit.h>

#ifdef OMEGA_OS_WIN
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C"
#endif

extern "C" {
	#include <libavutil/opt.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/pixfmt.h>
	#include "libavutil/avconfig.h"

	#include <libavutil/opt.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/common.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/samplefmt.h>
}

#if LIBAVCODEC_VERSION_MAJOR < 57
    #define AV_PIX_FMT_RGBA PIX_FMT_RGBA
    #define av_frame_alloc avcodec_alloc_frame
#endif

using namespace omega;
using namespace omegaToolkit;

//Global Variables
AVFormatContext *ofmt_ctx;


// FFMPEG Encoder class.
class Encoder: public IEncoder
{
public:
    Encoder();

    bool configure(int width, int height, int fps = 30, int quality = 100);
    void shutdown();

    bool encodeFrame(RenderTarget* rt);
    bool dataAvailable();
    bool lockBitstream(const void** stptr, uint32_t* bytes);
    void unlockBitstream();

private:
	int flush_encoder(unsigned int stream_index);
	int encodeFrameHelper(AVFrame *frame, unsigned int stream_index, int *got_frame);
    bool myTransferObjectInitialized;
    
    AVFrame *frame;
    enum AVMediaType type;
    unsigned int stream_index;
    int got_frame;
	int width;
	int height;
	int numStreams;

	AVStream *out_stream;
	AVCodecContext *enc_ctx;
	AVCodec *encoder;
	AVPacket enc_pkt;
	
    int myMaxOutstandingTransfers;
    int myReceivedTransfers;
    int myTransferCounter;
};

////////////////////////////////////////////////////////////////////////////////
// Library entry point.
//WHAT IS THIS FOR?
/*
DLL_EXPORT IEncoder* createEncoder() 
{ return new Encoder(); }
*/    
////////////////////////////////////////////////////////////////////////////////
Encoder::Encoder()
{
    myTransferObjectInitialized = false;
	frame = NULL;
    myMaxOutstandingTransfers = 4;
    myReceivedTransfers = 0;
    myTransferCounter = 0;
	width = 0;
	height = 0;
	numStreams = 1; //Need a reason to modify this?? 
	
	av_register_all();
    avformat_network_init();
}

////////////////////////////////////////////////////////////////////////////////
bool Encoder::configure(int w, int h, int fps, int quality)
{
    olog(Debug, "[Encoder::initialize]");
    width = w;
	height = h;
	
    if(myTransferObjectInitialized)
    {
        myTransferCounter = 0;
        myReceivedTransfers = 0;
    }
    
	
	
	
	
    int ret;

    ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "rtsp", NULL);		//Edit: to plugin to sample code //last entry == filename
    
	if (!ofmt_ctx) {
		olog(Debug, "AVERROR_UNKNOWN");
		return false;
	}

		
	//Allocating output stream
	out_stream = avformat_new_stream(ofmt_ctx, NULL);
	if (!out_stream) {
		av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
		//return AVERROR_UNKNOWN;
	}
        
	enc_ctx = out_stream->codec;

	encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!encoder) {
		av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
		//return AVERROR_INVALIDDATA;
	}
	
	
	// Find values for commented values (NEED TO FIX)
	enc_ctx->height = height;
	enc_ctx->width = width;
	//enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
	enc_ctx->pix_fmt = AV_PIX_FMT_RGB32_1;	//IS THIS CORRECT??????????????????????????????
	//enc_ctx->time_base = dec_ctx->time_base;
	
	//additional information to create h264 encoder
	enc_ctx->bit_rate = width * height * 5.69;		// taken from llenc
	enc_ctx->gop_size = 10;
	enc_ctx->qmin = 10;
	enc_ctx->qmax = 51;
	
	av_opt_set(enc_ctx, "preset", "slow", 0);	//enc_ctx->priv_data? 
	
	
	ret = avcodec_open2(enc_ctx, encoder, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder\n");
		return false;
	}

	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Unable to write stream header to output file\n");
        return false;
    }

    olog(Verbose, "ffenc Encoder initialized");
    myTransferObjectInitialized = true;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Encoder::shutdown()
{
    olog(Verbose, "ffenc Encoder shutdown");
    
	int ret;
	int i;
	
	//Flush encoders
    for (i = 0; i < numStreams; i++) {
        ret = flush_encoder(i);
        av_log(NULL, AV_LOG_DEBUG, "Flushing encoder (i)\n");
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
            break;
        }
    }

    av_write_trailer(ofmt_ctx);

    av_frame_free(&frame);

    for (i = 0; i < numStreams; i++) {
        
        //Freeing output codec data
        if (ofmt_ctx && ofmt_ctx->nb_streams > i
                && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
            avcodec_close(ofmt_ctx->streams[i]->codec);
    }
    
    //Closing i/o AVFormatContexts
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

	if (ret < 0) {
		olog(Debug, "Error Occurred");
		//av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
	}

}

////////////////////////////////////////////////////////////////////////////////

//Function to flush encoder and write to output frame
int Encoder::flush_encoder(unsigned int stream_index) {

    int ret;
    int got_frame;
    
    if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;
    
    while (1) {

        //av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
        ret = encodeFrameHelper(NULL, stream_index, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame)
            return 0;
    }

    return ret;
}


////////////////////////////////////////////////////////////////////////////////
bool Encoder::encodeFrame(RenderTarget* source)
{
    oassert(source != NULL);
	int ret;

    while(myTransferCounter - myReceivedTransfers > myMaxOutstandingTransfers)
    {
		//wait?
        //myDataReleasedEvent.wait();
    }

	stream_index = 0;		//iterate for multiple streams	//Todo: Make more generic
		
	frame = av_frame_alloc();
	if (!frame) {
		ret = AVERROR(ENOMEM);
		return false;
	}

	//frame = SOURCE //TODO: FIX NOW!!!!
	source->readback();
	PixelData* pixels = source->getOffscreenColorTarget();				//Assuming RGBA?? 
	
	/* the image can be allocated by any means and av_image_alloc() is
	* just the most convenient way if av_malloc() is to be used */
	ret = av_image_alloc(frame->data, frame->linesize, enc_ctx->width, enc_ctx->height,
		enc_ctx->pix_fmt, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		exit(1);
	}

	// Convert image into native format
	avpicture_fill((AVPicture *)frame, (uint8_t *)pixels, AV_PIX_FMT_RGBA, width, height);
	
	
	/*
	av_image_alloc();
	frame->data[0] = source;
	*/
	
	
	frame->pts = av_frame_get_best_effort_timestamp(frame); // Check what this does and if required

	ret = encodeFrameHelper(frame, stream_index, NULL);
		
	if (ret < 0) {
			shutdown();
			return false;
		}
	
	
	av_frame_free(&frame);
	myTransferCounter++;
    return true;
}


///////////////////////////////////////////////////////////////////////////////////
	
int Encoder::encodeFrameHelper(AVFrame *frame, unsigned int stream_index, int *got_frame) {
    
    int ret;
    int got_frame_local;
    
    AVCodecContext *c_ctx = ofmt_ctx->streams[stream_index]->codec;

    if (!got_frame)
        got_frame = &got_frame_local;

    //Encoding the frame
	av_init_packet(&enc_pkt);
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    
    
   
    /*Write SEI NAL unit*/
	// Modify as required once this works :)
    int len = 12;		
    
    //sei payload
    unsigned char buf[] ="\x00\x00\x01\x06\x05\x05hello\x80";			//prefix to indicate it's a sei message of type 5 //postfix to denote end of message
    unsigned char *buf2 = (unsigned char *)malloc(sizeof(char)*len);
    unsigned int i;
    for (i = 0; i < len; i++) {
        buf2[i] = buf[i];
    }
   
    //COPY SEI NAL INFORMATION TO THE CORRECT LOCATION IN PIPELINE
	memcpy((AVCodecContext *)c_ctx->priv_data + 1192, &buf2, sizeof(uint8_t *));
	memcpy((AVCodecContext *)c_ctx->priv_data + 1200, &len, sizeof(int));


    ret = avcodec_encode_video2(c_ctx, &enc_pkt, frame, got_frame);

    if (ret < 0)
        return ret;
    if (!(*got_frame))
        return 0;


	/***** DONT NEED THIS?  ******/
	/*
	//Prepare packet for muxing
    enc_pkt.stream_index = stream_index;
    av_packet_rescale_ts(&enc_pkt,c_ctx->time_base, ofmt_ctx->streams[stream_index]->time_base);
    
    //Mux encoded frame
    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
	*/
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
bool Encoder::dataAvailable()
{
    //ofmsg("TransferPending %1%    Received %2%", %myTransferCounter %myReceivedTransfers);   
    return (myTransferCounter > myReceivedTransfers);
}

////////////////////////////////////////////////////////////////////////////////
bool Encoder::lockBitstream(const void** stptr, uint32_t* bytes)
{
    while(myTransferCounter == myReceivedTransfers) {
		//myTransferStartedEvent.wait();
	}
    //ofmsg("lock %1%      %2%", %myReceivedTransfers %myTransferCounter);

	//TODO: send enc_pkt.data & enc_pkt.size
	stptr = (const void**)&enc_pkt.data;
	bytes = (uint32_t*)&enc_pkt.size;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Encoder::unlockBitstream()
{
    myReceivedTransfers++;
	av_free_packet(&enc_pkt);
}