(function() {
	'use strict'

	var path = '';

	var EMSCRIPTEN_FILE = 'artoolkit.js';
	var EMSCRIPTEN_MEM_FILE = 'artoolkit.js.mem';

	// Emscripten Module
	var Module = {
		onRuntimeInitialized: function() {
			ready = true;
		},
		locateFile: function(mem_file) {
			return path + EMSCRIPTEN_MEM_FILE;
		}
	};

	// ARToolKit JS API
	var artoolkit = {
		init: function(p) {
			path = p.slice(-1) === '/' ? p : p + '/';
			var script = document.createElement('script');
			script.src = path + EMSCRIPTEN_FILE;
			document.body.appendChild(script);

			return this;
		},
		onReady: onReady,
		setup: arSetup,
		onFrameMalloc: onFrameMalloc,
		onGetMarker: onGetMarker,
		onMarkerNum: onMarkerNum,
		debugSetup: debugSetup,
		process: process,
		getCameraMatrix: function() {
			return camera_mat;
		},
		getTransformationMatrix: function() {
			return transform_mat;
		},
	};

	var framepointer = 0, framesize = 0;
	var marker;

	var camera_mat;
	var transform_mat;
	 // = new Float32Array(16);
	var detected_markers = [];

	var ready = false;
	var w = 320, h = 240;
	var canvas, ctx, image;

	function onReady(ofunc) {
		var func = function() {
			// FS.mkdir('/Data2');
			// FS.mkdir('/DataNFT');
			var files = [
				// [path + '../bin/Data2/markers2.dat', '/Data2/markers.dat'],
				// [path + '../bin/DataNFT/pinball.fset3', '/Data2/pinball.fset3'],
				// [path + '../bin/DataNFT/pinball.iset', '/Data2/pinball.iset'],
				// [path + '../bin/DataNFT/pinball.fset', '/Data2/pinball.fset'],
			];
			ajaxDependencies(files, ofunc);
			ready = true;
		}
		if (ready) func();
		else Module.onRuntimeInitialized = func;
	}

	function onFrameMalloc(params) {
		framepointer = params.framepointer;
		framesize = params.framesize;

		camera_mat = new Float64Array(Module.HEAPU8.buffer, params.camera, 16);
		transform_mat = new Float64Array(Module.HEAPU8.buffer, params.modelView, 16);

		console.log('onMalloc', params);
		console.log('camera_mat', camera_mat);
	}

	function onMarkerNum(number) {
		detected_markers = new Array(number);
		// console.log('Detected', number);
	}

	function onGetMarker(object, i) {
		marker = object;
		detected_markers[i] = marker;
		// console.log(marker.id, marker.idMatrix, marker.cf);
	}

	function arSetup(_w, _h) {
		w = _w;
		h = _h;

		Module.setup(w, h);

		// setup canvas
		canvas = document.createElement('canvas');
		canvas.width = w;
		canvas.height = h;
		ctx = canvas.getContext('2d')

		// console.log('setup marker');
		// _startSetupMarker();

		_setThreshold(50);
	}

	var bwpointer;

	function debugSetup() {
		document.body.appendChild(canvas)
		bwpointer = _setDebugMode(1);
	}

	function debugDraw() {
		var yoz = new Uint8ClampedArray(Module.HEAPU8.buffer, bwpointer, framesize);
		var id = new ImageData(yoz, w, h)
		ctx.putImageData(id, 0, 0)

		if (!marker) return;

		detected_markers.forEach(debugMarker);
	}

	function debugMarker(marker) {
		var vertex, pos;
		vertex = marker.vertex;
		ctx.strokeStyle = 'red';

		ctx.beginPath()
		ctx.moveTo(vertex[0][0], vertex[0][1])
		ctx.lineTo(vertex[1][0], vertex[1][1])
		ctx.stroke();

		ctx.beginPath()
		ctx.moveTo(vertex[2][0], vertex[2][1])
		ctx.lineTo(vertex[3][0], vertex[3][1])
		ctx.stroke()

		ctx.strokeStyle = 'green';
		ctx.beginPath()
		ctx.lineTo(vertex[1][0], vertex[1][1])
		ctx.lineTo(vertex[2][0], vertex[2][1])
		ctx.stroke();

		ctx.beginPath()
		ctx.moveTo(vertex[3][0], vertex[3][1])
		ctx.lineTo(vertex[0][0], vertex[0][1])
		ctx.stroke();

		pos = marker.pos
		ctx.beginPath()
		ctx.arc(pos[0], pos[1], 8, 0, Math.PI * 2)
		ctx.fillStyle = 'red'
		ctx.fill()
	}

	// transfer image

	function process(target) {
		// console.time('draw');
		ctx.drawImage(target, 0, 0, w, h); // draw video
		// console.timeEnd('draw');

		// console.time('getImage');
		image = ctx.getImageData(0, 0, w, h);
		data = image.data;
		// console.time('getImage');

		// console.time('transferImage');
		var dataHeap = new Uint8Array(Module.HEAPU8.buffer, framepointer, framesize);
		dataHeap.set( new Uint8Array(data.buffer) );
		console.timeEnd('transferImage');

		// console.time('process')
		_process();
		// console.timeEnd('process')

		debugDraw();
	}

	// Eg.
	//	ajax('../bin/Data2/markers.dat', '/Data2/markers.dat', callback);
	//	ajax('../bin/Data/patt.hiro', '/patt.hiro', callback);

	function ajax(url, target, callback) {
		var oReq = new XMLHttpRequest();
		oReq.open('GET', url, true);
		oReq.responseType = 'arraybuffer'; // blob arraybuffer

		oReq.onload = function(oEvent) {
			console.log('ajax done for ', url);
			var arrayBuffer = oReq.response;
			var byteArray = new Uint8Array(arrayBuffer);
			FS.writeFile(target, byteArray, { encoding: 'binary' });
			console.log('FS written', target);

			callback();
		};

		oReq.send();
	}

	function ajaxDependencies(files, callback) {
		var next = files.pop();
		if (next) {
			ajax(next[0], next[1], function() {
				ajaxDependencies(files, callback);
			});
		} else {
			callback();
		}
	}

	/* Exports */

	window.Module = Module;
	window.artoolkit = artoolkit;

})();