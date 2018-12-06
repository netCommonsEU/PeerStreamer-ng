$(document).ready(function() {
	fetch_channels();
	setInterval(fetch_channels, 5000);
});

function init_player()
{
	if (!init) {
	Janus.init({debug: "all", callback: function() {
		var janus_session = new Janus({
			server: server,
			success: function() {
				init = true;
			},
			error: generic_error,
			destroyed: function () {}
		});
	}
	}
	);
	}
}

function update_channels(chs)
{
	var list = document.getElementById("channel-list");
	while (list.firstChild) {
		list.removeChild(list.firstChild);
	}
	for (el in chs) {
		if (chs[el].name.split(id_separator).length == 1) { // filtering the partyhub streamings
			var node = document.createElement("LI");
			node.className="list-group-item btn btn-default";
			var textnode = document.createTextNode(chs[el].name);
			node.appendChild(textnode);
			list.appendChild(node);
			(function (ch) {
			node.onclick = function(e){init_player(); remote_objs.del_streamer(channel_name); remote_objs.add_streamer(ch.name, ch.ipaddr, ch.port); channel_name=ch.name;};
		
			})(chs[el]);
		}
	}
}

function fetch_channels()
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


