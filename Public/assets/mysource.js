// we assume all participants have different nicknames. This can be enforced with a control on input afterwards.

$(document).ready(function() {
	
});

function start_source() 
{
	var create_form = document.getElementById('creation_form');
	source_name = document.getElementById('source_name').value;
	channel_name = source_name;
	local_nickname = source_name;
	var radios = document.getElementsByName('bitrate_radio');
	var i=0;
	while(i < radios.length)
	{
		if (radios[i].checked)
		{
			bitrate = Number(radios[i].value);
			source_name = source_name + "@" + (bitrate/1000).toString() + "K";
			document.getElementById('status').innerHTML = "Streaming channel";
			document.getElementById('screenname').innerHTML = source_name;
			i = radios.length;
		}
		i++;
	}

	if (source_name.split("@")[0] != "")
	{
		create_form.style.display = "none";
		document.getElementById('vids').style.display = "block";
		init_source();
	}
	else
		bootbox.alert("Invalid channel name");
}

function init_source()
{
	if (!init) {
	Janus.init({debug: "all", callback: function() {
		var janus_session = new Janus({
			server: server,
			success: function() {
				janus_session.attach(publisher_obj);
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
