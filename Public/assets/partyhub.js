// we assume all participants have different nicknames. This can be enforced with a control on input afterwards.

var server = null;
if(window.location.protocol === 'http:')
	server = "http://" + window.location.hostname + ":8088/janus";
else
	server = "https://" + window.location.hostname + ":8089/janus";

var local_nickname = "";
var room_name = "";
var channels = [];

function id2nickname(id) {
	return id.split(":")[0];
}

function id2roomname(id) {
	return id.split(":")[1];
}

function join_room() 
{
	var create_form = document.getElementById('creation_form');
	room_name = document.getElementById('room_name').value;
	local_nickname = document.getElementById('nickname').value;

	if (room_name != "" && local_nickname != "")
	{
		create_form.style.display = "none";
		document.getElementById('screenname').innerHTML = local_nickname;
		document.getElementById('vids').style.display = "block";
		init_room();
	}
	else
		bootbox.alert("Invalid room name or nickname", function() {});

}

function init_room()
{
	Janus.init({debug: "all", callback: function() {
		var janus_session = new Janus({
			server: server,
			success: function() {
				janus_session.attach(publisher_obj);
				setInterval(fetch_channels, 3000);
			},
			error: generic_error,
			destroyed: function () {}
		});
	}
	}
	);
}

function generic_error(error)
{
	Janus.error(error);
	bootbox.alert(error, function() {
		window.location.reload();
	});
}

publisher_obj = {
	plugin: "janus.plugin.videoroom",
	opaqueID: Janus.randomString(12),
	room_id: Math.floor((Math.random() * 10000000) + 1),
	publisher_id: Math.floor((Math.random() * 10000000) + 1),

	success: function(plugin_handle) {
		publisher_obj.handle = plugin_handle;
		Janus.log("Plugin attached");
		publisher_obj.request_room_creation();
	},

	request_room_creation: function () {
		var xhttp = new XMLHttpRequest();
		xhttp.onreadystatechange = function() {
			if (this.readyState == 4 && this.status == 200) {
				publisher_obj.start_streaming();
			}
			if (this.readyState == 4 && this.status != 200) {
				generic_error("An error occurred while creating the room");
			}
		};
		xhttp.open("POST", "/sources/" + publisher_obj.room_id, true);
		xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		xhttp.send();
	},

	start_streaming: function () {
		var register = { "request": "joinandconfigure", "bitrate": 128000, "room": publisher_obj.room_id, "ptype": "publisher", "display": local_nickname, "id": publisher_obj.publisher_id };
		publisher_obj.handle.send({"message": register});
		var xhttp = new XMLHttpRequest();
		xhttp.onreadystatechange = function() {
			if (this.readyState == 4 && this.status == 200) {
				Janus.debug("Streaming ");
			}
			if (this.readyState == 4 && this.status != 200) {
				generic_error("An error occurred with the streaming process");
			}
		};

		xhttp.open("UPDATE", "/sources/" + publisher_obj.room_id, true);
		xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		var params = "participant_id=" + encodeURIComponent(publisher_obj.publisher_id) + "&channel_name=" + encodeURIComponent(local_nickname+":"+room_name);
		xhttp.send(params);
	},

	error: function(error) {
		generic_error(error);
	},

	onmessage: function(msg, jsep) {
		Janus.debug(msg);
		var evt = msg["videoroom"];
		switch (evt) 
		{
			case "joined":
				Janus.debug("Joined");
				publisher_obj.send_sdp_offer();
				setInterval(publisher_obj.keep_alive, 3000);
				break;
			case "destroyed":
				generic_error("Total destruction");
				break;
		}
		if (jsep !== undefined && jsep !== null)
		{
			Janus.debug("Handling SDP");
			publisher_obj.handle.handleRemoteJsep({jsep: jsep});
		}
	},

	send_sdp_offer: function() {
		Janus.debug("Creating SDP offer");
		publisher_obj.handle.createOffer(
			{
				media: { audioRecv: false, videoRecv: false, audioSend: true, videoSend: true },
				simulcast: false,
				success: function(jsep) {
					Janus.debug("Got publisher SDP");
					var publish = { "request": "configure", "audio": true, "video": true, "audiocodec": "opus", "videocodec": "vp8" };
					publisher_obj.handle.send({"message": publish, "jsep": jsep});
				},
				error: function(error) {
					generic_error("WebRTC error");
				},
			});
		Janus.debug("SDP offer sent");
	},

	onlocalstream: function(stream) {
		var videotag = document.getElementById("myvideo");
		Janus.attachMediaStream(videotag, stream);
	},
	
	onremotestream: function(stream) {
		// nothing to do
	},

	oncleanup: function(stream) {
		// nothing to do
	},

	destroyed: function(stream) {
		// nothing to do
	},

	keep_alive: function() {
		var xhttp = new XMLHttpRequest();
		xhttp.open("UPDATE", "/sources/" + publisher_obj.room_id, true);
		xhttp.send();
	},

};

function RemoteFeed(id, ipaddr, port) {
	var streamer = null;

	var janus_session = new Janus({
		server: server,
		success: function() {
			streamer = new Streamer(id, ipaddr, port);
			janus_session.attach(streamer)},
		error: generic_error,
		destroyed: function () {}
	});

	this.id = function () {
		return streamer.id;
	}

	this.streamer = streamer;
	
	return this;
};

