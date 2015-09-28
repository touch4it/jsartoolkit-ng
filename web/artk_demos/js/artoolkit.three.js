/* THREE.js ARToolKit integration */

THREE.Matrix4.prototype.setFromArray = function(m) {
	return this.elements.set(m);
};

artoolkit.getUserMediaThreeScene = function(width, height, onSuccess, onError) {
	artoolkit.init('../../builds');
	if (!onError) {
		onError = function(err) {
			console.log("ERROR: artoolkit.getUserMediaThreeScene");
			console.log(err);
		};
	}
	var video = document.createElement('video');
	navigator.getUserMedia  = navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia || navigator.msGetUserMedia;
	var hdConstraints = {
		audio: false,
		video: {
			mandatory: {
				maxWidth: width,
				maxHeight: height
	    	}
	  	}
	};

	var completeInit = function() {
		artoolkit.setup(video.videoWidth, video.videoHeight);
		// artoolkit.debugSetup();

		var scenes = artoolkit.createThreeScene(video);
		onSuccess(scenes);
	};

	var initWaitCount = 2;
	var initProgress = function() {
		initWaitCount--;
		if (initWaitCount === 0) {
			completeInit();
		}
	};

	var success = function(stream) {
		video.addEventListener('loadedmetadata', initProgress, false);

		video.src = window.URL.createObjectURL(stream);
		video.play();

		artoolkit.onReady(initProgress);

	};

	if (navigator.getUserMedia) {
		navigator.getUserMedia(hdConstraints, success, onError);
	} else {
		onError('');
	}
};

artoolkit.createThreeScene = function(video) {
	// To display the video, first create a texture from it.
	var videoTex = new THREE.Texture(video);

	videoTex.minFilter = THREE.LinearFilter;
	videoTex.flipY = false;

	// Then create a plane textured with the video.
	var plane = new THREE.Mesh(
	  new THREE.PlaneGeometry(2, 2),
	  new THREE.MeshBasicMaterial({map: videoTex, side: THREE.DoubleSide})
	);

	// The video plane shouldn't care about the z-buffer.
	plane.material.depthTest = false;
	plane.material.depthWrite = false;

	// Create a camera and a scene for the video plane and
	// add the camera and the video plane to the scene.
	var videoCamera = new THREE.OrthographicCamera(-1, 1, -1, 1, -1, 1);
	var videoScene = new THREE.Scene();
	videoScene.add(plane);
	videoScene.add(videoCamera);

	var scene = new THREE.Scene();
	var camera = new THREE.PerspectiveCamera(45, 1, 1, 1000)
	scene.add(camera);

	camera.matrixAutoUpdate = false;

	return {
		scene: scene,
		videoScene: videoScene,
		camera: camera,
		videoCamera: videoCamera,

		video: video,

		process: function() {
			for (var i in artoolkit.markers) {
				artoolkit.markers[i].visible = false;
			}
			artoolkit.process(video);
			camera.projectionMatrix.setFromArray(artoolkit.getCameraMatrix());
		},

		renderOn: function(renderer) {
			videoTex.needsUpdate = true;

			var ac = renderer.autoClear;
			renderer.autoClear = false;
			renderer.clear();
			renderer.render(this.videoScene, this.videoCamera);
			renderer.render(this.scene, this.camera);
			renderer.autoClear = ac;
		}
	};
};

artoolkit.onGetMarker = function(marker) {
	var obj = this.markers[marker.id];
	if (obj) {
		obj.matrix.setFromArray(artoolkit.getTransformationMatrix());
		obj.visible = true;
	}
};

artoolkit.markers = {};

artoolkit.loadMarker = function(url, onSuccess, onError) {
	if (!onError) {
		onError = function(err) {
			console.log("ERROR: artoolkit.loadMarker");
			console.log(err);
		};
	}
	artoolkit.addMarker('/bin/Data/patt.hiro', onSuccess, onError);
}

artoolkit.createThreeMarker = function(marker) {
	var obj = new THREE.Object3D();
	obj.matrixAutoUpdate = false;
	this.markers[marker] = obj;
	return obj;
};
