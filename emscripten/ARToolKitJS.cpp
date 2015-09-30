#include <stdio.h>
#include <AR/ar.h>
#include <AR/gsub_lite.h>
// #include <AR/gsub_es2.h>
#include <AR/arMulti.h>
#include <emscripten.h>
#include <string>
#include <vector>
#include <unordered_map>

// ============================================================================
//	Global variables
// ============================================================================

static ARParam param;
static ARParamLT *paramLT = NULL;
static ARHandle *arhandle = NULL;
static ARUint8 *gVideoFrame = NULL;
static int gVideoFrameSize;

static AR3DHandle* ar3DHandle;
static ARdouble	transform[3][4];
static int transformContinue = 0;

static ARdouble width = 40.0;
static ARdouble CAMERA_VIEW_SCALE = 1.0;
static ARdouble NEAR_PLANE = 0.0001;    ///< Near plane d	istance for projection matrix calculation
static ARdouble FAR_PLANE = 1000.0;   ///< Far plane distance for projection matrix calculation

static ARdouble cameraLens[16];
static ARdouble modelView[16];

static char patt_name[]  = "/patt.hiro";
static int gPatt_id; // Running pattern marker id


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

std::vector<simple_marker> pattern_markers;
std::vector<multi_marker> multi_markers;
std::unordered_map<int, simple_marker> barcode_markers;

static ARPattHandle	*gARPattHandle = NULL;
static ARMultiMarkerInfoT *gARMultiMarkerHandle = NULL;

int patternDetectionMode = 0;
AR_MATRIX_CODE_TYPE matrixType = AR_MATRIX_CODE_3x3;

char cparam_name[] = "/camera_para.dat";

