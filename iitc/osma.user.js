// ==UserScript==
// @name       	osma
// @namespace  	https://github.com/MarekKnapek
// @version    	0.1
// @description	osma
// @author     	Marek Knápek
// @updateURL  	https://raw.githubusercontent.com/MarekKnapek/stuff/master/iitc/osma.meta.js
// @downloadURL	https://raw.githubusercontent.com/MarekKnapek/stuff/master/iitc/osma.user.js
// @include    	https://www.ingress.com/intel*
// @include    	http://www.ingress.com/intel*
// @match      	https://www.ingress.com/intel*
// @match      	http://www.ingress.com/intel*
// @include    	https://www.ingress.com/mission/*
// @include    	http://www.ingress.com/mission/*
// @match      	https://www.ingress.com/mission/*
// @match      	http://www.ingress.com/mission/*
// @grant      	none
// ==/UserScript==


console.log("iitc osma " + (new Date().toISOString()) + " I'm alive!");
function wrapper(plugin_info){
	console.log("iitc osma " + (new Date().toISOString()) + " Wrapper.");
	if(typeof window.plugin !== "function"){
		window.plugin = function(){};
	}
	window.plugin.osma = function(){};
	var osma = window.plugin.osma;
	osma.state = {};
	osma.funcs = function(){};
	osma.config = {};
	osma.config.post_url = "https://example.com/ingress.php";
	osma.config.post_param = "portal";
	osma.config.done_url = "https://example.com/ingress.php";
	osma.config.iterate_min = 5 * 1000;
	osma.config.iterate_max = 10 * 1000;
	osma.config.again_min = 3 * 1000 * 60 * 60;
	osma.config.again_max = 5 * 1000 * 60 * 60;
	osma.config.start_up = 5 * 1000 * 60;

	osma.funcs.toolbox_menu = function(){
		osma.funcs.begin();
	};

	osma.funcs.begin = function(){
		var bounds = map.getBounds();
		var portals = window.portals;
		var my_guids = [];
		var guids = Object.keys(portals);
		var len = guids.length;
		for(i = 0; i != len; ++i){
			var guid = guids[i];
			var portal = portals[guid];
			var pos = portal.getLatLng();
			if(bounds.contains(pos)){
				my_guids.push(guid);
			}
		}

		osma.state.curr = 0;
		osma.state.guids = my_guids;
		osma.funcs.iterate();
	};

	osma.funcs.iterate = function(){
		var curr = osma.state.curr;
		var guids = osma.state.guids;
		var len = guids.length;
		if(curr >= len){
			var xhttp = new XMLHttpRequest();
			xhttp.open("GET", osma.config.done_url, true);
			xhttp.send(null);
			window.setTimeout(function(){ osma.funcs.begin(); }, Math.floor(Math.random() * (osma.config.again_max - osma.config.again_min)) + osma.config.again_min);
			return;
		}
		var guid = guids[curr];
		var portals = window.portals;
		var portal = portals[guid];
		var detail = portalDetail.get(guid);
		if(!detail){
			portalDetail.request(guid);
			window.setTimeout(function(){ osma.funcs.iterate(); }, Math.floor(Math.random() * (osma.config.iterate_max - osma.config.iterate_min)) + osma.config.iterate_min);
			return;
		}

		var ret = {};
		ret["guid"] = guid;
		ret["detail"] = detail;
		var str = JSON.stringify(ret);

		console.log("iitc osma " + (new Date().toISOString()) + " portal: " + str);

		var xhttp = new XMLHttpRequest();
		xhttp.onreadystatechange = function(){
			if(xhttp.readyState == 4 && xhttp.status == 200){
				console.log("iitc osma " + (new Date().toISOString()) + " xhttp response: " + xhttp.responseText);
			}
		};
		xhttp.open("POST", osma.config.post_url, true);
		xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		xhttp.send(osma.config.post_param + "=" + str);

		osma.state.curr = curr + 1;
		window.setTimeout(function(){ osma.funcs.iterate(); }, 1);
	};

	osma.funcs.setup = function(){
		console.log("iitc osma " + (new Date().toISOString()) + " Setup.");
		$("#toolbox").append("<a onclick=\"window.plugin.osma.funcs.toolbox_menu();\">osma</a>");
		window.setTimeout(function(){ osma.funcs.begin(); }, osma.config.start_up);
	};

	var setup = osma.funcs.setup;
	setup.info = plugin_info;
	if(!window.bootPlugins){
		window.bootPlugins = [];
	}
	window.bootPlugins.push(setup);
	if(window.iitcLoaded){
		setup();
	}
}

var script = document.createElement("script");
var info = {};
if(typeof GM_info !== "undefined" && GM_info && GM_info.script){
	info.script = {
		version: GM_info.script.version,
		name: GM_info.script.name,
		description: GM_info.script.description
	};
}
script.appendChild(document.createTextNode("(" + wrapper + ")(" + JSON.stringify(info) + ");"));
(document.body || document.head || document.documentElement).appendChild(script);
