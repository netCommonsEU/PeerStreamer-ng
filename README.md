# PeerStreamer-ng (ffmpeg based_)
PeerStreamer-ng [1] is P2P rel-time streaming platform.
It is specifically design for mesh networks and it is meant to be purely decentralized.

## Documentation
You can find documentation on usage and development on the official wiki:
https://ans.disi.unitn.it/redmine/projects/peerstreamer-ng/wiki

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
PeerStreamer-ng comes with a unit test suite. It does require valgrind installed to run.
In the "test" folder are stored the test files. To run them and check code consistency run:
``
$> make tests
``

## Example
You can run a test streaming network by following the example documentation [2]

## References
[1] https://ans.disi.unitn.it/redmine/projects/peerstreamer-ng
[2] https://ans.disi.unitn.it/redmine/projects/peerstreamer-ng/wiki/Streaming_a_live_camera