function RemoteObjs () {
	var feeds = new Set();

	this.add_streamer = function (id, ipaddr, port) {
		Janus.debug("Adding streamer " + id);
		obj = new RemoteFeed(id, ipaddr, port);
		feeds.add(obj);
	};

	this.del_streamer = function(id) {
		Janus.debug("Removing streamer " + id);
		obj = this.get_streamer_by_id(id);
		if (obj != null)
		{
			feeds.streamer.stop_stream();
			feeds.delete(obj);
		}
	};

	this.get_streamer_by_id = function(id) {
		feeds.forEach(function(feed) {
			if (feed.id() == id)
			{
				return feed.obj;
			}
		});
		return null;
	};

	return this;
};

function Streamer(id, ipaddr, port) {
	var id = id;
	var plugin = "janus.plugin.streaming";
	var opaqueID = Janus.randomString(12);
	var handle = null;
	var keep_alive_task = null;
	var ipaddr = ipaddr;
	var port = port;
	var rfid = null;

	this.plugin = plugin;
	this.opaqueID = opaqueID;

	this.success = function(plugin_handle) {
		handle = plugin_handle;
		Janus.log("Plugin attached");
		stream = this;

		var xhttp = new XMLHttpRequest();
		xhttp.onreadystatechange = function() {
			if (this.readyState == 4 && this.status == 200) {
				var ch = JSON.parse(this.responseText);
				stream.start_stream(ch);
			}
			if (this.readyState == 4 && this.status != 200) {
				bootbox.alert("An error occurred with channel");
				stream.stop_stream();
			}
		};
		rfid = Math.random().toString(36).substr(2, 8);
		xhttp.open("POST", "/channels/" + rfid, true);
		xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		var params = "ipaddr=" + encodeURIComponent(ipaddr) + "&port=" + encodeURIComponent(port);
		xhttp.send(params);
	};

	this.start_stream = function (ch) {
		Janus.log("Starting stream...");
		var body = { "request": "watch", id: parseInt(ch.janus_streaming_id) };
		handle.send({"message": body});
		var vids = document.getElementById("remote_vids");

		var div = document.createElement("div");
		div.classList.add("text-center");
		var vid = document.createElement("video");
		vid.id = id; 
		vid.classList.add("rounded");
		vid.classList.add("centered");
		vid.autoplay = true;
		var head = document.createElement("h3");
		head.classList.add("row");
		head.classList.add("text-center");
		head.innerHTML = id2nickname(ch.name);

		div.appendChild(head);
		div.appendChild(vid);
		vids.appendChild(div);

		keep_alive_task = setInterval(this.keep_alive, 3000);
	};

	this.stop_stream = function () {
		var body = { "request": "stop" };
		if (handle) {
			handle.send({"message": body});
			handle.hangup();
		}
		if (keep_alive_task)
			clearInterval(keep_alive_task);
	},

	this.error = function(error) {
		generic_error(error);
	};

	this.onmessage = function(msg, jsep) {
		Janus.debug(msg);
		var evt = msg["result"];
		switch (evt) 
		{
			case "starting":
				Janus.debug("Streaming starting..");
				break;
			case "statred":
				Janus.debug("Streaming started");
				break;
			case "stopped":
				Janus.debug("Streaming stopped");
				this.stop_stream();
				break;
		}
		if (jsep !== undefined && jsep !== null)
		{
			Janus.debug("Handling SDP for streaming...");
			handle.createAnswer( {
				jsep: jsep,
				media: { audioSend: false, videoSend: false },	// We want recvonly audio/video
				success: function(jsep) {
					Janus.debug("Got SDP!");
					Janus.debug(jsep);
					var body = { "request": "start" };
					handle.send({"message": body, "jsep": jsep});
				},
				error: function(error) {
					Janus.error("WebRTC error:", error);
					bootbox.alert("WebRTC error... " + JSON.stringify(error));
				}
			});
		}
	};

	this.onlocalstream = function(stream) {
		// nothing to do
	};
	
	this.onremotestream = function(stream) {
		var videotag = document.getElementById(id);
		/*
		if (videotag.length > 0) {
			var videoTracks = stream.getVideoTracks();
			if (videoTracks && videoTracks.length > 0 && !videoTracks[0].muted)
				videotag.show();
			return;
		}
		*/
		Janus.debug("::::WE GOT IT, " + id);
		Janus.debug(stream);
		Janus.attachMediaStream(videotag, stream);
	};

	this.oncleanup = function(stream) {
		// nothing to do
	};

	this.destroyed = function(stream) {
		// nothing to do
	};
	this.keep_alive = function() {
		if (rfid)
		{
			var xhttp = new XMLHttpRequest();
			xhttp.open("UPDATE", "/channels/" + rfid, true);
			xhttp.send();
		}
	};
};

function update_channels(chs)
{
	var to_add = [];
	var to_remove = [];

	for (nch in chs) {
		var found = false;
		if (id2roomname(chs[nch].name) == room_name) {
			for (och in channels) {
				if (channels[och].name == chs[nch].name)
				{
					found = true;
					break;
				}
			}
			if (!found)
				to_add.push(chs[nch]);
		}
	}
	for (och in channels) {
		var found = false;
			if (id2roomname(channels[och].name) == room_name) {
			for (nch in chs) {
				if (channels[och].name == chs[nch].name)
				{
					found = true;
					break;
				}
			}
			if (!found)
				to_remove.push(channels[och]);
		}
	}

	for (ch in to_add)
		remote_objs.add_streamer(to_add[ch].name, to_add[ch].ipaddr, to_add[ch].port);
	for (ch in to_remove)
		remote_objs.del_streamer(to_remove[ch].name);

	channels = chs;
}

function fetch_channels()
{
	Janus.log("Fetching channels");
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			var channels = JSON.parse(this.responseText);
			update_channels(channels);
		}
	};
	xhttp.open("GET", "/channels", true);
	xhttp.send();
}

remote_objs = new RemoteObjs();
