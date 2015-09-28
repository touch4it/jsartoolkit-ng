arw.registerLogCallback = function(callback) {
	arw.logCallback = callback;
};

arw.initialiseAR = function() {

};

arw.initialiseARWithOptions = function(pattSize, pattCountMax) {

};

arw.getARToolKitVersion = function() {
};

arw.getError = function() {
};

arw.startRunning = function(vconf, cparaName, nearPlane, farPlane) {
};

arw.startRunningB = function(vconf, cparaBuff, nearPlane, farPlane) {
};

arw.startRunningStereo = function(vconfL, cparaNameL, vconfR, cparaNameR, transL2RName, nearPlane, farPlane) {
};

arw.startRunningStereoB = function(vconfL, cparaBuffL, vconfR, cparaBuffR, transL2RBuff, nearPlane, farPlane) {
};

arw.isRunning = function() {
};

arw.stopRunning = function() {

};

arw.shutdownAR = function() {

};

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
