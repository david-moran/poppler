import dispatcher = require("./base/dispatcher");
import config = require("./config");
import mongo = require("./persistence/mongo");

import CameraPreview = require("./utils/CameraPreview");

mongo.start(function(db){
	var d = new dispatcher.Dispatcher('127.0.0.1', 2030);

	for (var i in config.plugins) {
		d.registerPlugin(config.plugins[i])
	}

	d.dispatch()

	new CameraPreview.CameraPreview("127.0.0.1", 1337)
});
