# PeerStreamer-ng
PeerStreamer-ng [1] is P2P real-time streaming platform.
It is specifically designed for mesh networks and it is meant to be purely decentralized.

## Documentation
You can find documentation on usage and development on the official wiki:
https://ans.disi.unitn.it/redmine/projects/peerstreamer-ng/wiki

## Compilation
You need to install [Janus dependencies](https://github.com/meetecho/janus-gateway), on ubuntu:
```
sudo apt install libmicrohttpd-dev libjansson-dev libnice-dev \
	libssl-dev libsrtp-dev libsofia-sip-ua-dev libglib2.0-dev \
	libopus-dev libogg-dev libcurl4-openssl-dev pkg-config gengetopt \
	libtool automake libssl-dev
```

After that, you can get the executable by running:
``
$> make
``

To turn on all the debugging features, set the DEBUG environment variable:
``
$> DEBUG=1 make
``

## Test
PeerStreamer-ng comes with a unit test suite. It does require valgrind installed to run.
In the "test" folder are stored the test files. To run them and check code consistency run:
``
$> make tests
``

## Tutorial: Stream a web camera with WebRTC
_Tested on Ubuntu 17.10_

In this tutorial we are creating a streaming source first, 
 * Acquiring a media content from a local (usb) camera;
 * Encapsulating it in a RTP stream;
 * Creating a pstreamer P2P overlay;
 * Injecting the RTP stream in the overlay.

Then, we will launch PeerStreamer-ng to serve the P2P stream to the user through a browser supporting WebRTC.

#### The source
The following script captures a usb camera (/dev/video0) with FFmpeg which is used to stream it locally through RTP.
It is important to note, to be able to serve the stream through WebRTC, FFmpeg must transcode the video with VP8.
It also launches a source instance of pstreamer that takes such stream in input and use P2P mechanism to distribute it to the attaching peers.

Overall, this script creates a _PeerStreamer-ng channel_ at IP address 127.0.0.1 on port 6000 and it saves its description on a local channel list file called _channels.csv_ which we will use to feed PeerStreamer-ng with.

```
NAME="My Channel"
VIDEO=/dev/video0
SDPFILE="file://${PWD}/channel.sdp"
SOURCE_PEER_PORT=6000
RTP_BASE_PORT=5002
HOST_EXT_IP=127.0.0.1

echo "$NAME,$HOST_EXT_IP,$SOURCE_PEER_PORT,QUALITY,http://$HOST_EXT_IP:8000/channel.sdp" > channels.csv

ffmpeg -re -i ${VIDEO} -vcodec libvpx -deadline realtime -an -f rtp rtp://127.0.0.1:$((RTP_BASE_PORT+2))\
	-sdp_file $SDPFILE &
FFMPEG_PID=$!

Libs/pstreamer/pstreamer -p 0 -c "iface=lo,port=$SOURCE_PEER_PORT,chunkiser=rtp,audio=$RTP_BASE_PORT,\
	video=$((RTP_BASE_PORT+2)),addr=127.0.0.1,max_delay_ms=50" 
SOURCE_PEER_PID=$!

trap "kill $SOURCE_PEER_PID $FFMPEG_PID" SIGINT SIGTERM EXIT
```

You can download [the script above](https://ans.disi.unitn.it/redmine/attachments/download/118/webrtp_source_camera.sh) from the project site.

#### PeerStreamer-ng

The following command executes PeerStreamer-ng and it start the HTTP interfaces.
Pages are now served on port 3000 (overridable through the flags, see the command line help).

```
./peerstreamer-ng -c channel.csv
```

## References
[1] https://ans.disi.unitn.it/redmine/projects/peerstreamer-ng
