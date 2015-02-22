var http = require('http');

http.createServer(function (request, response) {
    response.writeHead(200, {
        'Content-Type': 'text/plain',
        'Access-Control-Allow-Origin' : '*'
    });
    response.end('Hello World\n');
}).listen(1337);

app.use("/", express.static(__dirname));

var express = require('express');
var app = express();
app.use('/', express.static(__dirname + '/views'));
app.listen(1337, function() { console.log('listening')});