var draconisFormat = {
	name = "Draconis Map format",
	extension = "lvl",

	read: function(fileName) {
		var file = new BinaryFile(fileName, BinaryFile.ReadOnly);
		var map = new TileMap();

		// read header bytes
		var buf = file.read(7);
		var view = new Uint8Array(buf);

		var width = 1 + view[0] + view[1] * 256;
		var height = 1 + view[2] + view[3] * 256;
		var l_num = view[4];
		var ts_num = view[5];
		var extraLength = view[6];
		
		// room for extra data here (usually unused)
		var extra = file.read(extraLength);

		tiled.log("Opening file '" + fileName + "', w=" + width + ", h=" + height + ", l=" + l_num + ", ts=" + ts_num + ", extra=" + extraLength);

		// create map object
		map.setSize(width, height);
		map.setTileSize(32, 32);
		map.orientation = TileMap.Orthogonal;

		// load tileset
		var tsPath = fileName.replace(/maps\/.*\.lvl$/i, 'tiles/' + ts_num + '/tileset.tsx');
		tiled.log("Using tileset " + tsPath);
		var tileset = tiled.open(tsPath);
		if(tileset && tileset.isTileset) {
			map.addTileset(tileset);

			// create each layer for the map
			var layers = Array();
			var edits = Array();
			for (var i = 0; i < l_num; i ++)
			{
				var layer = new TileLayer();
				layer.width = map.width;
				layer.height = map.height;
				layer.name = "Layer " + i;
				var layerEdit = layer.edit();

				layers[i] = layer;
				edits[i] = layerEdit;
			}

			// read all rest of object into buffer
			buf = file.read(map.height * map.width * l_num);
			view = new Uint8Array(buf);

			// plot every tile
			var ptr = 0;
			for (var y = 0; y < map.height; y ++) {
				for (var x = 0; x < map.width; x ++) {
					for (var i = 0; i < l_num; i ++) {
						if(view[ptr] > 0)
							edits[i].setTile(x, y, tileset.tile(view[ptr]));
						ptr ++;
					}
				}
			}

			// apply every layer
			for (var i = 0; i < l_num; i ++)
			{
				edits[i].apply();
				map.addLayer(layers[i]);
			}
		}

		// all done!
		file.close();
		return map;
	}
}

tiled.registerMapFormat("Draconis Map", draconisFormat);
