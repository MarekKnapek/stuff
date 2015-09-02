// ==UserScript==
// @id           iitc-plugin-seznam
// @name         IITC map layer from seznam.cz
// @category     Map Tiles
// @namespace    iitcseznamcz
// @description  IITC map layer from seznam.cz
// @author       Marek
// @version      0.1
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
	if(typeof window.plugin !== "function"){
		window.plugin = function(){};
	}
	window.plugin.map_seznam = function(){};


	window.plugin.map_seznam.setup = function()
	{
		var seznam_api_loaded = function()
		{
			var seznam_loader_done = function()
			{
				var layer_types =
				[
					["BASE", SMap.DEF_BASE],
					["BIKE", SMap.DEF_BIKE],
					["GEOGRAPHY", SMap.DEF_GEOGRAPHY],
					["HISTORIC", SMap.DEF_HISTORIC],
					["HYBRID", SMap.DEF_HYBRID],
					["OBLIQUE", SMap.DEF_OBLIQUE],
					["OBLIQUE_HYBRID", SMap.DEF_OBLIQUE_HYBRID],
					["OPHOTO", SMap.DEF_OPHOTO],
					["OPHOTO0203", SMap.DEF_OPHOTO0203],
					["OPHOTO0406", SMap.DEF_OPHOTO0406],
					["PANO", SMap.DEF_PANO],
					["RELIEF", SMap.DEF_RELIEF],
					["SMART_BASE", SMap.DEF_SMART_BASE],
					["SMART_OPHOTO", SMap.DEF_SMART_OPHOTO],
					["SMART_SUMMER", SMap.DEF_SMART_SUMMER],
					["SMART_TURIST", SMap.DEF_SMART_TURIST],
					["SMART_WINTER", SMap.DEF_SMART_WINTER],
					["SUMMER", SMap.DEF_SUMMER],
					["TRAIL", SMap.DEF_TRAIL],
					["TURIST", SMap.DEF_TURIST],
					["TURIST_WINTE", SMap.DEF_TURIST_WINTER]
				];

				var seznam_layer = L.Class.extend({
					initialize: function(code, name)
					{
						this.sz_code = code;
						this.sz_name = name;
					},
					onAdd: function(map)
					{
						//debugger;

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
						//debugger;

						console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Removing layer " + this.sz_name + ".");

						var map = this.sz_map;
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
						div_outer = this.sz_div_outer;
						div_inner = this.sz_div_inner;

						var size = map.getSize();
						div_outer.style.width = size.x + "px";
						div_outer.style.height = size.y + "px";
						div_inner.style.width = size.x + "px";
						div_inner.style.height = size.y + "px";
						this.sz_smap.setCenterZoom(SMap.Coords.fromWGS84(this.sz_map.getCenter().lng, this.sz_map.getCenter().lat), this.sz_map.getZoom());
					}
				});

				window.plugin.map_seznam.layer_group = [];
				var len = layer_types.length;
				for(i = 0; i != len; ++i){
					var lg = new L.LayerGroup();
					lg.addLayer(new seznam_layer(layer_types[i][1], layer_types[i][0]));
					layerChooser.addBaseLayer(lg, "seznam.cz " + layer_types[i][0]);
					window.plugin.map_seznam.layer_group.push(lg);
				}
				console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " seznam.cz loader finished.");
			};

			console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " seznam.cz API downloaded, starting loader.");
			Loader.async = true;
			Loader.load(null, {}, seznam_loader_done);
		};

		console.log("iitc seznam.cz maps plugin " + (new Date().toISOString()) + " Donwloading seznam.cz API.");
		var seznam_api = "https://api.mapy.cz/loader.js";
		load(seznam_api).thenRun(seznam_api_loaded);
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



