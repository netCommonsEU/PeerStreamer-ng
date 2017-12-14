function response_channel(ch)
{
	// TODO: check if browser supports video tag and video/mp4 format
	set_state(ch.name);
	channel = ch.id;
	var p = document.getElementById("video-player");
	var videlem = document.createElement("video");
	var sourceMP4 = document.createElement("source");

	videlem.controls = true;
	videlem.autoplay = true;
	videlem.width = "720";
	videlem.height = "480";

	if (videlem.canPlayType("video/mp4")) {
		sourceMP4.type = "video/mp4";
		// sourceMP4.src = "channels/" + channel + ".mp4"
		sourceMP4.src = "channels/" + channel
		videlem.appendChild(sourceMP4);

		p.appendChild(videlem);
	}

	var d = document.getElementById("video-player-alt");
	d.innerHTML = "Alternatively, you can use an external player opening this temporary URI: <strong>http://" + location.host + "/" +  ch.sdpfile + "</strong>. The streaming will terminate if you close this web page.";

}

function set_state(s)
{
	var t = document.getElementById("player-title");
	t.innerHTML = s;
}

function request_channel(ch)
{
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			set_state(this.responseText);
			var ch = JSON.parse(this.responseText);
			response_channel(ch);
		}
		if (this.readyState == 4 && this.status != 200) {
			set_state("&lt;An error occurred with channel&gt;");
		}
	};
	var id = Math.random().toString(36).substr(2, 8);
	xhttp.open("POST", "/channels/" + id, true);
	xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	var params = "ipaddr=" + encodeURIComponent(ch.ipaddr) + "&port=" + encodeURIComponent(ch.port)
	xhttp.send(params);
}

function update_channels(chs)
{
	var list = document.getElementById("channel-list");
	while (list.firstChild) {
		list.removeChild(list.firstChild);
	}
	for (el in chs) {
		var node = document.createElement("LI");
		node.className="list-group-item btn btn-default";
		var textnode = document.createTextNode(chs[el].name);
		node.appendChild(textnode);
		list.appendChild(node);
		(function (ch) {
		node.onclick = function(e){request_channel(ch);};
		})(chs[el]);
	}
}

function refresh_channels()
{
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

function update_channel()
{
	if (channel.length > 0)
	{
		var xhttp = new XMLHttpRequest();
		xhttp.open("UPDATE", "/channels/" + channel, true);
		xhttp.send();
	}
}

refresh_channels();
var refresh_channels_id = setInterval(refresh_channels, 5000);
var update_channel_id = setInterval(update_channel, 3000);
var channel = "";