extern "C" {
	void setScale(ARdouble tmp) {
		CAMERA_VIEW_SCALE = tmp;
	}

	void setWidth(ARdouble tmp) {
		width = tmp;
	}

	void setProjectionNearPlane(const ARdouble projectionNearPlane) {
		NEAR_PLANE = projectionNearPlane;
	}

	void setProjectionFarPlane(const ARdouble projectionFarPlane) {
		FAR_PLANE = projectionFarPlane;
	}

	void setPatternDetectionMode(int mode) {
		patternDetectionMode = mode;
		if (arSetPatternDetectionMode(arhandle, patternDetectionMode) == 0) {
			printf("Pattern detection mode set to %d.", patternDetectionMode);
		}
	}

	void setPattRatio(float ratio) {
		if (ratio <= 0.0f || ratio >= 1.0f) return;
		ARdouble pattRatio = (ARdouble)ratio;
		if (arhandle) {
			if (arSetPattRatio(arhandle, pattRatio) == 0) {
				printf("Pattern ratio size set to %f.", pattRatio);
			}
		}
	}

	void setMatrixCodeType(int type) {
		matrixType = (AR_MATRIX_CODE_TYPE)type;
		arSetMatrixCodeType(arhandle, matrixType);
	}

	void setLabelingMode(int mode) {
		int labelingMode = mode;

		if (arSetLabelingMode(arhandle, labelingMode) == 0) {
			printf("Labeling mode set to %d", labelingMode);
		}
	}

	int setup(int width, int height, int load_camera) {
		gVideoFrameSize = width * height * 4 * sizeof(ARUint8);
		gVideoFrame = (ARUint8*) malloc(gVideoFrameSize);

		if (load_camera) {
			if (arParamLoad(cparam_name, 1, &param) < 0) {
				ARLOGe("setupCamera(): Error loading parameter file %s for camera.\n", cparam_name);
				return (FALSE);
			}

			if (param.xsize != width || param.ysize != height) {
				ARLOGw("*** Camera Parameter resized from %d, %d. ***\n", param.xsize, param.ysize);
				arParamChangeSize(&param, width, height, &param);
			}

			ARLOG("*** Camera Parameter ***\n");
			arParamDisp(&param);
		}

		if ((paramLT = arParamLTCreate(&param, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
			ARLOGe("setupCamera(): Error: arParamLTCreate.\n");
			return (FALSE);
		}

		printf("arParamLTCreated\n..%d, %d\n", (paramLT->param).xsize, (paramLT->param).ysize);

		// setup camera
		if ((arhandle = arCreateHandle(paramLT)) == NULL) {
			ARLOGe("setupCamera(): Error: arCreateHandle.\n");
			return (FALSE);
		}
		// AR_DEFAULT_PIXEL_FORMAT
		int set = arSetPixelFormat(arhandle, AR_PIXEL_FORMAT_RGBA);

		printf("arCreateHandle done\n");

		ar3DHandle = ar3DCreateHandle(&param);
		if (ar3DHandle == NULL) {
			ARLOGe("Error creating 3D handle");
		}

		if (gARPattHandle != NULL) {
			ARLOGe("setup(): arPattCreateHandle already created.\n");
		} else if ((gARPattHandle = arPattCreateHandle()) == NULL) {
			ARLOGe("setup(): Error: arPattCreateHandle.\n");
		}

		arPattAttach(arhandle, gARPattHandle);
		printf("pattern handler created.\n");

		printf("Allocated gVideoFrameSize %d\n", gVideoFrameSize);

		EM_ASM_({
			artoolkit.onFrameMalloc({
				framepointer: $0,
				framesize: $1,
				camera: $2,
				modelView: $3
			});
		}, gVideoFrame, gVideoFrameSize,
			cameraLens,
			modelView
		);

		return 0;
	}

	int teardown() {
		if (gVideoFrame) {
			free(gVideoFrame);
			gVideoFrame = NULL;
			gVideoFrameSize = 0;
		}

		arPattDetach(arhandle);
		arPattDeleteHandle(gARPattHandle);
		arDeleteHandle(arhandle);

		arParamLTFree(&paramLT);

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


	int addMarker(std::string patt_name) {
		// const char *patt_name
		// Load marker(s).
		if (!loadMarker(patt_name.c_str(), &gPatt_id, arhandle, &gARPattHandle)) {
			ARLOGe("main(): Unable to set up AR marker.\n");
			teardown();
			exit(-1);
		}

		pattern_markers.push_back(simple_marker());
		pattern_markers[gPatt_id].id = gPatt_id;
		pattern_markers[gPatt_id].found = false;


		return gPatt_id;
	}

	int addMultiMarker(std::string patt_name) {
		// const char *patt_name
		// Load marker(s).
		if (!loadMultiMarker(patt_name.c_str(), arhandle, &gARPattHandle, &gARMultiMarkerHandle)) {
			ARLOGe("main(): Unable to set up AR multimarker.\n");
			teardown();
			exit(-1);
		}

		int gMultiMarker_id = 1000000000 - multi_markers.size();
		multi_marker marker = multi_marker();
		marker.id = gMultiMarker_id;
		marker.found = false;
		marker.multiMarkerHandle = gARMultiMarkerHandle;

		multi_markers.push_back(marker);
		return gMultiMarker_id;
	}

	void setThreshold(int threshold) {
		if (threshold < 0 || threshold > 255) return;
		if (arSetLabelingThresh(arhandle, threshold) == 0) {
			printf("Threshold set to %d", threshold);
		};
		// default 100
		// arSetLabelingThreshMode
		// AR_LABELING_THRESH_MODE_MANUAL, AR_LABELING_THRESH_MODE_AUTO_MEDIAN, AR_LABELING_THRESH_MODE_AUTO_OTSU, AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE
	}

	void setThresholdMode(int mode) {
		AR_LABELING_THRESH_MODE thresholdMode = (AR_LABELING_THRESH_MODE)mode;

		if (arSetLabelingThreshMode(arhandle, thresholdMode) == 0) {
			printf("Threshold mode set to %d", (int)thresholdMode);
		}
	}


	ARUint8* setDebugMode(int enable) {
		arSetDebugMode(arhandle, enable ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE);
		printf("Debug mode set to %s", enable ? "on." : "off.");

		return arhandle->labelInfo.bwImage;
	}

	void setImageProcMode(int mode) {
		int imageProcMode = mode;
		if (arSetImageProcMode(arhandle, mode) == 0) {
			printf("Image proc. mode set to %d.", imageProcMode);
		}
	}

	void transferMultiMarker(int multiMarkerId) { //, int index, ARMultiEachMarkerInfoT *marker) {
		EM_ASM_({
			artoolkit.onGetMultiMarker($0
			// , 
			// {
			// 	visible: $2,
			// 	pattId: $3,
			// 	pattType: $4,
			// 	width: $5
			// },
			// $1
			);
		},
			multiMarkerId
			// ,
			// index,
			// marker->visible,
			// marker->patt_id,
			// marker->patt_type,
			// marker->width
		);
	}

	void transferMarker(ARMarkerInfo* markerInfo, int index) {
		// see /artoolkit5/doc/apiref/ar_h/index.html#//apple_ref/c/tdef/ARMarkerInfo

		EM_ASM_({
			var $a = arguments;
			var i = 24;
			artoolkit.onGetMarker({
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
			}, $a[i++]);
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
			index
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

	void process() {

		int success = arDetectMarker(
			arhandle, gVideoFrame
		);

		if (success) return;

		// printf("arDetectMarker: %d\n", success);

		int markerNum = arGetMarkerNum(arhandle);
		ARMarkerInfo* markerInfo = arGetMarker(arhandle);

		EM_ASM_({
			artoolkit.onMarkerNum($0);
		}, markerNum);

		int i, j, k;

		k = -1;

		ARMarkerInfo* marker;
		simple_marker* match;
		multi_marker* multiMatch;

		for (j = 0; j < arhandle->marker_num; j++) {
			marker = &arhandle->markerInfo[j];

			// Pattern found
			if (marker->idPatt > -1 && marker->idMatrix == -1) {
				match = &pattern_markers[marker->idPatt];

				if (!match->found) {
					arGetTransMatSquare(ar3DHandle, marker, width, match->transform);
				} else {
					arGetTransMatSquareCont(ar3DHandle, marker, match->transform, width, match->transform);
				}

				// copy values
				for (int x = 0; x < 3; x++) {
					for (int y = 0; y < 4; y++) {
						transform[x][y] = match->transform[x][y];
					}
				}
			}
			// Barcode found
			else if (marker->idMatrix > -1) {
				if (barcode_markers.find(marker->idMatrix) == barcode_markers.end()) {
					barcode_markers[marker->idMatrix] = simple_marker();

					match = &barcode_markers[marker->idMatrix];
					match->found = true;
					match->id = marker->idMatrix;
					arGetTransMatSquare(ar3DHandle, marker, width, match->transform);
				}
				else {
					match = &barcode_markers[marker->idMatrix];
					arGetTransMatSquareCont(ar3DHandle, marker, match->transform, width, match->transform);
				}
				// copy values
				for (int x = 0; x < 3; x++) {
					for (int y = 0; y < 4; y++) {
						transform[x][y] = match->transform[x][y];
					}
				}
			}
			// everything else
			else {
				arGetTransMatSquare(ar3DHandle, &arhandle->markerInfo[j], width, transform);
			}

			arglCameraViewRH(transform, modelView, CAMERA_VIEW_SCALE);
			transferMarker(&arhandle->markerInfo[j], j);
		}

		arglCameraFrustumRH(&paramLT->param, NEAR_PLANE, FAR_PLANE, cameraLens);

		// toggle transform found flag
		for (j = 0; j < pattern_markers.size(); j++) {
			match = &pattern_markers[j];
			match->found = false;
			for (k=0; k < arhandle->marker_num; k++) {
				marker = &arhandle->markerInfo[k];
				if (marker->idPatt == match->id && marker->idMatrix == -1) {
					match->found = true;
					break;
				}
			}
		}

		for (auto &any : barcode_markers) {
			match = &any.second;
			match->found = false;
			for (k=0; k < arhandle->marker_num; k++) {
				marker = &arhandle->markerInfo[k];
				if (marker->idMatrix == -1) {
					match->found = true;
					break;
				}
			}

			//if (!match->found) barcode_markers.erase(marker->id);
		}

		for (j = 0; j < multi_markers.size(); j++) {
			multiMatch = &multi_markers[j];
			multiMatch->found = false;
			ARMultiMarkerInfoT *arMulti = multiMatch->multiMarkerHandle;


			int err = 0;
			int robustFlag = 1;

			if( robustFlag ) {
				err = arGetTransMatMultiSquareRobust( ar3DHandle, markerInfo, markerNum, arMulti );
			} else {
				err = arGetTransMatMultiSquare( ar3DHandle, markerInfo, markerNum, arMulti );
			}
			arglCameraViewRH(arMulti->trans, modelView, CAMERA_VIEW_SCALE);
			transferMultiMarker(multiMatch->id);

			// for (k = 0; k < arMulti->marker_num; k++) {
			// 	matrixMul(transform, arMulti->trans, (arMulti->marker[k]).trans);
			// 	arglCameraViewRH(transform, modelView, CAMERA_VIEW_SCALE);
			// 	transferMultiMarker(multiMatch->id, k, &(arMulti->marker[k]));
			// }
		}
	}
}

#include "ARBindEM.cpp"