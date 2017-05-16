function update_channels(chs)
{
	var list = document.getElementById("channel-list");
	while (list.firstChild) {
		list.removeChild(list.firstChild);
	}
	for (el in chs) {
		var node = document.createElement("LI");
		node.className="list-group-item";
		var textnode = document.createTextNode(chs[el].name);
		node.appendChild(textnode);   
		list.appendChild(node);   
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

refresh_channels();
var refresh_channels_id = setInterval(refresh_channels, 5000);
