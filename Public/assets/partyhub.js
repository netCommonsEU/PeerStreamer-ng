// we assume all participants have different nicknames. This can be enforced with a control on input afterwards.

var server = null;
if(window.location.protocol === 'http:')
	server = "http://" + window.location.hostname + ":8088/janus";
else
	server = "https://" + window.location.hostname + ":8089/janus";

var local_nickname = "";
var room_name = "";

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
				janus_session.attach(publisher_obj)},
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
				Janus.log("Streaming ");
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
