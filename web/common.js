(function() {
	'use strict'


	var ARController = function(width, height, camera) {
		var id;
		var w = width, h = height;

		if (typeof width !== 'number') {
			var image = width;
			camera = height;
			w = image.videoWidth || image.width;
			h = image.videoHeight || image.height;
			this.image = image;
		}

		this.canvas = document.createElement('canvas');
		this.canvas.width = w;
		this.canvas.height = h;
		this.ctx = this.canvas.getContext('2d');

		this.addEventListeners();

		this.id = artoolkit.setup(w, h, camera);
	};

	ARController.prototype.addEventListeners = function() {

		var self = this;

		artoolkit.addEventListener('markerNum', function(ev) {
			if (ev.target === self.id) {
				console.log('ARController '+self.id+' got marker num', ev.data);
			}
		});

		artoolkit.addEventListener('frameMalloc', function(ev) {
			if (ev.target === self.id || self.id === undefined) {

				console.log('ARController '+ev.target+' got frame malloc', ev.data);

				var params = ev.data;
				self.framepointer = params.framepointer;
				self.framesize = params.framesize;

				self.dataHeap = new Uint8Array(Module.HEAPU8.buffer, self.framepointer, self.framesize);

				self.camera_mat = new Float64Array(Module.HEAPU8.buffer, params.camera, 16);
				self.transform_mat = new Float64Array(Module.HEAPU8.buffer, params.modelView, 16);
			}
		});

		artoolkit.addEventListener('getMarker', function(ev) {
			if (ev.target === self.id) {
				console.log('ARController '+self.id+' got marker', ev.data, ev.index);
			}
		});

		artoolkit.addEventListener('getMultiMarker', function(ev) {
			if (ev.target === self.id) {
				console.log('getMultiMarker', ev.data);
			}
		});

		artoolkit.addEventListener('getMultiMarkerSub', function(ev) {
			if (ev.target === self.id) {
			}
		});

	};

	ARController.prototype.debugSetup = function() {
		artoolkit.debugSetup(this.id);
	};

	ARController.prototype.process = function(image) {
		if (!image) {
			image = this.image;
		}
		
		this.ctx.drawImage(image, 0, 0, this.canvas.width, this.canvas.height); // draw video

		var imageData = this.ctx.getImageData(0, 0, this.canvas.width, this.canvas.height);
		var data = imageData.data;

		if (this.dataHeap) {
			this.dataHeap.set( new Uint8Array(data.buffer) );

			artoolkit.process(this.id);

			this.debugDraw();
		}
	};

	ARController.prototype.debugSetup = function() {
		document.body.appendChild(this.canvas)
		this.bwpointer = artoolkit.setDebugMode(this.id, 1);
	};

	ARController.prototype.debugDraw = function() {
		var debugBuffer = new Uint8ClampedArray(Module.HEAPU8.buffer, this.bwpointer, this.framesize);
		var id = new ImageData(debugBuffer, this.canvas.width, this.canvas.height)
		this.ctx.putImageData(id, 0, 0)

		if (!marker) return;

		for (var i=0; i<detected_markers.length; i++) {
			this.debugMarker(detected_markers[i]);
		}
	};

	ARController.prototype.debugMarker = function(marker) {
		var vertex, pos;
		vertex = marker.vertex;
		var ctx = this.ctx;
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
	};



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
		init: init,
		onReady: onReady,
		setup: setup,
		process: process,

		listeners: {},

		addEventListener: function(name, callback) {
			if (!this.listeners[name]) {
				this.listeners[name] = [];
			}
			this.listeners[name].push(callback);
		},

		removeEventListener: function(name, callback) {
			if (this.listeners[name]) {
				var index = this.listeners[name].indexOf(callback);
				if (index > -1) {
					this.listeners[name].splice(index, 1);
				}
			}
		},

		dispatchEvent: function(event) {
			var listeners = this.listeners[event.name];
			if (listeners) {
				for (var i=0; i<listeners.length; i++) {
					listeners[i](event);
				}
			}
		},

		addMarker: addMarker,
		addMultiMarker: addMultiMarker,

		onFrameMalloc: onFrameMalloc,
		onMarkerNum: onMarkerNum,
		_onGetMarker: onGetMarker,

		onGetMultiMarker: onGetMultiMarker,
		onGetMultiMarkerSub: onGetMultiMarkerSub,

		setDebugMode: setDebugMode,

		getDetectedMarkers: function() {
			return detected_markers;
		},

		getCameraMatrix: function() {
			return camera_mat;
		},
		getTransformationMatrix: function() {
			return transform_mat;
		},

		registerMarker: function(marker) {
			return -1;
		}
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

	var arID;

	var FUNCTIONS = [
		// 'process',
		// 'teardown',
		// 'setDebugMode',

		'setProjectionNearPlane',
		'setProjectionFarPlane',

		'setThresholdMode',
		'setThreshold',
		'setLabelingMode',
		'setPatternDetectionMode',
		'setMatrixCodeType',
		'setImageProcMode',

		'setPattRatio',
	];

	var files_to_load = [
		// Array Tuples of [ajax path, fs path]
		// ['bin/Data/patt.hiro', '/patt.hiro'],
		// ['bin/Data/camera_para.dat', '/camera_para.dat'],
		// [path + '../bin/Data2/markers2.dat', '/Data2/markers.dat'],
		// [path + '../bin/DataNFT/pinball.fset3', '/Data2/pinball.fset3'],
		// [path + '../bin/DataNFT/pinball.iset', '/Data2/pinball.iset'],
		// [path + '../bin/DataNFT/pinball.fset', '/Data2/pinball.fset'],
	];

	var readyFunc;
	var camera_path;

	// Initalize base path for loading emscripten
	// Also loads path for camera info
	function init(p, camera) {
		path = p.slice(-1) === '/' ? p : p + '/';
		var script = document.createElement('script');
		script.src = path + EMSCRIPTEN_FILE;
		document.body.appendChild(script);

		if (camera) {
			camera_path = camera;
			files_to_load.push([camera_path, '/camera_para.dat']);
		}

		return this;
	}




	function runWhenLoaded() {
		FUNCTIONS.forEach(function(n) {
			artoolkit[n] = Module[n];
		})


		artoolkit.CONSTANTS = {};

		for (var m in Module) {
			if (m.match(/^AR/))
			artoolkit.CONSTANTS[m] = Module[m];
		}

		// FS
		// FS.mkdir('/DataNFT');

		ajaxDependencies(files_to_load, function() {
			if (readyFunc) readyFunc();
		});

		ready = true;
	}

	function onReady(ofunc) {
		readyFunc = ofunc;
		if (ready) runWhenLoaded();
		else Module.onRuntimeInitialized = runWhenLoaded;
	}

	function onFrameMalloc(id, params) {
		this.dispatchEvent({name: 'frameMalloc', target: id, data: params});

		// framepointer = params.framepointer;
		// framesize = params.framesize;

		// camera_mat = new Float64Array(Module.HEAPU8.buffer, params.camera, 16);
		// transform_mat = new Float64Array(Module.HEAPU8.buffer, params.modelView, 16);

		// console.log('onMalloc', params);
		// console.log('matrices', camera_mat, transform_mat);
	}

	function onMarkerNum(id, number) {
		this.dispatchEvent({name: 'markerNum', target: id, data: number});

		detected_markers = new Array(number);
		// console.log('Detected', number);
	}

	function onGetMarker(object, i, id) {
		this.dispatchEvent({name: 'getMarker', target: id, index: i, data: object});

		marker = object;
		detected_markers[i] = marker;

		if (artoolkit.onGetMarker) artoolkit.onGetMarker(arID, object, i);
		// console.log(marker.id, marker.idMatrix, marker.cf);
	}

	function onGetMultiMarker(id, object) {
		this.dispatchEvent({name: 'getMultiMarker', target: id, data: object});
	}

	function onGetMultiMarkerSub(id, multiId, subMarker, subMarkerId) {
		this.dispatchEvent({name: 'getMultiMarkerSub', target: id, data: {multiMarkerId: multiId, markerId: subMarkerId, marker: subMarker}});
	}

	function setup(_w, _h) {
		w = _w;
		h = _h;

		arID = _setup(w, h, camera_path ? 1 : 0);

		Module.setScale(arID, 1);
		Module.setWidth(arID, 1);
		artoolkit.setProjectionNearPlane(arID, 0.1)
		artoolkit.setProjectionFarPlane(arID, 1000);

		return arID;
	}

	var marker_count = 0;
	function addMarker(arId, url, callback) {
		var filename = '/marker_' + marker_count++;
		ajax(url, filename, function() {
			var id = Module._addMarker(arId, filename);
			if (callback) callback(id);
		});
	}

	var multi_marker_count = 0;
	function addMultiMarker(arId, url, callback) {
		var filename = '/multi_marker_' + multi_marker_count++;
		ajax(url, filename, function() {
			var markerID = Module._addMultiMarker(arId, filename);
			var markerNum = Module._getMultiMarkerNum(arId, markerID);
			if (callback) callback(markerID, markerNum);
		});
	}

	function setDebugMode(arId, mode) {
		return _setDebugMode(arId, mode);
	}


	// transfer image

	function process(arId) {
		_process(arId);
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
	window.ARController = ARController;

})();