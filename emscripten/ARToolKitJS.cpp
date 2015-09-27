#include <stdio.h>
#include <AR/ar.h>
#include <AR/gsub_lite.h>
#include <emscripten.h>
#include <string>

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
static ARdouble CAMERA_VIEW_SCALE = 2.0;
static ARdouble NEAR_PLANE = 0.0001;    ///< Near plane d	istance for projection matrix calculation
static ARdouble FAR_PLANE = 10000.0;   ///< Far plane distance for projection matrix calculation

static ARdouble cameraLens[16];
static ARdouble modelView[16];

static char patt_name[]  = "/patt.hiro";
static int			gPatt_id;				// Per-marker, but we are using only 1 marker.
static ARPattHandle	*gARPattHandle = NULL;

int patternDetectionMode = 0;
AR_MATRIX_CODE_TYPE matrixType = AR_MATRIX_CODE_3x3;

extern "C" {
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

	int setup(int width, int height) {
		// setup parameters
		arParamClear(&param, width, height, AR_DIST_FUNCTION_VERSION_DEFAULT);

		if ((paramLT = arParamLTCreate(&param, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
			ARLOGe("setupCamera(): Error: arParamLTCreate.\n");
			return (FALSE);
		}

		printf("arParamLTCreated\n..%d, %d\n", (paramLT->param).xsize, param.ysize);

		// setup camera
		if ((arhandle = arCreateHandle(paramLT)) == NULL) {
			ARLOGe("setupCamera(): Error: arCreateHandle.\n");
			return (FALSE);
		}

		printf("arCreateHandle done\n");

		// AR_DEFAULT_PIXEL_FORMAT
		int set = arSetPixelFormat(arhandle, AR_PIXEL_FORMAT_RGBA);


		gVideoFrameSize = width * height * 4 * sizeof(ARUint8);
		gVideoFrame = (ARUint8*) malloc(gVideoFrameSize);

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

		ar3DHandle = ar3DCreateHandle(&paramLT->param);
		if (ar3DHandle == NULL) {
			ARLOGe("Error creating 3D handle");
		}

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

	static int setupMarker(const char *patt_name, int *patt_id, ARHandle *arhandle, ARPattHandle **pattHandle_p) {
		if ((*pattHandle_p = arPattCreateHandle()) == NULL) {
			ARLOGe("setupMarker(): Error: arPattCreateHandle.\n");
			return (FALSE);
		}

		// Loading only 1 pattern in this example.
		if ((*patt_id = arPattLoad(*pattHandle_p, patt_name)) < 0) {
			ARLOGe("setupMarker(): Error loading pattern file %s.\n", patt_name);
			arPattDeleteHandle(*pattHandle_p);
			return (FALSE);
		}

		arPattAttach(arhandle, *pattHandle_p);

		return (TRUE);
	}

	int startSetupMarker(std::string patt_name) {
		// const char *patt_name
		// Load marker(s).
		if (!setupMarker(patt_name.c_str(), &gPatt_id, arhandle, &gARPattHandle)) {
			ARLOGe("main(): Unable to set up AR marker.\n");
			teardown();
			exit(-1);
		}

		return gPatt_id;
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
		for (j = 0; j < arhandle->marker_num; j++) {
			transferMarker(&arhandle->markerInfo[j], j);
			// if (arhandle->markerInfo[j].id == gPatt_id) {
			// 	if (k == -1) k = j; // First marker detected.
			// 	else if (arhandle->markerInfo[j].cf > arhandle->markerInfo[k].cf) k = j; // Higher confidence marker detected.
			// }
		}

		// printf("Best match: %d\n", k);

		arglCameraFrustumRH(&paramLT->param, NEAR_PLANE, FAR_PLANE, cameraLens);

		if (!markerNum) {
			transformContinue = 0;
		} else {
			if (transformContinue) {
				arGetTransMatSquareCont(ar3DHandle, markerInfo, transform, width, transform);
			} else {
				arGetTransMatSquare(ar3DHandle, markerInfo, width, transform);
				transformContinue = 1;
			}

			// Create the OpenGL projection from the calibrated camera parameters.

			arglCameraViewRH(transform, modelView, CAMERA_VIEW_SCALE);
		}
	}
}

#include "ARBindEM.cpp"
