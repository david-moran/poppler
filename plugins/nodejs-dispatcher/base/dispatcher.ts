///<reference path="../node/node.d.ts" />

import net = module('net');

var plugins = {};

export class Dispatcher {
    address: string;
    port: number;

    constructor(address: string, port: number) {
        this.address = address;
        this.port = port;
    }

    __socketHandler(socket) {
        socket.on('data', function(data) {
            var parts = data.toString().split(":");

            var type = parts[0];
            var host = parts[1];
            var port = parts[2];

            if (plugins.hasOwnProperty(type)) {
                plugins[type].check(host, port);
            } else {
                console.log('Plugin ' + type + ' Not registered');
            }
        });
    }

    dispatch() {
        net.createServer(this.__socketHandler).listen(
            this.port, this.address);
    }

    registerPlugin(pluginName: string) {
        plugins[pluginName] = require('../plugins/' + pluginName);
    }
}
