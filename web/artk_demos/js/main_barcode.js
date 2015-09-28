/* THREE.js ARToolKit integration */

THREE.Matrix4.prototype.setFromArray = function(m) {
	return this.elements.set(m);
};

artoolkit.getUserMediaThreeScene = function(width, height, onSuccess, onError) {
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
	}
};

artoolkit.markers = {};

artoolkit.createThreeMarker = function(marker) {
	var id = this.registerMarker(marker);
	var obj = new THREE.Object3D();
	obj.matrixAutoUpdate = false;
	this.markers[id] = obj;
	return obj;
};






var findObjectUnderEvent = function(ev, renderer, camera, objects) {
	var mouse3D = new THREE.Vector3(
		( ev.layerX / renderer.domElement.width ) * 2 - 1,
		-( ev.layerY / renderer.domElement.height ) * 2 + 1,
		0.5
	);
	mouse3D.unproject( camera );
	mouse3D.sub( camera.position );
	mouse3D.normalize();
	var raycaster = new THREE.Raycaster( camera.position, mouse3D );
	var intersects = raycaster.intersectObjects( objects );
	if ( intersects.length > 0 ) {
		var obj = intersects[ 0 ].object
		return obj;
	}
};

var createBox = function() {
	// The AR scene.
	//
	// The box object is going to be placed on top of the marker in the video.
	// I'm adding it to the markerRoot object and when the markerRoot moves,
	// the box and its children move with it.
	//
	var box = new THREE.Object3D();
	var boxWall = new THREE.Mesh(
		new THREE.BoxGeometry(1, 1, 0.1, 1, 1, 1),
		new THREE.MeshLambertMaterial({color: 0xffffff})
	);
	boxWall.position.z = -0.5;
	box.add(boxWall);

	boxWall = boxWall.clone();
	boxWall.position.z = +0.5;
	box.add(boxWall);

	boxWall = boxWall.clone();
	boxWall.position.z = 0;
	boxWall.position.x = -0.5;
	boxWall.rotation.y = Math.PI/2;
	box.add(boxWall);

	boxWall = boxWall.clone();
	boxWall.position.x = +0.5;
	box.add(boxWall);

	boxWall = boxWall.clone();
	boxWall.position.x = 0;
	boxWall.position.y = -0.5;
	boxWall.rotation.y = 0;
	boxWall.rotation.x = Math.PI/2;
	box.add(boxWall);

	// Keep track of the box walls to test if the mouse clicks happen on top of them.
	var walls = box.children.slice();

	// Create a pivot for the lid of the box to make it rotate around its "hinge".
	var pivot = new THREE.Object3D();
	pivot.position.y = 0.5;
	pivot.position.x = 0.5;

	// The lid of the box is attached to the pivot and the pivot is attached to the box.
	boxWall = boxWall.clone();
	boxWall.position.y = 0;
	boxWall.position.x = -0.5;
	pivot.add(boxWall);
	box.add(pivot);

	walls.push(boxWall);

	box.position.z = 0.5;
	box.rotation.x = Math.PI/2;

	box.open = false;

	box.tick = function() {
		// Animate the box lid to open rotation or closed rotation, depending on the value of the open variable. 
		pivot.rotation.z += ((box.open ? -Math.PI/1.5 : 0) - pivot.rotation.z) * 0.1;
	};

	return {box: box, walls: walls};
};


(function() {
	var tw = 1280 / 2;
	var th = 720 / 2;

	var initThreeJS = function(arScene) {
		var renderer = new THREE.WebGLRenderer({antialias: true});
		renderer.setSize(arScene.video.videoWidth, arScene.video.videoHeight);
		document.body.appendChild(renderer.domElement);

		// Create a couple of lights for our AR scene.
		var light = new THREE.PointLight(0xffffff);
		light.position.set(40, 40, 40);
		arScene.scene.add(light);

		var light = new THREE.PointLight(0xff8800);
		light.position.set(-40, -20, -30);
		arScene.scene.add(light);


		// Create an object that tracks the marker transform.
		var marker = 'patt.hiro';
		var markerRoot = artoolkit.createThreeMarker(marker);
		arScene.scene.add(markerRoot);

		// Create the openable box object for our AR scene.
		var boxAndWalls = createBox();

		// Add the box to the markerRoot object to make it track the marker.
		markerRoot.add(boxAndWalls.box);

		var open = false;
		renderer.domElement.onclick = function(ev) {
			if (findObjectUnderEvent(ev, renderer, arScene.camera, boxAndWalls.walls)) {
				boxAndWalls.box.open = !boxAndWalls.box.open;
			}
		};

		var tick = function() {
			requestAnimationFrame(tick);
			arScene.process();

			boxAndWalls.box.tick();
			arScene.renderOn(renderer);
		};
		tick();
	};


	artoolkit.init('../../builds');
	artoolkit.getUserMediaThreeScene(tw, th, initThreeJS);


})();
