// ==UserScript==
// @id           iitc-plugin-seznam
// @name         IITC map layer from seznam.cz
// @category     Map Tiles
// @namespace    https://github.com/MarekKnapek
// @description  IITC map layer from seznam.cz
// @author       Marek
// @version      0.4
// @downloadURL  https://github.com/MarekKnapek/stuff/raw/release/iitc/iitc-seznam-map.user.js
// @updateURL    https://github.com/MarekKnapek/stuff/raw/release/iitc/iitc-seznam-map.user.update.js
// @include      https://www.ingress.com/intel*
// @include      http://www.ingress.com/intel*
// @match        https://www.ingress.com/intel*
// @match        http://www.ingress.com/intel*
// @include      https://www.ingress.com/mission/*
// @include      http://www.ingress.com/mission/*
// @match        https://www.ingress.com/mission/*
// @match        http://www.ingress.com/mission/*
// @grant        none
// ==/UserScript==


//TODO:
// - zoom animations
// - overlays (bike, ski, ...)


function wrapper(plugin_info)
{
	console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " And kicking!");

	if(typeof window.plugin !== "function"){
		window.plugin = function(){};
	}
	window.plugin.map_seznam = function(){};


	window.plugin.map_seznam.seznam_api_loaded = function()
	{
		console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " API downloaded, starting loader.");
		Loader.async = true;
		Loader.load(null, {}, window.plugin.map_seznam.seznam_loader_done);
	};

	window.plugin.map_seznam.seznam_loader_done = function()
	{
		console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Loader finished.");

		window.plugin.map_seznam.waiting = false;

		window.plugin.map_seznam.layer_codes = [
			SMap.DEF_BASE          	,
			SMap.DEF_BIKE          	,
			SMap.DEF_GEOGRAPHY     	,
			SMap.DEF_HISTORIC      	,
			SMap.DEF_HYBRID        	,
			SMap.DEF_OBLIQUE       	,
			SMap.DEF_OBLIQUE_HYBRID	,
			SMap.DEF_OPHOTO        	,
			SMap.DEF_OPHOTO0203    	,
			SMap.DEF_OPHOTO0406    	,
			SMap.DEF_PANO          	,
			SMap.DEF_RELIEF        	,
			SMap.DEF_SMART_BASE    	,
			SMap.DEF_SMART_OPHOTO  	,
			SMap.DEF_SMART_SUMMER  	,
			SMap.DEF_SMART_TURIST  	,
			SMap.DEF_SMART_WINTER  	,
			SMap.DEF_SUMMER        	,
			SMap.DEF_TRAIL         	,
			SMap.DEF_TURIST        	,
			SMap.DEF_TURIST_WINTER
		];

		var len = window.plugin.map_seznam.layer_codes.length;
		for(i = 0; i !== len; ++i){
			window.plugin.map_seznam.layer_group[i].getLayers()[0].sz_code = window.plugin.map_seznam.layer_codes[i];
		}

		var queue = [];
		len = window.plugin.map_seznam.queue.length;
		for(i = 0; i !== len; ++i){
			queue.push(window.plugin.map_seznam.queue[i]);
		}
		window.plugin.map_seznam.queue.length = 0;
		len = queue.length;
		for(i = 0; i !== len; ++i){
			queue[i]();
		}
	};

	window.plugin.map_seznam.layer_names = [
		"base"          	,
		"bike"          	,
		"geography"     	,
		"historic"      	,
		"hybrid"        	,
		"oblique"       	,
		"oblique hybrid"	,
		"ophoto"        	,
		"ophoto0203"    	,
		"ophoto0406"    	,
		"pano"          	,
		"relief"        	,
		"smart base"    	,
		"smart ophoto"  	,
		"smart summer"  	,
		"smart turist"  	,
		"smart winter"  	,
		"summer"        	,
		"trail"         	,
		"turist"        	,
		"turist winter"
	];

	window.plugin.map_seznam.waiting = true;

	window.plugin.map_seznam.queue = [];

	window.plugin.map_seznam.seznam_layer = L.Class.extend({

		initialize: function(name)
		{
			this.sz_name = name;
			var opts = { maxZoom : 18 };
			L.Util.setOptions(this, opts);
		},

		onAdd: function(map)
		{
			if(window.plugin.map_seznam.waiting || window.plugin.map_seznam.queue.length !== 0){
				console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Adding layer " + this.sz_name + " to queue.");
				self = this;
				window.plugin.map_seznam.queue.push(function(){
					self.onAdd(map);
				});
				return;
			}

			console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Adding layer " + this.sz_name + ".");

			this.sz_map = map;
			var div_outer = document.createElement("div");
			var div_inner = document.createElement("div");
			this.sz_div_outer = div_outer;
			this.sz_div_inner = div_inner;
			div_outer.appendChild(div_inner);
			div_outer.id = "seznamouter";
			div_inner.id = "seznammap";
			map.getContainer().insertBefore(div_outer, map.getContainer().firstChild);
			var ct = map.getCenter();
			var zoom = map.getZoom();
			var center = SMap.Coords.fromWGS84(ct.lng, ct.lat);
			var smap = new SMap(div_inner, center, zoom);
			smap.addDefaultLayer(this.sz_code).enable();
			var sync = new SMap.Control.Sync({ bottomSpace : 0 });
			smap.addControl(sync);
			this.sz_smap = smap;
			map.on("move", this.sz_move, this);
			map.on("viewreset", this.sz_reset, this);
			this.sz_update();
		},

		onRemove: function(map)
		{
			if(window.plugin.map_seznam.waiting || window.plugin.map_seznam.queue.length !== 0){
				console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Removing layer " + this.sz_name + " to queue.");
				self = this;
				window.plugin.map_seznam.queue.push(function(){
					self.onRemove(map);
				});
				return;
			}

			console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Removing layer " + this.sz_name + ".");

			map.off("move", this.sz_move, this);
			map.off("viewreset", this.sz_reset, this);
			this.sz_div_outer.parentNode.removeChild(this.sz_div_outer);
			this.sz_div_outer = null;
			this.sz_div_inner = null;
			this.sz_map = null;
			this.sz_smap = null;
		},

		sz_move: function(e)
		{
			this.sz_update();
		},
		sz_reset: function(e)
		{
			this.sz_update();
		},
		sz_update: function()
		{
			map = this.sz_map;
			smap = this.sz_smap;
			div_outer = this.sz_div_outer;
			div_inner = this.sz_div_inner;

			var size = map.getSize();
			div_outer.style.width = size.x + "px";
			div_outer.style.height = size.y + "px";
			div_inner.style.width = size.x + "px";
			div_inner.style.height = size.y + "px";
			var center = map.getCenter();
			smap.setCenterZoom(SMap.Coords.fromWGS84(center.lng, center.lat), map.getZoom());
		}

	});


	window.plugin.map_seznam.setup = function()
	{
		console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Setup.");

		window.plugin.map_seznam.layer_group = [];
		var len = window.plugin.map_seznam.layer_names.length;
		for(i = 0; i !== len; ++i){
			var lg = new L.LayerGroup();
			lg.addLayer(new window.plugin.map_seznam.seznam_layer(window.plugin.map_seznam.layer_names[i]));
			layerChooser.addBaseLayer(lg, "seznam.cz " + window.plugin.map_seznam.layer_names[i]);
			window.plugin.map_seznam.layer_group.push(lg);
		}
	};


	console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Donwloading seznam.cz API.");
	var seznam_api = "https://api.mapy.cz/loader.js";
	load(seznam_api).thenRun(window.plugin.map_seznam.seznam_api_loaded);


	var setup = window.plugin.map_seznam.setup;
	setup.info = plugin_info;
	if(!window.bootPlugins){
			window.bootPlugins = [];
	}
	window.bootPlugins.push(setup);
	if(window.iitcLoaded){
			setup();
	}
}


console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " I'm alive!");

var script = document.createElement("script");
var info = {};
if(typeof GM_info !== 'undefined' && GM_info && GM_info.script){
	info.script = {
		version: GM_info.script.version,
		name: GM_info.script.name,
		description: GM_info.script.description
	};
}
script.appendChild(document.createTextNode("(" + wrapper + ")(" + JSON.stringify(info) + ");"));
(document.body || document.head || document.documentElement).appendChild(script);



