// we assume all participants have different nicknames. This can be enforced with a control on input afterwards.

$(document).ready(function() {
	var uri = new URL(window.location.href);
	var c = uri.searchParams.get("room");

	if (c)
		document.getElementById('room_name').value = c;
	
});

function fetch_room_channels()
{
	Janus.log("Fetching channels");
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			var channels = JSON.parse(this.responseText);
			update_room_channels(channels);
		}
	};
	xhttp.open("GET", "/channels", true);
	xhttp.send();
}

function update_room_channels(chs)
{
	var to_add = [];
	var to_remove = [];
	var stat;

	for (nch in chs) {
		if (id2roomname(chs[nch].name) == room_name) {
			stat = remote_objs.get_streamer_status(chs[nch].name);
			if (stat == "absent" && id2nickname(chs[nch].name) !== local_nickname) {
				to_add.push(chs[nch]);
			}
		}
	}
	for (och in channels) {
		if (id2roomname(channels[och].name) == room_name) {
			var found = false;
			for (nch in chs) {
				if (chs[nch].name == channels[och].name)
					found = true;
			}
			if (!found) {
				stat = remote_objs.get_streamer_status(channels[och].name);
				if (stat == "loaded") {
					to_remove.push(channels[och]);
				}
			}
		}
	}

	for (ch in to_add)
		remote_objs.add_streamer(to_add[ch].name, to_add[ch].ipaddr, to_add[ch].port);
	for (ch in to_remove)
		remote_objs.del_streamer(to_remove[ch].name);

	channels = chs;
}

function init_room()
{
	if (!init) {
	Janus.init({debug: "all", callback: function() {
		var janus_session = new Janus({
			server: server,
			success: function() {
				janus_session.attach(publisher_obj);
				fetch_room_channels();
				setInterval(fetch_room_channels, 3000);
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

function join_room() 
{
	var create_form = document.getElementById('creation_form');
	room_name = document.getElementById('room_name').value;
	local_nickname = document.getElementById('nickname').value;

	if (room_name != "" && local_nickname != "")
	{
		create_form.style.display = "none";
		document.getElementById('screenname').innerHTML = local_nickname;
		document.getElementById('roomname').innerHTML = "Room \"" + room_name + "\"";
		document.getElementById('vids').style.display = "block";
		channel_name = local_nickname+id_separator+room_name;
		init_room();
	}
	else
		bootbox.alert("Invalid room name or nickname", function() {});

}

