var express = require('express');
var fs = require('fs');
var app = express();

// app.use(express.static(__dirname + '/public'));
app.use(express.static(__dirname + '/public'));

app.listen(process.env.PORT || 3000);
