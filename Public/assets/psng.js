function response_channel(ch)
{
	set_state(ch.name);
	channel = ch.id;
	var v = document.getElementById("vlc");
	v.playlist.playItem(vlc.playlist.add(ch.sdpfile));
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
		node.onclick = function(){request_channel(chs[el]);}
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
