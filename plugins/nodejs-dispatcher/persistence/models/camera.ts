///<reference path="../../node/node.d.ts" />

var mongoose = require('mongoose');

var cameraSchema = mongoose.Schema({
	host: String,
	port: Number,
	contentType: String,
	image: String
})

mongoose.model('Camera', cameraSchema);
