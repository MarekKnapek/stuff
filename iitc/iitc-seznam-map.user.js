// ==UserScript==
// @id           iitc-plugin-seznam
// @name         IITC map layer from seznam.cz
// @category     Map Tiles
// @namespace    iitcseznamcz
// @description  IITC map layer from seznam.cz
// @author       Marek
// @version      0.2
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
// - page refresh doesn't remember selected map


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
		var layer_types =
			[
				["base"          	, SMap.DEF_BASE          	],
				["bike"          	, SMap.DEF_BIKE          	],
				["geography"     	, SMap.DEF_GEOGRAPHY     	],
				["historic"      	, SMap.DEF_HISTORIC      	],
				["hybrid"        	, SMap.DEF_HYBRID        	],
				["oblique"       	, SMap.DEF_OBLIQUE       	],
				["oblique hybrid"	, SMap.DEF_OBLIQUE_HYBRID	],
				["ophoto"        	, SMap.DEF_OPHOTO        	],
				["ophoto0203"    	, SMap.DEF_OPHOTO0203    	],
				["ophoto0406"    	, SMap.DEF_OPHOTO0406    	],
				["pano"          	, SMap.DEF_PANO          	],
				["relief"        	, SMap.DEF_RELIEF        	],
				["smart base"    	, SMap.DEF_SMART_BASE    	],
				["smart ophoto"  	, SMap.DEF_SMART_OPHOTO  	],
				["smart summer"  	, SMap.DEF_SMART_SUMMER  	],
				["smart turist"  	, SMap.DEF_SMART_TURIST  	],
				["smart winter"  	, SMap.DEF_SMART_WINTER  	],
				["summer"        	, SMap.DEF_SUMMER        	],
				["trail"         	, SMap.DEF_TRAIL         	],
				["turist"        	, SMap.DEF_TURIST        	],
				["turist winter" 	, SMap.DEF_TURIST_WINTER 	]
			];

		window.plugin.map_seznam.layer_group = [];
		var len = layer_types.length;
		for(i = 0; i != len; ++i){
			var lg = new L.LayerGroup();
			lg.addLayer(new window.plugin.map_seznam.seznam_layer(layer_types[i][1], layer_types[i][0]));
			layerChooser.addBaseLayer(lg, "seznam.cz " + layer_types[i][0]);
			window.plugin.map_seznam.layer_group.push(lg);
		}
		console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Loader finished.");
	};

	window.plugin.map_seznam.seznam_layer = L.Class.extend({

		initialize: function(code, name)
		{
			this.sz_code = code;
			this.sz_name = name;
			var opts = {maxZoom:16};
			L.Util.setOptions(this, opts);
		},

		onAdd: function(map)
		{
			console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Adding layer " + this.sz_name + ".");

			this.sz_map = map;
			var div_outer = document.createElement("div");
			var div_inner = document.createElement("div");
			div_outer.appendChild(div_inner);
			this.sz_div_outer = div_outer;
			this.sz_div_inner = div_inner;
			div_outer.id = "seznamouter";
			div_inner.id = "seznammap";
			//map.getPanes().tilePane.appendChild(div_outer);
			map.getContainer().insertBefore(div_outer, map.getContainer().firstChild);
			var size = map.getSize();
			//div_outer.style.position = "absolute";
			//div_outer.style.left = "0px";
			//div_outer.style.top = "0px";
			//div_outer.style.zIndex = "9999";
			//div_outer.classList.add("leaflet-layer");
			//div_inner.classList.add("leaflet-layer");
			//div_outer.classList.add("leaflet-tile-container");
			div_outer.style.width = size.x + "px";
			div_outer.style.height = size.y + "px";
			div_inner.style.width = size.x + "px";
			div_inner.style.height = size.y + "px";
			var ct = map.getCenter();
			var zoom = map.getZoom();
			var center = SMap.Coords.fromWGS84(ct.lng, ct.lat);
			var smap = new SMap(div_inner, center, zoom/*, this.sz_code/*, SMap.DEF_TURIST*/);
			smap.addDefaultLayer(this.sz_code/*SMap.DEF_BASE*/).enable();
			var sync = new SMap.Control.Sync({bottomSpace:30});
			smap.addControl(sync);
			this.sz_smap = smap;
			map.on("move", this.sz_move, this);
			map.on("viewreset", this.sz_reset, this);
		},

		onRemove: function(map)
		{
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
		console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Donwloading seznam.cz API.");
		var seznam_api = "https://api.mapy.cz/loader.js";
		load(seznam_api).thenRun(window.plugin.map_seznam.seznam_api_loaded);
	};


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



