
var SYS = require("sys");
var Buffer = require("buffer").Buffer;
var Transcoder = require("transcode").Transcoder;
var transcoder = new Transcoder("utf-8", "ascii");

var source = new Buffer(10);
for (var i = 0; i < 10; i++)
    source[i] = '-'.charCodeAt();
source[0] = 0xE2;
source[1] = 0x9D;
source[2] = 0xA4;
var target = new Buffer(10);
var state = {source: source, target: target};
var state2 = transcoder.transcode(state);
SYS.puts(state === state2);
SYS.puts(SYS.inspect(state));

