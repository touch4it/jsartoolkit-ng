#include <stdio.h>
#include <AR/ar.h>
#include <AR/gsub_lite.h>
#include <emscripten.h>

#include <helloar.h>

// #include <AR/arMulti.h>
// #include <AR/video.h>
// #include <AR/gsub_lite.h>
// #include <AR/arFilterTransMat.h>
#include <AR2/tracking.h>

#include "ARMarkerNFT.h"
#include "trackingSub.h"

// ============================================================================
//	Constants
// ============================================================================

#define PAGES_MAX               10          // Maximum number of pages expected. You can change this down (to save memory) or up (to accomodate more pages.)

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
static const ARdouble NEAR_PLANE = 0.0001;    ///< Near plane d	istance for projection matrix calculation
static const ARdouble FAR_PLANE = 10000.0;   ///< Far plane distance for projection matrix calculation
// 100, 10000 10 5000.0f

static ARdouble cameraLens[16];
static ARdouble modelView[16];

static char patt_name[]  = "/patt.hiro";
static int			gPatt_id;				// Per-marker, but we are using only 1 marker.
static ARPattHandle	*gARPattHandle = NULL;

// NFT Support
const char markerConfigDataFilename[] = "/Data2/markers.dat"; // /Data2

// Markers.
ARMarkerNFT *markersNFT = NULL;
int markersNFTCount = 0;

// NFT.
static THREAD_HANDLE_T     *threadHandle = NULL;
static AR2HandleT          *ar2Handle = NULL;
static KpmHandle           *kpmHandle = NULL;
static int                  surfaceSetCount = 0;
static AR2SurfaceSetT      *surfaceSet[PAGES_MAX];


