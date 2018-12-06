// we assume all participants have different nicknames. This can be enforced with a control on input afterwards.

var server = null;
if(window.location.protocol === 'http:')
	server = "http://" + window.location.hostname + ":8088/janus";
else
	server = "https://" + window.location.hostname + ":8089/janus";

var init = false;
var local_nickname = "";
var room_name = "";
var id_separator = ":";
var channels = [];
var channel_name = "";
var bitrate = 64000;

function id2nickname(id) {
	return id.split(id_separator)[0];
}

function id2roomname(id) {
	return id.split(id_separator)[1];
}

function id2containername(id) {
	return id + "_div";
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
		var register = { "request": "joinandconfigure", "bitrate": bitrate, "room": publisher_obj.room_id, "ptype": "publisher", "display": local_nickname, "id": publisher_obj.publisher_id };
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
		var params = "participant_id=" + encodeURIComponent(publisher_obj.publisher_id) + "&channel_name=" + encodeURIComponent(channel_name);
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
				media: { audioRecv: false, videoRecv: false, audioSend: true, videoSend: document.getElementById("videoactive").checked },
				simulcast: false,
				success: function(jsep) {
					Janus.debug("Got publisher SDP");
					var publish = { "request": "configure", "audio": true, "video": document.getElementById("videoactive").checked, "audiocodec": "opus", "videocodec": "vp8" };
					publisher_obj.handle.send({"message": publish, "jsep": jsep});
				},
				error: function(error) {
					Janus.error("WebRTC error:", error);
					// generic_error("WebRTC error");
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
	var id = id;
	var stat = "loading";

	var janus_session = new Janus({
		server: server,
		success: function() {
			streamer = new Streamer(id, ipaddr, port);
			stat = "loaded";
			janus_session.attach(streamer)},
		error: generic_error,
		destroyed: function () {}
	});

	this.id = function () {
		return id;
	}

	this.streamer = function () {
		return streamer;
	}

	this.stat = function() {
		return stat;
	}
	
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
		var feed = null;
		a = Array.from(feeds);
		for (var f in a) {
			if (a[f].id() == id)
				feed = a[f];
		}
		if (feed != null && feed.stat() == "loaded")
		{
			feed.streamer().stop_stream();
			feeds.delete(feed);
			
			let cont = document.getElementById(id2containername(id));
			if (cont)
				document.getElementById("remote_vids").removeChild(cont);
		}// else
		//	bootbox.alert("Could not delete video of " + id2nickname(id));
	};

	this.get_streamer_by_id = function(id) {
		var obj = null;
		a = Array.from(feeds);
		for (var feed in a) {
			if (a[feed].id() == id && a[feed].stat() == "loaded")
				obj = a[feed].streamer();
		}

		return obj;
	};

	this.get_streamer_status = function(id) {
		var res = "absent";
		a = Array.from(feeds);
		for (var feed in a) {
			if (a[feed].id() == id)
				res = a[feed].stat();
		}
		return res;
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
	this.id = id;

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
				bootbox.alert("An error occurred with channel (" + this.status + ")");
				remote_objs.del_streamer(id);
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
		div.id = id2containername(ch.name); 
		div.classList.add("panel");
		div.classList.add("panel-success");
		div.classList.add("remote-video");
		var vid = document.createElement("video");
		vid.id = id; 
		vid.classList.add("embed-responsive-item");
		vid.classList.add("panel-body");
		vid.autoplay = true;
		vid.rounded = true;
		vid.controls = true;
		var head = document.createElement("div");
		head.classList.add("panel-heading");
		head.classList.add("text-center");
		head.innerHTML = id2nickname(ch.name) + " - ";

		var reload_btn = document.createElement("span");
		reload_btn.innerHTML = "&#9851;";
		reload_btn.title = "Recycle";
		reload_btn.style.cursor = "crosshair"; 
		reload_btn.setAttribute('href', '#');
		reload_btn.onclick = function() { reload_stream(ch.name); };
		head.appendChild(reload_btn);

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

function reload_stream(name)
{
	remote_objs.del_streamer(name);
}

remote_objs = new RemoteObjs();
