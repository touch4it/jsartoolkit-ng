/*

	Main usage pattern:

	initialize
	[startRunning] // because of the way HTML5 video source works, this is useless?
	[registerLogCallback] // Uh? Is this needed?
	[getError] // Is this needed?
	{
		[isRunning] // useless (see above)
		getProjectionMatrix -> camera projection matrix
		update(image) -> updates all the madness

		// Pattern detection config

		get / setVideoThreshold
		get / setVideoThresholdMode
		get / setLabelingMode
		get / setPatternDetectionMode
		get / setBorderSize
		get / setMatrixCodeType
		get / setImageProcMode
		get / setNFTMultiMode
		loadOpticalParams

		// Marker management

		addMarker(config) -> marker
		removeMarker(marker)
		removeAllMarkers()
		marker.visible -> bool
		marker.getTransformation(matrix)
		marker.patternCount
		marker.patternConfig
		marker.patternImage
		marker.set/getOption
	}
	[stopRunning] // because of the way HTML5 video source works, this is useless?
	shutdown

*/

// Basic API

arw.initialiseAR = Module.arwInitialiseAR;
arw.initialiseARWithOptions = Module.arwInitialiseARWithOptions;

arw.getARToolKitVersion = function() {
	return "4.5.1";
};


arw.startRunning = function(vconf, cparaName, nearPlane, farPlane) {
};

arw.startRunningB = function(vconf, cparaBuff, nearPlane, farPlane) {
};

arw.isRunning = Module.arwIsRunning();

arw.stopRunning = Module.arwStopRunning();

arw.shutdownAR = Module.arwShutdownAR();

arw.getProjectionMatrix = function(matrixFloatArray) {

};

arw.getProjectionMatrixStereo = function(matrixFloatArrayL, matrixFloatArrayR) {

};

arw.getVideoParams = function() {
	return {
		width: width,
		height: height,
		pixelSize: pixelSize,
		pixelFormat: pixelFormat
	};
};

arw.getVideoParamsStereo = function() {
	return {
		left: {
			width: widthL,
			height: heightL,
			pixelSize: pixelSizeL,
			pixelFormat: pixelFormatL
		},
		right: {
			width: widthR,
			height: heightR,
			pixelSize: pixelSizeR,
			pixelFormat: pixelFormatR
		}
	};
};

arw.capture = function() {

};

arw.updateAR = function() {

};

arw.updateTexture = function(buffer) {};
arw.updateTexture32 = function(buffer) {};
arw.updateTextureStereo = function(bufferL, bufferR) {};
arw.updateTexture32Stereo = function(bufferL, bufferR) {};

arw.setVideoDebugMode = function(debug) {

};

arw.getVideoDebugMode = function() {

};

arw.arwUpdateDebugTexture = function(buffer, alpha) {};

arw.updateTextureGL = function(textureID) {};
arw.updateTextureGLStereo = function(textureID_L, textureID_R) {};

arw.threeRenderEvent = function() {};
// arw.UnityRenderEvent (int eventID)
// arw.SetUnityRenderEventUpdateTextureGLTextureID (int textureID)
// arw.SetUnityRenderEventUpdateTextureGLStereoTextureIDs (int textureID_L, int textureID_R)
 
arw.setVideoThreshold = function(threshold) {

};

arw.getVideoThreshold = function() {

};

arw.setVideoThresholdMode = function(mode) {

};

arw.getVideoThresholdMode = function() {

};

arw.setLabelingMode = function(mode) {

};

arw.getLabelingMode = function() {

};

arw.setPatternDetectionMode = function(mode) {

};

arw.getPatternDetectionMode = function() {

};


arw.setBorderSize = function(size) {
};
 
arw.getBorderSize = function() {
};
 
arw.setMatrixCodeType = function(type) {
};
 
arw.getMatrixCodeType = function () {
};
 
arw.setImageProcMode = function (mode) {
};
 
arw.getImageProcMode = function () {
};
 
arw.setNFTMultiMode = function (on) {
};
 
arw.getNFTMultiMode = function () {
};
 
arw.addMarker = function (cfg) {
};
 
arw.removeMarker = function (markerUID) {
};
 
arw.removeAllMarkers = function () {
};
 
arw.queryMarkerVisibility = function (markerUID) {
};
 
arw.queryMarkerTransformation = function (markerUID, matrixFloatArray) {
};
 
arw.queryMarkerTransformationStereo = function (markerUID, matrixL, matrixR) {
};
 
arw.getMarkerPatternCount = function (markerUID) {
};
 
arw.getMarkerPatternConfig = function (markerUID, patternID, matrix) {
	// float *width, float *height, int *imageSizeX, int *imageSizeY
};
 
arw.getMarkerPatternImage = function (markerUID, patternID, buffer) {
};
 
arw.setMarkerOptionBool = function (markerUID, option, value) {
};
 
arw.setMarkerOptionInt = function (markerUID, option, value) {
};
 
arw.setMarkerOptionFloat = function (markerUID, option, value) {
};
 
arw.getMarkerOptionBool = function (markerUID, option) {
};
 
arw.getMarkerOptionInt = function (markerUID, option) {
};
 
arw.getMarkerOptionFloat = function (markerUID, option) {
};
 
arw.loadOpticalParams = function (optical_param_name) {
	// optical_param_buff, float *fovy_p, float *aspect_p, float m[16], float p[16]
};


// LOGGING

arw.registerLogCallback = function(callback) {
	arw.logCallback = callback;
};

arw.getError = function() {
};


// STEREO SUPPORT

arw.startRunningStereo = function(vconfL, cparaNameL, vconfR, cparaNameR, transL2RName, nearPlane, farPlane) {
};

arw.startRunningStereoB = function(vconfL, cparaBuffL, vconfR, cparaBuffR, transL2RBuff, nearPlane, farPlane) {
};



