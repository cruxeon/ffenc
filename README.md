ffenc is a framework that uses FFMPEG to encode raw frames into H.264 video streams. It is an addon to the porthole module to facilitate low latency streaming of VR content to client nodes.

### Global Functions ###

#### configure ####
> configure(int width, int height, int fps = 30, int quality = 100);

Configures the FFMPEG H.264 Encoder
- `width` : width of each frame of the stream
- `height` : height of each frame of the stream
- `fps` (optional) : frames per second for the created video stream. Default: 30
- `quality` (optional) : ??? Default: 100

#### shutdown ####
> shutdown()

Destroys the encoder instance and frees all memory instances.

#### encodeFrame ####
> encodeFrame(renderTarget *rt)

Encodes a raw video frame into an H.264 video stream
- `rt` : raw video frame to be encoded
