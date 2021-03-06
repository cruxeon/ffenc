ffenc is a framework that uses FFMPEG to encode raw frames into H.264 video streams. It is an addon to the porthole module to facilitate low latency h.264 streaming of VR content to client nodes.

### Global Functions ###

#### initialize ####
> bool initialize()

initialized the ffmpeg callbacks required for the encoder.

#### configure ####
> bool configure(int width = 352, int height = 288, int fps = 25);

Locates the H264 encoder, configures the codec & opens it
- `width` : width of each frame of the stream. Default: 352
- `height` : height of each frame of the stream. Default: 288
- `fps` : frames per second for the created video stream. Default: 25

This function is also responsible for configuring other variables needed to encode raw frames from PixelData.

#### encodeFrame ####
> bool encodeFrame(RenderTarget* source);

Encodes the raw data provided by Omegalib Returns `true` on successfully encoding the given frame.

#### dataAvailable ####
> bool dataAvailable()

Returns `true` if there exists an encoded frame to be transferred.

#### lockBitstream ####
> bool lockBitstream(const void** stptr, uint32_t* bytes)

Coppies the encoded frame data & its size to the pointers passed to it.

#### unlockBitstream ####
> void unlockBitstream()

Frees the frame data.

#### shutdown ####
> void shutdown()

Frees all memory instances.