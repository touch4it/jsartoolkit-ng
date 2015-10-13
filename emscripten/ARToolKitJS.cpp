#include <stdio.h>
#include <AR/ar.h>
#include <AR/gsub_lite.h>
// #include <AR/gsub_es2.h>
#include <AR/arMulti.h>
#include <emscripten.h>
#include <string>
#include <vector>
#include <unordered_map>


struct simple_marker {
	int id;
	ARdouble transform[3][4];
	bool found;
};

struct multi_marker {
	int id;
	ARMultiMarkerInfoT *multiMarkerHandle;
	ARdouble transform[3][4];
	bool found;
};

struct arController {
	int id;

	ARParam param;
	ARParamLT *paramLT = NULL;

	ARUint8 *videoFrame = NULL;
	int videoFrameSize;

	ARHandle *arhandle = NULL;
	ARPattHandle *arPattHandle = NULL;
	ARMultiMarkerInfoT *arMultiMarkerHandle = NULL;
	AR3DHandle* ar3DHandle;

	ARdouble width = 40.0;
	ARdouble cameraViewScale = 1.0;
	ARdouble nearPlane = 0.0001;
	ARdouble farPlane = 1000.0;

	std::vector<simple_marker> pattern_markers;
	std::vector<multi_marker> multi_markers;
	std::unordered_map<int, simple_marker> barcode_markers;

	int patt_id = 0; // Running pattern marker id

};


std::vector<simple_marker> pattern_markers;
std::vector<multi_marker> multi_markers;
std::unordered_map<int, simple_marker> barcode_markers;

std::unordered_map<int, arController> arControllers;


// ============================================================================
//	Global variables
// ============================================================================

static ARdouble	transform[3][4];

static ARdouble cameraLens[16];
static ARdouble modelView[16];
static ARdouble matrix[16];

static char patt_name[]  = "/patt.hiro";
char cparam_name[] = "/camera_para.dat";

static int gARControllerID = 0;