extern "C" {
	void setMatrixCodeType() {
		printf("setting matrix mode\n");
		arSetPatternDetectionMode(arhandle, AR_MATRIX_CODE_DETECTION);
		arSetMatrixCodeType(arhandle, AR_MATRIX_CODE_4x4);
	}

	void setPatternDetectionMode(AR_MATRIX_CODE_TYPE type) {
		arSetMatrixCodeType(arhandle, type);
	}

	// void nft() {
	// 	//
	// 	// AR init.
	// 	//

	// 	// Create the OpenGL projection from the calibrated camera parameters.
	// 	arglCameraFrustumRH(&(gCparamLT->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, cameraLens);

	// 	if (!initNFT(gCparamLT, arVideoGetPixelFormat())) {
	// 		ARLOGe("main(): Unable to init NFT.\n");
	// 		exit(-1);
	// 	}

	// 	newMarkers(markerConfigDataFilename, &markersNFT, &markersNFTCount);
	// 	if (!markersNFTCount) {
	// 		ARLOGe("Error loading markers from config. file '%s'.\n", markerConfigDataFilename);
	// 		cleanup();
	// 		exit(-1);
	// 	}
	// 	ARLOGi("Marker count = %d\n", markersNFTCount);


	// Modifies globals: kpmHandle, ar2Handle.
	static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat)
	{
		ARLOGd("Initialising NFT.\n");
		//
		// NFT init.
		//

		// KPM init.
		kpmHandle = kpmCreateHandle(cparamLT, pixFormat);
		if (!kpmHandle) {
			ARLOGe("Error: kpmCreateHandle.\n");
			return (FALSE);
		}
		printf("kpmCreateHandle()\n");

		// AR2 init.
		if( (ar2Handle = ar2CreateHandle(cparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL ) {
			ARLOGe("Error: ar2CreateHandle.\n");
			kpmDeleteHandle(&kpmHandle);
			return (FALSE);
		}
		printf("ar2CreateHandle()\n");

		if (threadGetCPU() <= 1) {
			ARLOGi("Using NFT tracking settings for a single CPU.\n");
			ar2SetTrackingThresh(ar2Handle, 5.0);
			ar2SetSimThresh(ar2Handle, 0.50);
			ar2SetSearchFeatureNum(ar2Handle, 16);
			ar2SetSearchSize(ar2Handle, 6);
			ar2SetTemplateSize1(ar2Handle, 6);
			ar2SetTemplateSize2(ar2Handle, 6);
		} else {
			ARLOGi("Using NFT tracking settings for more than one CPU.\n");
			ar2SetTrackingThresh(ar2Handle, 5.0);
			ar2SetSimThresh(ar2Handle, 0.50);
			ar2SetSearchFeatureNum(ar2Handle, 16);
			ar2SetSearchSize(ar2Handle, 12);
			ar2SetTemplateSize1(ar2Handle, 6);
			ar2SetTemplateSize2(ar2Handle, 6);
		}
		// NFT dataset loading will happen later.
		return (TRUE);
	}

	// Modifies globals: threadHandle, surfaceSet[], surfaceSetCount
	static int unloadNFTData(void)
	{
		int i, j;

		if (threadHandle) {
			ARLOGi("Stopping NFT2 tracking thread.\n");
			trackingInitQuit(&threadHandle);
		}
		j = 0;
		for (i = 0; i < surfaceSetCount; i++) {
			if (j == 0) ARLOGi("Unloading NFT tracking surfaces.\n");
			ar2FreeSurfaceSet(&surfaceSet[i]); // Also sets surfaceSet[i] to NULL.
			j++;
		}
		if (j > 0) ARLOGi("Unloaded %d NFT tracking surfaces.\n", j);
		surfaceSetCount = 0;

		return 0;
	}

	// References globals: markersNFTCount
	// Modifies globals: threadHandle, surfaceSet[], surfaceSetCount, markersNFT[]
	static int loadNFTData(void)
	{
		int i;
		KpmRefDataSet *refDataSet;

		// If data was already loaded, stop KPM tracking thread and unload previously loaded data.
		if (threadHandle) {
			ARLOGi("Reloading NFT data.\n");
			unloadNFTData();
		} else {
			ARLOGi("Loading NFT data.\n");
		}

		refDataSet = NULL;

		for (i = 0; i < markersNFTCount; i++) {
			// Load KPM data.
			KpmRefDataSet  *refDataSet2;
			ARLOGi("Reading %s.fset3\n", markersNFT[i].datasetPathname);
			if (kpmLoadRefDataSet(markersNFT[i].datasetPathname, "fset3", &refDataSet2) < 0 ) {
				ARLOGe("Error reading KPM data from %s.fset3\n", markersNFT[i].datasetPathname);
				markersNFT[i].pageNo = -1;
				continue;
			}
			markersNFT[i].pageNo = surfaceSetCount;
			ARLOGi("  Assigned page no. %d.\n", surfaceSetCount);
			if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0) {
				ARLOGe("Error: kpmChangePageNoOfRefDataSet\n");
				exit(-1);
			}
			if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
				ARLOGe("Error: kpmMergeRefDataSet\n");
				exit(-1);
			}
			ARLOGi("  Done.\n");

			// Load AR2 data.
			ARLOGi("Reading %s.fset\n", markersNFT[i].datasetPathname);

			if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(markersNFT[i].datasetPathname, "fset", NULL)) == NULL ) {
				ARLOGe("Error reading data from %s.fset\n", markersNFT[i].datasetPathname);
			}
			ARLOGi("  Done.\n");

			surfaceSetCount++;
			if (surfaceSetCount == PAGES_MAX) break;
		}
		if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0) {
			ARLOGe("Error: kpmSetRefDataSet\n");
			exit(-1);
		}
		kpmDeleteRefDataSet(&refDataSet);

		// // Start the KPM tracking thread.
		// threadHandle = trackingInitInit(kpmHandle);
		// if (!threadHandle) exit(-1);

		ARLOGi("Loading of NFT data complete.\n");
		return (TRUE);
	}

	static void nft_cleanup(void)
	{
		if (markersNFT) deleteMarkers(&markersNFT, &markersNFTCount);

		// NFT cleanup.
		//TODO
		unloadNFTData();
		ARLOGd("Cleaning up ARToolKit NFT handles.\n");
		ar2DeleteHandle(&ar2Handle);
		kpmDeleteHandle(&kpmHandle);
	}



	int setup(int width, int height) {
		/// NFT VERSION!!

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

		//
		// AR init.
		//

		if (!initNFT(paramLT, AR_PIXEL_FORMAT_RGBA)) {
			ARLOGe("main(): Unable to init NFT.\n");
			exit(-1);
		}

		printf("initNFT() done\n");

		//
		// Markers setup.
		//

		// Load marker(s).
		newMarkers(markerConfigDataFilename, &markersNFT, &markersNFTCount);
		if (!markersNFTCount) {
			ARLOGe("Error loading markers from config. file '%s'.\n", markerConfigDataFilename);
			teardown();
			exit(-1);
		}
		ARLOGi("Marker count = %d\n", markersNFTCount);

		// Marker data has been loaded, so now load NFT data.
		if (!loadNFTData()) {
			ARLOGe("Error loading NFT data.\n");
			teardown();
			exit(-1);
		}




		gVideoFrameSize = width * height * 4 * sizeof(ARUint8);
		gVideoFrame = (ARUint8*) malloc(gVideoFrameSize);

		printf("Allocated gVideoFrameSize %d\n", gVideoFrameSize);

		EM_ASM_({
			onFrameMalloc({
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

		setMatrixCodeType();

		return 0;
	}

	//
	// NFT Get Frame Loop
	//

	void nft_loop() {
		int             i, j, k;
		// NFT results.
	    static int detectedPage = -2; // -2 Tracking not inited, -1 tracking inited OK, >= 0 tracking online on page.
	    static float trackingTrans[3][4];

		// Run marker detection on frame
        if (threadHandle) {
            // Perform NFT tracking.
            float            err;
            int              ret;
            int              pageNo;

            if( detectedPage == -2 ) {
                trackingInitStart( threadHandle, gVideoFrame );
                detectedPage = -1;
            }
            if( detectedPage == -1 ) {
                ret = trackingInitGetResult( threadHandle, trackingTrans, &pageNo);
                if( ret == 1 ) {
                    if (pageNo >= 0 && pageNo < surfaceSetCount) {
                        ARLOGd("Detected page %d.\n", pageNo);
                        detectedPage = pageNo;
                        ar2SetInitTrans(surfaceSet[detectedPage], trackingTrans);
                    } else {
                        ARLOGe("Detected bad page %d.\n", pageNo);
                        detectedPage = -2;
                    }
                } else if( ret < 0 ) {
                    ARLOGd("No page detected.\n");
                    detectedPage = -2;
                }
            }
            if( detectedPage >= 0 && detectedPage < surfaceSetCount) {
                if( ar2Tracking(ar2Handle, surfaceSet[detectedPage], gVideoFrame, trackingTrans, &err) < 0 ) {
                    ARLOGd("Tracking lost.\n");
                    detectedPage = -2;
                } else {
                    ARLOGd("Tracked page %d (max %d).\n", detectedPage, surfaceSetCount - 1);
                }
            }
        } else {
            ARLOGe("Error: threadHandle\n");
            detectedPage = -2;
        }

        // Update markers.
        for (i = 0; i < markersNFTCount; i++) {
            markersNFT[i].validPrev = markersNFT[i].valid;
            if (markersNFT[i].pageNo >= 0 && markersNFT[i].pageNo == detectedPage) {
                markersNFT[i].valid = TRUE;
                for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) markersNFT[i].trans[j][k] = trackingTrans[j][k];
            }
            else markersNFT[i].valid = FALSE;
            if (markersNFT[i].valid) {

                // Filter the pose estimate.
                if (markersNFT[i].ftmi) {
                    if (arFilterTransMat(markersNFT[i].ftmi, markersNFT[i].trans, !markersNFT[i].validPrev) < 0) {
                        ARLOGe("arFilterTransMat error with marker %d.\n", i);
                    }
                }

                if (!markersNFT[i].validPrev) {
                    // Marker has become visible, tell any dependent objects.
                    // --->
                }

                // We have a new pose, so set that.
                arglCameraViewRH(markersNFT[i].trans, markersNFT[i].pose.T, CAMERA_VIEW_SCALE);
                // Tell any dependent objects about the update.
                // --->

            } else {

                if (markersNFT[i].validPrev) {
                    // Marker has ceased to be visible, tell any dependent objects.
                    // --->
                }
            }
        }
	}

	int setup_normal(int width, int height) {
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


		if (!initNFT(paramLT, AR_PIXEL_FORMAT_RGBA)) {
			ARLOGe("main(): Unable to init NFT.\n");
			exit(-1);
		}

		printf("initNFT() done\n");

		gVideoFrameSize = width * height * 4 * sizeof(ARUint8);
		gVideoFrame = (ARUint8*) malloc(gVideoFrameSize);

		printf("Allocated gVideoFrameSize %d\n", gVideoFrameSize);

		EM_ASM_({
			onFrameMalloc({
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

		setMatrixCodeType();

		return 0;
	}

	int teardown() {
		nft_cleanup();

		if (gVideoFrame) {
			free(gVideoFrame);
			gVideoFrame = NULL;
			gVideoFrameSize = 0;
		}

		if (ar3DHandle) ar3DDeleteHandle(&ar3DHandle);
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

	void startSetupMarker() {
		// Load marker(s).
		if (!setupMarker(patt_name, &gPatt_id, arhandle, &gARPattHandle)) {
			ARLOGe("main(): Unable to set up AR marker.\n");
			teardown();
			exit(-1);
		}
	}

	void setThreshold(int threshold) {
		arSetLabelingThresh(arhandle, threshold);
		// default 100
		// arSetLabelingThreshMode
		// AR_LABELING_THRESH_MODE_MANUAL, AR_LABELING_THRESH_MODE_AUTO_MEDIAN, AR_LABELING_THRESH_MODE_AUTO_OTSU, AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE
	}

	ARUint8* setDebugMode(int enable) {
		arSetDebugMode(arhandle, enable ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE);

		return arhandle->labelInfo.bwImage;
	}

	void transferMarker(ARMarkerInfo* markerInfo, int index) {
		// see /artoolkit5/doc/apiref/ar_h/index.html#//apple_ref/c/tdef/ARMarkerInfo

		EM_ASM_({
			var $a = arguments;
			var i = 24;
			onGetMarker({
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

		nft_loop();

		int success = arDetectMarker(
			arhandle, gVideoFrame
		);

		if (success) return;

		// printf("arDetectMarker: %d\n", success);

		int markerNum = arGetMarkerNum(arhandle);
		ARMarkerInfo* markerInfo = arGetMarker(arhandle);

		EM_ASM_({
			onMarkerNum($0);
		}, markerNum);

		int i, j, k;

		k = -1;
		for (j = 0; j < arhandle->marker_num; j++) {
			transferMarker(&arhandle->markerInfo[j], j);
			if (arhandle->markerInfo[j].id == gPatt_id) {
				if (k == -1) k = j; // First marker detected.
				else if (arhandle->markerInfo[j].cf > arhandle->markerInfo[k].cf) k = j; // Higher confidence marker detected.
			}
		}

		// printf("Best match: %d\n", k);

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
			arglCameraFrustumRH(&paramLT->param, NEAR_PLANE, FAR_PLANE, cameraLens);
			arglCameraViewRH(transform, modelView, CAMERA_VIEW_SCALE);

			// transferMarker(markerInfo, 0);
		}

		// EclipseProjects/ARMovie/jni/ARMovie.cpp
		// EclipseProjects/ARNative/jni/ARNative.cpp
		// RGBA buffer simpleLite

		// printf("DONE%d\n", success);
	}


}

// #include "ARBindEM.cpp"
