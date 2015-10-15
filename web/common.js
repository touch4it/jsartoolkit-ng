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

		this.id = artoolkit.setup(w, h, camera.id);

		this.setScale(1);
		this.setMarkerWidth(1);
		this.setProjectionNearPlane(0.1)
		this.setProjectionFarPlane(1000);

	};

	ARController.prototype.setScale = function(value) {
		return artoolkit.setScale(this.id, value);
	};

	ARController.prototype.getScale = function() {
		return artoolkit.getScale(this.id);
	};

	ARController.prototype.setMarkerWidth = function(value) {
		return artoolkit.setMarkerWidth(this.id, value);
	};

	ARController.prototype.getMarkerWidth = function() {
		return artoolkit.getMarkerWidth(this.id);
	};

	ARController.prototype.setProjectionNearPlane = function(value) {
		return artoolkit.setProjectionNearPlane(this.id, value);
	};

	ARController.prototype.getProjectionNearPlane = function() {
		return artoolkit.getProjectionNearPlane(this.id);
	};

	ARController.prototype.setProjectionFarPlane = function(value) {
		return artoolkit.setProjectionFarPlane(this.id, value);
	};

	ARController.prototype.getProjectionFarPlane = function() {
		return artoolkit.getProjectionFarPlane(this.id);
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


	var ARCameraParam = function() {
		this.id = -1;
		this._src = '';
		this.complete = false;
	};

	Object.defineProperty(ARCameraParam.prototype, 'src', {
		set: function(src) {
			if (src === this._src) {
				return;
			}
			this.dispose();
			this._src = src;
			if (src) {
				var self = this;
				artoolkit.loadCamera(src, function(id) {
					self.id = id;
					self.complete = true;
					self.onload();
				});
			}
		},
		get: function() {
			return this._src;
		}
	});

	ARCameraParam.prototype.dispose = function() {
		if (this.id !== -1) {
			artoolkit.deleteCamera(this.id);
		}
		this.id = -1;
		this._src = '';
		this.complete = false;
	};



	// ARToolKit JS API
	var artoolkit = {
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

		loadCamera: loadCamera,

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
		}
	};

	var detected_markers = [];

	var FUNCTIONS = [
		// 'process',
		// 'teardown',
		// 'setDebugMode',

		'setProjectionNearPlane',
		'setProjectionFarPlane',

		'setScale',
		'setMarkerWidth',

		'setThresholdMode',
		'setThreshold',
		'setLabelingMode',
		'setPatternDetectionMode',
		'setMatrixCodeType',
		'setImageProcMode',

		'setPattRatio',
	];

	function runWhenLoaded() {
		FUNCTIONS.forEach(function(n) {
			artoolkit[n] = Module[n];
		})

		artoolkit.CONSTANTS = {};

		for (var m in Module) {
			if (m.match(/^AR/))
			artoolkit.CONSTANTS[m] = Module[m];
		}
	}

	function onFrameMalloc(id, params) {
		this.dispatchEvent({name: 'frameMalloc', target: id, data: params});
	}

	function onMarkerNum(id, number) {
		this.dispatchEvent({name: 'markerNum', target: id, data: number});

		detected_markers = new Array(number);
		// console.log('Detected', number);
	}

	function onGetMarker(object, i, id) {
		this.dispatchEvent({name: 'getMarker', target: id, index: i, data: object});

		var marker = object;
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

	function setup(_w, _h, _camera_id) {
		return _setup(_w, _h, _camera_id);
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

	var camera_count = 0;
	function loadCamera(url, callback) {
		var filename = '/camera_param_' + camera_count++;
		ajax(url, filename, function() {
			var id = Module._loadCamera(filename);
			if (callback) callback(id);
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
	window.ARCameraParam = ARCameraParam;

	runWhenLoaded();

})();