extern "C" {

	void setScale(int id, ARdouble tmp) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);
		arc->cameraViewScale = tmp;
	}

	void setWidth(int id, ARdouble tmp) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);
		arc->width = tmp;
	}

	void setProjectionNearPlane(int id, const ARdouble projectionNearPlane) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);
		arc->nearPlane = projectionNearPlane;
	}

	void setProjectionFarPlane(int id, const ARdouble projectionFarPlane) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);
		arc->farPlane = projectionFarPlane;
	}

	void setPatternDetectionMode(int id, int mode) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);
		if (arSetPatternDetectionMode(arc->arhandle, mode) == 0) {
			printf("Pattern detection mode set to %d.", mode);
		}
	}

	void setPattRatio(int id, float ratio) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		if (ratio <= 0.0f || ratio >= 1.0f) return;
		ARdouble pattRatio = (ARdouble)ratio;
		if (arc->arhandle) {
			if (arSetPattRatio(arc->arhandle, pattRatio) == 0) {
				printf("Pattern ratio size set to %f.", pattRatio);
			}
		}
	}

	void setMatrixCodeType(int id, int type) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		AR_MATRIX_CODE_TYPE matrixType = (AR_MATRIX_CODE_TYPE)type;
		arSetMatrixCodeType(arc->arhandle, matrixType);
	}

	void setLabelingMode(int id, int mode) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		int labelingMode = mode;

		if (arSetLabelingMode(arc->arhandle, labelingMode) == 0) {
			printf("Labeling mode set to %d", labelingMode);
		}
	}

	int setup(int width, int height, int load_camera) {
		int id = gARControllerID++;
		arController *arc = &(arControllers[id]);

		arc->videoFrameSize = width * height * 4 * sizeof(ARUint8);
		arc->videoFrame = (ARUint8*) malloc(arc->videoFrameSize);

		if (load_camera) {
			if (arParamLoad(cparam_name, 1, &(arc->param)) < 0) {
				ARLOGe("setupCamera(): Error loading parameter file %s for camera.\n", cparam_name);
				return (FALSE);
			}

			if (arc->param.xsize != width || arc->param.ysize != height) {
				ARLOGw("*** Camera Parameter resized from %d, %d. ***\n", arc->param.xsize, arc->param.ysize);
				arParamChangeSize(&(arc->param), width, height, &(arc->param));
			}

			ARLOG("*** Camera Parameter ***\n");
			arParamDisp(&(arc->param));
		}

		if ((arc->paramLT = arParamLTCreate(&(arc->param), AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
			ARLOGe("setupCamera(): Error: arParamLTCreate.\n");
			return (FALSE);
		}

		printf("arParamLTCreated\n..%d, %d\n", (arc->paramLT->param).xsize, (arc->paramLT->param).ysize);

		// setup camera
		if ((arc->arhandle = arCreateHandle(arc->paramLT)) == NULL) {
			ARLOGe("setupCamera(): Error: arCreateHandle.\n");
			return (FALSE);
		}
		// AR_DEFAULT_PIXEL_FORMAT
		int set = arSetPixelFormat(arc->arhandle, AR_PIXEL_FORMAT_RGBA);

		printf("arCreateHandle done\n");

		arc->ar3DHandle = ar3DCreateHandle(&(arc->param));
		if (arc->ar3DHandle == NULL) {
			ARLOGe("Error creating 3D handle");
		}

		if (arc->arPattHandle != NULL) {
			ARLOGe("setup(): arPattCreateHandle already created.\n");
		} else if ((arc->arPattHandle = arPattCreateHandle()) == NULL) {
			ARLOGe("setup(): Error: arPattCreateHandle.\n");
		}

		arPattAttach(arc->arhandle, arc->arPattHandle);
		printf("pattern handler created.\n");

		printf("Allocated videoFrameSize %d\n", arc->videoFrameSize);

		EM_ASM_({
			artoolkit.onFrameMalloc($0, {
				framepointer: $1,
				framesize: $2,
				camera: $3,
				modelView: $4
			});
		},
			arc->id,
			arc->videoFrame,
			arc->videoFrameSize,
			cameraLens,
			modelView
		);

		return arc->id;
	}

	int teardown(int id) {
		if (arControllers.find(id) == arControllers.end()) { return -1; }
		arController *arc = &(arControllers[id]);

		if (arc->videoFrame) {
			free(arc->videoFrame);
			arc->videoFrame = NULL;
			arc->videoFrameSize = 0;
		}

		arPattDetach(arc->arhandle);
		arPattDeleteHandle(arc->arPattHandle);
		arDeleteHandle(arc->arhandle);

		arParamLTFree(&(arc->paramLT));

		// TODO free all patterns and such

		return 0;
	}

	static int loadMarker(const char *patt_name, int *patt_id, ARHandle *arhandle, ARPattHandle **pattHandle_p) {
		// Loading only 1 pattern in this example.
		if ((*patt_id = arPattLoad(*pattHandle_p, patt_name)) < 0) {
			ARLOGe("loadMarker(): Error loading pattern file %s.\n", patt_name);
			arPattDeleteHandle(*pattHandle_p);
			return (FALSE);
		}

		return (TRUE);
	}

	static int loadMultiMarker(const char *patt_name, ARHandle *arHandle, ARPattHandle **pattHandle_p, ARMultiMarkerInfoT **arMultiConfig) {
		if( (*arMultiConfig = arMultiReadConfigFile(patt_name, *pattHandle_p)) == NULL ) {
			ARLOGe("config data load error !!\n");
			arPattDeleteHandle(*pattHandle_p);
			return (FALSE);
		}
		if( (*arMultiConfig)->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE ) {
			arSetPatternDetectionMode( arHandle, AR_TEMPLATE_MATCHING_COLOR );
		} else if( (*arMultiConfig)->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_MATRIX ) {
			arSetPatternDetectionMode( arHandle, AR_MATRIX_CODE_DETECTION );
		} else { // AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX
			arSetPatternDetectionMode( arHandle, AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX );
		}

		return (TRUE);
	}


	int addMarker(int id, std::string patt_name) {
		if (arControllers.find(id) == arControllers.end()) { return -1; }
		arController *arc = &(arControllers[id]);

		// const char *patt_name
		// Load marker(s).
		if (!loadMarker(patt_name.c_str(), &(arc->patt_id), arc->arhandle, &(arc->arPattHandle))) {
			ARLOGe("main(): Unable to set up AR marker.\n");
			teardown(id);
			exit(-1);
		}

		arc->pattern_markers.push_back(simple_marker());
		arc->pattern_markers[arc->patt_id].id = arc->patt_id;
		arc->pattern_markers[arc->patt_id].found = false;


		return arc->patt_id;
	}

	int addMultiMarker(int id, std::string patt_name) {
		if (arControllers.find(id) == arControllers.end()) { return -1; }
		arController *arc = &(arControllers[id]);

		// const char *patt_name
		// Load marker(s).
		if (!loadMultiMarker(patt_name.c_str(), arc->arhandle, &(arc->arPattHandle), &(arc->arMultiMarkerHandle))) {
			ARLOGe("main(): Unable to set up AR multimarker.\n");
			teardown(id);
			exit(-1);
		}

		int multiMarker_id = 1000000000 - arc->multi_markers.size();
		multi_marker marker = multi_marker();
		marker.id = multiMarker_id;
		marker.found = false;
		marker.multiMarkerHandle = arc->arMultiMarkerHandle;

		arc->multi_markers.push_back(marker);

		return marker.id;
	}

	int getMultiMarkerNum(int id, int multiMarker_id) {
		if (arControllers.find(id) == arControllers.end()) { return -1; }
		arController *arc = &(arControllers[id]);

		int mId = -multiMarker_id + 1000000000;
		if (multi_markers.size() <= mId) {
			return -1;
		}
		return (arc->multi_markers[mId].multiMarkerHandle)->marker_num;
	}

	void setThreshold(int id, int threshold) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		if (threshold < 0 || threshold > 255) return;
		if (arSetLabelingThresh(arc->arhandle, threshold) == 0) {
			printf("Threshold set to %d", threshold);
		};
		// default 100
		// arSetLabelingThreshMode
		// AR_LABELING_THRESH_MODE_MANUAL, AR_LABELING_THRESH_MODE_AUTO_MEDIAN, AR_LABELING_THRESH_MODE_AUTO_OTSU, AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE
	}

	void setThresholdMode(int id, int mode) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		AR_LABELING_THRESH_MODE thresholdMode = (AR_LABELING_THRESH_MODE)mode;

		if (arSetLabelingThreshMode(arc->arhandle, thresholdMode) == 0) {
			printf("Threshold mode set to %d", (int)thresholdMode);
		}
	}


	ARUint8* setDebugMode(int id, int enable) {
		if (arControllers.find(id) == arControllers.end()) { return NULL; }
		arController *arc = &(arControllers[id]);

		arSetDebugMode(arc->arhandle, enable ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE);
		printf("Debug mode set to %s", enable ? "on." : "off.");

		return arc->arhandle->labelInfo.bwImage;
	}

	void setImageProcMode(int id, int mode) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		int imageProcMode = mode;
		if (arSetImageProcMode(arc->arhandle, mode) == 0) {
			printf("Image proc. mode set to %d.", imageProcMode);
		}
	}

	void transferMultiMarker(int id, int multiMarkerId) {
		EM_ASM_({
			artoolkit.onGetMultiMarker($0, $1);
		},
			id,
			multiMarkerId
		);
	}

	void transferMultiMarkerSub(int id, int multiMarkerId, int index, ARMultiEachMarkerInfoT *marker) {
		EM_ASM_({
			artoolkit.onGetMultiMarkerSub($0, $1, 
				{
					visible: $2,
					pattId: $3,
					pattType: $4,
					width: $5
				},
				$6
			);
		},
			id,
			multiMarkerId,
			marker->visible,
			marker->patt_id,
			marker->patt_type,
			marker->width,
			index
		);
	}

	void transferMarker(int id, ARMarkerInfo* markerInfo, int index) {
		// see /artoolkit5/doc/apiref/ar_h/index.html#//apple_ref/c/tdef/ARMarkerInfo

		EM_ASM_({
			var $a = arguments;
			var i = 24;
			artoolkit._onGetMarker({
				area: $0,
				id: $1,
				idPatt: $2,
				idMatrix: $3,
				dir: $4,
				dirPatt: $5,
				dirMatrix: $6,
				cf: $7,
				cfPatt: $8,
				cfMatrix: $9,
				pos: [$10, $11],
				line: [
					[$12, $13, $14],
					[$15, $16, $17],
					[$18, $19, $20],
					[$21, $22, $23]
				],
				vertex: [
					[$a[i++], $a[i++]],
					[$a[i++], $a[i++]],
					[$a[i++], $a[i++]],
					[$a[i++], $a[i++]]
				],
				// ARMarkerInfo2 *markerInfo2Ptr;
				// AR_MARKER_INFO_CUTOFF_PHASE cutoffPhase;
				errorCorrected: $a[i++]
				// globalID: $a[i++]
			}, $a[i++], $a[i++]);
		},
			markerInfo->area,
			markerInfo->id,
			markerInfo->idPatt,
			markerInfo->idMatrix,
			markerInfo->dir,
			markerInfo->dirPatt,
			markerInfo->dirMatrix,
			markerInfo->cf,
			markerInfo->cfPatt,
			markerInfo->cfMatrix,

			markerInfo->pos[0],
			markerInfo->pos[1],

			markerInfo->line[0][0],
			markerInfo->line[0][1],
			markerInfo->line[0][2],

			markerInfo->line[1][0],
			markerInfo->line[1][1],
			markerInfo->line[1][2],

			markerInfo->line[2][0],
			markerInfo->line[2][1],
			markerInfo->line[2][2],

			markerInfo->line[3][0],
			markerInfo->line[3][1],
			markerInfo->line[3][2],

			//

			markerInfo->vertex[0][0],
			markerInfo->vertex[0][1],

			markerInfo->vertex[1][0],
			markerInfo->vertex[1][1],

			markerInfo->vertex[2][0],
			markerInfo->vertex[2][1],

			markerInfo->vertex[3][0],
			markerInfo->vertex[3][1],

			//

			markerInfo->errorCorrected,
			index,
			id
			// markerInfo->globalID
		);
	}

	void matrixMul(ARdouble dst[3][4], ARdouble m[3][4], ARdouble n[3][4]) {
		int i, j;
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				dst[i][j] =
					m[i][0] * n[0][j] +
					m[i][1] * n[1][j] +
					m[i][2] * n[2][j];
			}
			dst[i][j] =
				m[i][0] * n[0][j] +
				m[i][1] * n[1][j] +
				m[i][2] * n[2][j] +
				m[i][3];
		}
	}

	void convertMatrixFormat( ARdouble para[3][4], ARdouble gl_para[16] ) {
		int     i, j;

		for( j = 0; j < 3; j++ ) {
			for( i = 0; i < 4; i++ ) {
				gl_para[i*4+j] = para[j][i];
			}
		}
		gl_para[0*4+3] = gl_para[1*4+3] = gl_para[2*4+3] = 0.0;
		gl_para[3*4+3] = 1.0;
	}

	void convert2(ARdouble origin[3][4], ARdouble convert[16]) {
		convert[ 0] = origin[0][0];
		convert[ 1] = origin[1][0];
		convert[ 2] = origin[2][0];
		convert[ 3] = 0.0;
		convert[ 4] = origin[0][1];
		convert[ 5] = origin[1][1];
		convert[ 6] = origin[2][1];
		convert[ 7] = 0.0;
		convert[ 8] = origin[0][2];
		convert[ 9] = origin[1][2];
		convert[10] = origin[2][2];
		convert[11] = 0.0;
		convert[12] = origin[0][3];
		convert[13] = origin[1][3];
		convert[14] = origin[2][3];
		convert[15] = 1.0;
	}

	void process(int id) {
		if (arControllers.find(id) == arControllers.end()) { return; }
		arController *arc = &(arControllers[id]);

		int success = arDetectMarker(
			arc->arhandle, arc->videoFrame
		);

		if (success) return;

		printf("arDetectMarker: %d\n", success);

		int markerNum = arGetMarkerNum(arc->arhandle);
		ARMarkerInfo* markerInfo = arGetMarker(arc->arhandle);

		EM_ASM_({
			artoolkit.onMarkerNum($0, $1);
		}, id, markerNum);

		int i, j, k;

		k = -1;

		ARMarkerInfo* marker;
		simple_marker* match;
		multi_marker* multiMatch;

		for (j = 0; j < (arc->arhandle)->marker_num; j++) {
			marker = &((arc->arhandle)->markerInfo[j]);

			// Pattern found
			if (marker->idPatt > -1 && marker->idMatrix == -1) {
				match = &(arc->pattern_markers[marker->idPatt]);

				if (!match->found) {
					arGetTransMatSquare(arc->ar3DHandle, marker, arc->width, match->transform);
				} else {
					arGetTransMatSquareCont(arc->ar3DHandle, marker, match->transform, arc->width, match->transform);
				}

				arglCameraViewRH(match->transform, modelView, arc->cameraViewScale);
			}
			// Barcode found
			else if (marker->idMatrix > -1) {
				if (arc->barcode_markers.find(marker->idMatrix) == arc->barcode_markers.end()) {
					arc->barcode_markers[marker->idMatrix] = simple_marker();

					match = &(arc->barcode_markers[marker->idMatrix]);
					match->found = true;
					match->id = marker->idMatrix;
					arGetTransMatSquare(arc->ar3DHandle, marker, arc->width, match->transform);
				}
				else {
					match = &(arc->barcode_markers[marker->idMatrix]);
					arGetTransMatSquareCont(arc->ar3DHandle, marker, match->transform, arc->width, match->transform);
				}

				arglCameraViewRH(match->transform, modelView, arc->cameraViewScale);
			}
			// everything else
			else {
				arGetTransMatSquare(arc->ar3DHandle, &((arc->arhandle)->markerInfo[j]), arc->width, transform);
				// places transform matrix to modelView
				arglCameraViewRH(transform, modelView, arc->cameraViewScale);
			}

			// send what we have down to JS land
			transferMarker(id, &((arc->arhandle)->markerInfo[j]), j);
		}

		arglCameraFrustumRH(&((arc->paramLT)->param), arc->nearPlane, arc->farPlane, cameraLens);

		// toggle transform found flag
		for (j = 0; j < arc->pattern_markers.size(); j++) {
			match = &(arc->pattern_markers[j]);
			match->found = false;
			for (k=0; k < arc->arhandle->marker_num; k++) {
				marker = &((arc->arhandle)->markerInfo[k]);
				if (marker->idPatt == match->id && marker->idMatrix == -1) {
					match->found = true;
					break;
				}
			}
		}

		for (auto &any : arc->barcode_markers) {
			match = &any.second;
			match->found = false;
			for (k=0; k < arc->arhandle->marker_num; k++) {
				marker = &((arc->arhandle)->markerInfo[k]);
				if (marker->idMatrix == -1) {
					match->found = true;
					break;
				}
			}

			//if (!match->found) barcode_markers.erase(marker->id);
		}

		for (j = 0; j < arc->multi_markers.size(); j++) {
			multiMatch = &(arc->multi_markers[j]);
			multiMatch->found = false;
			ARMultiMarkerInfoT *arMulti = multiMatch->multiMarkerHandle;

			int err = 0;
			int robustFlag = 1;

			if( robustFlag ) {
				err = arGetTransMatMultiSquareRobust( arc->ar3DHandle, markerInfo, markerNum, arMulti );
			} else {
				err = arGetTransMatMultiSquare( arc->ar3DHandle, markerInfo, markerNum, arMulti );
			}
			arglCameraViewRH(arMulti->trans, modelView, arc->cameraViewScale);
			transferMultiMarker(id, multiMatch->id);

			for (k = 0; k < arMulti->marker_num; k++) {
				matrixMul(transform, arMulti->trans, arMulti->marker[k].trans);
				arglCameraViewRH(transform, modelView, arc->cameraViewScale);
				transferMultiMarkerSub(id, multiMatch->id, k, &(arMulti->marker[k]));
			}
		}
	}
}

#include "ARBindEM.cpp"