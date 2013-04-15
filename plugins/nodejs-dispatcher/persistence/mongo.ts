///<reference path="../node/node.d.ts" />

var mongoose = require('mongoose'),
	fs = require('fs');


var modelsPath = __dirname + '/models/';

export function start(onDone){
	mongoose.connect('mongodb://localhost/poppler')

	var db = mongoose.connection;

	db.on('error', console.error.bind(console, 'connection error:'));

	db.once('open', function callback () {
		var modelsFiles = fs.readdirSync(modelsPath);

		modelsFiles.forEach(function(file){
			if(file.lastIndexOf('.js') != -1){
				require(modelsPath + file);
			}
		});

		if(onDone != null){
			onDone(db)
		}else{
			return db;
		}
	});
}

