var server = null;
if(window.location.protocol === 'http:')
	server = "http://" + window.location.hostname + ":8088/janus";
else
	server = "https://" + window.location.hostname + ":8089/janus";

var janus = null;
var streaming = null;
var opaqueId = "streamingtest-"+Janus.randomString(12);

var simulcastStarted = false;
var spinner = null;


$(document).ready(function() {
	// Initialize the library (all console debuggers enabled)
	createJanus();
});

function startStream(selectedStream) {
	stopStream();
	Janus.log("Selected video id #" + selectedStream);
	if(selectedStream === undefined || selectedStream === null) {
		bootbox.alert("Select a stream from the list");
		return;
	}
	var body = { "request": "watch", id: parseInt(selectedStream) };
	streaming.send({"message": body});
	// No remote video yet
	$('#stream').append('<video class="rounded centered" id="waitingvideo" width=320 height=240 />');
	if(spinner == null) {
		var target = document.getElementById('stream');
		spinner = new Spinner({top:100}).spin(target);
	} else {
		spinner.spin();
	}
}

function stopStream() {
	var body = { "request": "stop" };
	if (streaming) {
		streaming.send({"message": body});
		streaming.hangup();
	}
	//destroy janus?
}

function createJanus() {
	Janus.init({debug: "all", callback: function() {
		// Create session
		janus = new Janus(
			{
				server: server,
				success: function() {
					// Attach to streaming plugin
					janus.attach(
						{
							plugin: "janus.plugin.streaming",
							opaqueId: opaqueId,
							success: function(pluginHandle) {
								streaming = pluginHandle;
								Janus.log("Plugin attached! (" + streaming.getPlugin() + ", id=" + streaming.getId() + ")");
							},
							error: function(error) {
								Janus.error("  -- Error attaching plugin... ", error);
								bootbox.alert("Error attaching plugin... " + error);
							},
							onmessage: function(msg, jsep) {
								Janus.debug(" ::: Got a message :::");
								Janus.debug(msg);
								var result = msg["result"];
								if(result !== null && result !== undefined) {
									if(result["status"] !== undefined && result["status"] !== null) {
										var status = result["status"];
										if(status === 'starting')
											$('#status').text("Starting, please wait...").show();
										else if(status === 'started')
											$('#status').text("Started").show();
										else if(status === 'stopped')
											stopStream();
									} 
								} else if(msg["error"] !== undefined && msg["error"] !== null) {
									bootbox.alert(msg["error"]);
									stopStream();
									return;
								}
								if(jsep !== undefined && jsep !== null) {
									Janus.debug("Handling SDP as well...");
									Janus.debug(jsep);
										// Answer
									streaming.createAnswer(
										{
											jsep: jsep,
											media: { audioSend: false, videoSend: false },	// We want recvonly audio/video
											success: function(jsep) {
												Janus.debug("Got SDP!");
												Janus.debug(jsep);
												var body = { "request": "start" };
												streaming.send({"message": body, "jsep": jsep});
											},
											error: function(error) {
												Janus.error("WebRTC error:", error);
												bootbox.alert("WebRTC error... " + JSON.stringify(error));
											}
										});
								}
								},
								onremotestream: function(stream) {
									Janus.debug(" ::: Got a remote stream :::");
									Janus.debug(stream);
									if($('#remotevideo').length > 0) {
										// Been here already: let's see if anything changed
										var videoTracks = stream.getVideoTracks();
										if(videoTracks && videoTracks.length > 0 && !videoTracks[0].muted) {
											if($("#remotevideo").get(0).videoWidth)
												$('#remotevideo').show();
										}
										return;
									}
									$('#stream').append('<video class="rounded centered hide" id="remotevideo" width=320 height=240 autoplay/>');
									// Show the stream and hide the spinner when we get a playing event
									$("#remotevideo").bind("playing", function () {
										if(this.videoWidth)
											$('#remotevideo').removeClass('hide').show();
										if(spinner !== null && spinner !== undefined)
											spinner.stop();
										spinner = null;
										$('#waitingvideo').hide();
										var videoTracks = stream.getVideoTracks();
										if(videoTracks === null || videoTracks === undefined || videoTracks.length === 0)
											return;
										var width = this.videoWidth;
										var height = this.videoHeight;
										if(Janus.webRTCAdapter.browserDetails.browser === "firefox") {
											// Firefox Stable has a bug: width and height are not immediately available after a playing
											setTimeout(function() {
												var width = $("#remotevideo").get(0).videoWidth;
												var height = $("#remotevideo").get(0).videoHeight;
											}, 2000);
										}
									});
									var videoTracks = stream.getVideoTracks();
									if(videoTracks && videoTracks.length &&
											(Janus.webRTCAdapter.browserDetails.browser === "chrome" ||
												Janus.webRTCAdapter.browserDetails.browser === "firefox" ||
												Janus.webRTCAdapter.browserDetails.browser === "safari")) {
										$('#curbitrate').removeClass('hide').show();
									}
									Janus.attachMediaStream($('#remotevideo').get(0), stream);
									var videoTracks = stream.getVideoTracks();
									if(videoTracks === null || videoTracks === undefined || videoTracks.length === 0 || videoTracks[0].muted) {
										// No remote video
										$('#remotevideo').hide();
										$('#stream').append(
											'<div id="novideo" class="no-video-container">' +
												'<i class="fa fa-video-camera fa-5 no-video-icon"></i>' +
												'<span class="no-video-text">Loading video</span>' +
											'</div>');
									}
								},
								oncleanup: function() {
									Janus.log(" ::: Got a cleanup notification :::");
									$('#remotevideo').remove();
									$('.no-video-container').remove();
								}
							});
					},
					error: function(error) {
						Janus.error(error);
						bootbox.alert(error, function() {
							window.location.reload();
						});
					},
					destroyed: function() {
						window.location.reload();
					}
				});
	}});
}

refresh_channels();
var refresh_channels_id = setInterval(refresh_channels, 5000);
var update_channel_id = setInterval(update_channel, 3000);
var channel = "";
