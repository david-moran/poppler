///<reference path="../node/node.d.ts" />

var http = require('http'),
	mongoose = require("mongoose");;

export class CameraPreview {

	constructor(ip: string, port: number) {
		this.__startHTTPService(ip,port);
	}

	__startHTTPService(ip: string, port: number){

		var self = this;

		http.createServer(function (req, res) {
			res.writeHead(200, {'Content-Type': 'text/html'});

			self.__getCameraDocs(function(cModels){
				res.end(self.__getHTMLResponse(self.__getHTMLCameraItems(cModels)));
			});

		}).listen(port, ip);

		console.log("Server running at http://" + ip +":" + port);
	}

	__getHTMLResponse(htmlContent : string) : string{
		return "<html><head><title>CamPreview</title></head><body>" + htmlContent + "</body></html>";
	}

	__getCameraDocs(callback){

		var cameraModel = mongoose.model("Camera");

		cameraModel.sort.('-id').limit(500).exec(function(e, cModels){
			callback(cModels);
		})
	}

	__getHTMLCameraItems(cModels) : string{
		var html = "";

        /* TODO: .reverse().slice() no parece la forma más óptima de hacer esto... */
		for(var i in cModels){
			var model = cModels[i];
			html = html +
				"<div style='float:left;width:350px'><a href='http://"+ model.host + ":" + model.port + "'>" +
				"<img style='width: 350px;' alt='' src='data:"+ model.contentType +";base64,"+ model.image + "'>" +
				"</a></div>";
		}

		return html;

	}

}


