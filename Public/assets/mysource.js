var server = null;
if(window.location.protocol === 'http:')
	server = "http://" + window.location.hostname + ":8088/janus";
else
	server = "https://" + window.location.hostname + ":8089/janus";

var janus = null;
var janus_plugin = null;
var myid;
var myroom;
var description;
var opaqueId = "streamingtest-"+Janus.randomString(12);
var bitrate = 128000;

$(document).ready(function() {
	// Initialize the library (all console debuggers enabled)
	myroom = Math.floor((Math.random() * 10000000) + 1);
	myid = Math.floor((Math.random() * 10000000) + 1);
	createJanus();
});

function handleMessage(msg){
	var event = msg["videoroom"];
	Janus.debug("Event: " + event);
	if(event != undefined && event != null) {
		if(event === "joined") {
			// Publisher/manager created, negotiate WebRTC and attach to existing feeds, if any
			myid = msg["id"];
			mypvtid = msg["private_id"];
			Janus.log("Successfully joined room " + msg["room"] + " with ID " + myid);
			publishOwnFeed(true);
			// Any new feed to attach to?
			if(msg["publishers"] !== undefined && msg["publishers"] !== null) {
				var list = msg["publishers"];
				Janus.debug("Got a list of available publishers/feeds:");
				Janus.debug(list);
				for(var f in list) {
					var id = list[f]["id"];
					var display = list[f]["display"];
					var audio = list[f]["audio_codec"];
					var video = list[f]["video_codec"];
					Janus.debug("  >> [" + id + "] " + display + " (audio: " + audio + ", video: " + video + ")");
				}
			}
		} else if(event === "destroyed") {
			// The room has been destroyed
			Janus.warn("The room has been destroyed!");
			bootbox.alert("The room has been destroyed", function() {
			});
		}
	}
}

function publishOwnFeed(useAudio) {
	// Publish our stream
	janus_plugin.createOffer(
		{
			// Add data:true here if you want to publish datachannels as well
			media: { audioRecv: false, videoRecv: false, audioSend: useAudio, videoSend: true },	// Publishers are sendonly
			// If you want to test simulcasting (Chrome and Firefox only), then
			// pass a ?simulcast=true when opening this demo page: it will turn
			// the following 'simulcast' property to pass to janus.js to true
			simulcast: false,
			success: function(jsep) {
				Janus.debug("Got publisher SDP!");
				Janus.debug(jsep);
				var publish = { "request": "configure", "audio": useAudio, "video": true, "audiocodec": "opus", "videocodec": "vp8" };
				// You can force a specific codec to use when publishing by using the
				// audiocodec and videocodec properties, for instance:
				// 		publish["audiocodec"] = "opus"
				// to force Opus as the audio codec to use, or:
				// 		publish["videocodec"] = "vp9"
				// to force VP9 as the videocodec to use. In both case, though, forcing
				// a codec will only work if: (1) the codec is actually in the SDP (and
				// so the browser supports it), and (2) the codec is in the list of
				// allowed codecs in a room. With respect to the point (2) above,
				// refer to the text in janus.plugin.videoroom.cfg for more details
				janus_plugin.send({"message": publish, "jsep": jsep});
			},
			error: function(error) {
				Janus.error("WebRTC error:", error);
				if (useAudio) {
					 publishOwnFeed(false);
				} else {
					bootbox.alert("WebRTC error... " + JSON.stringify(error));
					$('#publish').removeAttr('disabled').click(function() { publishOwnFeed(true); });
				}
			}
		});
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
							plugin: "janus.plugin.videoroom",
							opaqueId: opaqueId,
							success: function(pluginHandle) {
								janus_plugin = pluginHandle;
								Janus.log("Plugin attached! (" + janus_plugin.getPlugin() + ", id=" + janus_plugin.getId() + ")");
								requestRoomCreation();
							},
							error: function(error) {
								Janus.error("  -- Error attaching plugin... ", error);
								bootbox.alert("Error attaching plugin... " + error);
							},
							onmessage: function(msg, jsep) {
								Janus.debug(" ::: Got a message :::");
								Janus.debug(msg);
								handleMessage(msg);
								if(jsep !== undefined && jsep !== null) {
									Janus.debug("Handling SDP as well...");
									Janus.debug(jsep);
									janus_plugin.handleRemoteJsep({jsep: jsep});
								}
								},
								onlocalstream: function(stream) {
									Janus.debug(" ::: Got a local stream :::");
									mystream = stream;
									Janus.debug(stream);
									Janus.attachMediaStream($('#myvideo').get(0), stream);
									$("#myvideo").get(0).muted = "muted";
									var videoTracks = stream.getVideoTracks();
									if(videoTracks === null || videoTracks === undefined || videoTracks.length === 0) {
										// No webcam
										$('#plugin').append(
											'<div class="no-video-container">' +
												'<i class="fa fa-video-camera fa-5 no-video-icon" style="height: 100%;"></i>' +
												'<span class="no-video-text" style="font-size: 16px;">No webcam available</span>' +
											'</div>');
									}
								},
								onremotestream: function(stream) {
									Janus.debug(" ::: Got a remote stream :::");
									Janus.debug(stream);
								},
								oncleanup: function() {
									Janus.log(" ::: Got a cleanup notification :::");
									mystream = null;
									$('#myvideo').remove();
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
					}
				});
	}});
}

function startStreaming() {
	var radios = document.getElementsByName('bitrate_radio');
	var i=0;
	while(i < radios.length)
	{
		if (radios[i].checked)
		{
			bitrate = Number(radios[i].value);
			description = description + "@" + (bitrate/1000).toString() + "K";
			i = radios.length;
		}
		i++;
	}

	var register = { "request": "joinandconfigure", "bitrate": bitrate, "room": myroom, "ptype": "publisher", "display": description, "id": myid };
	janus_plugin.send({"message": register});

	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			set_state("Streaming " + description);
			$('#creation_form').hide();
		}
		if (this.readyState == 4 && this.status != 200) {
			set_state("&lt;An error occurred with the streaming process&gt;");
		}
	};

	xhttp.open("UPDATE", "/sources/" + myroom, true);
	xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	var params = "participant_id=" + encodeURIComponent(myid) + "&channel_name=" + encodeURIComponent(description);
	xhttp.send(params);
}

function requestRoomCreation() {
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			set_state("Room created");
			var btn = document.getElementById("submit");
			btn.onclick = function(){	
				description = document.getElementById("description").value;
				startStreaming();
			};
		}
		if (this.readyState == 4 && this.status != 200) {
			set_state("&lt;An error occurred with room creation&gt;");
		}
	};
	xhttp.open("POST", "/sources/" + myroom, true);
	xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhttp.send();
}

setInterval(update_source, 3000);
