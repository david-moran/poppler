///<reference path="../node/node.d.ts" />

import http = module("http");

var cameraModel = require("mongoose").model("Camera");

export function check(host: string, port: number) {
    var options = {
        host : host,
        port : port,
        path : '/snapshot.cgi?user=admin&pwd=&next_url=tempsnapshot.jpg&count=1'
    };

    var request = http.get(options, function(res) {

		res.setEncoding('binary');

        var contentType = res.headers["content-type"];

        if(contentType != null){
            if(contentType.indexOf("image") != -1 ){

				var content = "";

				res.on("data", function(data){
					content += data;
				});

				res.on("end", function(){

					var imageB64 = new Buffer(content, 'binary').toString('base64');

					var cameraM = new cameraModel({
						host: host,
						port: port,
						contentType: contentType,
						image: imageB64
					});

					cameraM.save();

					console.log("http://" + host + ":" + port);

				})

            }
        }
    });

    request.on('error', function(e){});

};
