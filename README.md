# PeerStreamer-ng (ffmpeg based)

PeerStreamer-ng [1] is P2P rel-time streaming platform.  It is specifically
design for mesh networks and it is meant to be purely decentralized.

## Compilation

You can get the executable by running:

``
$> make
``

To turn on all the debugging features, set the DEBUG environment variable:

``
$> DEBUG=1 make
``

## Test

PeerStreamer-ng comes with a unit test suite. It does require valgrind
installed to run.  In the "test" folder are stored the test files. To run them
and check code consistency run:

``
$> make tests
``

## Example

In test\_script directory there are a few scripts that can be used to perform a
quick test of PeerStreamer-ng using the loopback interface. The test scenario
is very simple as it involves only a single PeerStreamer source.

Currently the scripts have been tested only on Ubuntu 16.04LTS.

Currenlty this version of PeerStreamer-ng supports only the Desktop version fo
Chrome browser.

NOTE: for installing ffmpeg and downloading the video file you need an Internet
connection.

First of all, the scripts relies on a custom built ffmpeg version for streaming
the video. Dont' use the ffmpeg version shipped by the package manager of your
OS distribution.

You can use the script ffmpeg\_install.sh for building the ffmpeg tool with the
required configuration. By default the custom ffmpeg tool will be installed in
${HOME}/ffmpeg-src/install. You can change this by modifing the value of the
variable FFDIR inside the ffmpeg\_install.sh script. Once the FFDIR variable
has been set to the desired value, run the following command:

```
./ffmpeg_install.sh
```

Then you will need a video to stream. You can download the one we used for
testing [sintel.m4v](https://drive.google.com/uc?id=1Phz0HYduIH2X2eMWvYIhUnVzq8_TuqhR&export=download)

Alternatively, you can generate your own file by using h264 encoding for the
video and AAC encoding for the audio. Moreover, we currently tested
PeerStreamer-ng using a video/audio file with the following characteristics:
for the h264 encoding we used the main profile with level 3.1, a constant frame
rate of 30fps and a constant bitrate of 2000kbps. For the AAC encoding,
instead, we used a sampling rate of 48kHz with a constant bitrate of 160kbps.

Once you have both the custom ffmpeg version and the video to stream you can
use the scripts start\_psng.sh and rtp\_source.sh for running the actual test.
However, before proceeding, you must open the script start\_psng.sh and set the
value of the variable PSNGBASEDIR to the directory where you cloned
PeerStreamer-ng. Then open the script rtp\_source.sh and modify the following
variables:

* FFMPEGBIN: this must point to the ffmpeg executable you built with the script
  ffmpeg\_install.sh

* PSTREAMERBIN: this must point to the pstreamer executable. If $PS is the
  directory where PeerStreamer-ng has been cloned, then the pstreamer
  executable is located in $PS/Libs/pstreamer/pstreamer.

* VNAME: thism must be set to the full path name of the video file.

At this point you can execute the source pstreamer by running in a terminal tab
the following command:

```
./rtp_source.sh 30000 5001
```

where the first parameter is the port used by the source pstreamer and the
second parameter is the RTP base port.

In a second terminal tab execute the following command for starting
peerstreamer-ng:

```
./start-psng.sh
```

Finally, use Chrome for connecting to http://localhost:3000 and select the
"Sintel" channel from the list on the right.


## References

[1] https://ans.disi.unitn.it/redmine/projects/peerstreamer-ng

