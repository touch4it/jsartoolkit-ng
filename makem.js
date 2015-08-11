/*
 * Simple script for running emcc on ARToolKit
 * @author zz85 github.com/zz85
 */

var USE_WEBIDL = 0;
var USE_EMBIND = 0;
var HAVE_NFT = 1;

var EMSCRIPTEN_PATH = '/usr/lib/emsdk_portable/emscripten/master/'

var EMCC = EMSCRIPTEN_PATH + 'emcc';
var EMPP = EMSCRIPTEN_PATH + 'em++';
var WEBIDL = 'python ' + EMSCRIPTEN_PATH + 'tools/webidl_binder.py';
var OPTIMIZE_FLAGS = ' -Oz '; // -Oz
// Oz smallest

var MAIN_SOURCES = [
	'emscripten/ARMarkerNFT.c',
	'emscripten/trackingSub.c',
	'emscripten/thread_sub.c',
	'emscripten/helloar.cpp'
].join(' ');

var sources = [
	'AR/arLabelingSub/*.c',
	'AR/*.c',
	'ARICP/*.c',
	'Gl/gsub_lite.c',
	'ARWrapper/ARMarker.cpp',
	'ARWrapper/ARMarkerMulti.cpp',
	// ARMarkerNFT // trackingSub
	'ARWrapper/ARController.cpp',
	// 'ARWrapper/ARPattern.cpp'

].map(function(src) {
	return 'lib/SRC/' + src;
});

var ar2_sources = [
	'handle.c',
	'imageSet.c',
	'jpeg.c',
	'marker.c',
	'featureMap.c',
	'featureSet.c',
	'selectTemplate.c',
	'surface.c',
	'tracking.c',
	'tracking2d.c',
	'matching.c',
	'matching2.c',
	'template.c',
	'searchPoint.c',
	'coord.c',
	'util.c',
].map(function(src) {
	return 'lib/SRC/AR2/' + src;
});

var kpm_sources = [
	'kpmHandle.c*',
	'kpmRefDataSet.c*',
	'kpmMatching.c*',
	'kpmResult.c*',
	'kpmUtil.c*',
	'kpmFopen.c*',
	'FreakMatcher/detectors/DoG_scale_invariant_detector.c*',
	'FreakMatcher/detectors/gaussian_scale_space_pyramid.c*',
	'FreakMatcher/detectors/gradients.c*',
	'FreakMatcher/detectors/harris.c*',
	'FreakMatcher/detectors/orientation_assignment.c*',
	'FreakMatcher/detectors/pyramid.c*',
	'FreakMatcher/facade/visual_database_facade.c*',
	'FreakMatcher/matchers/hough_similarity_voting.c*',
	'FreakMatcher/matchers/freak.c*',
	'FreakMatcher/framework/date_time.c*',
	'FreakMatcher/framework/image.c*',
	'FreakMatcher/framework/logger.c*',
	'FreakMatcher/framework/timers.c*',
].map(function(src) {
	return 'lib/SRC/KPM/' + src;
});

if (HAVE_NFT) {
	sources = sources
	.concat(ar2_sources)
	.concat(kpm_sources);
}

// if (USE_EMBIND) sources.push('ARBindEM.cpp')
if (USE_WEBIDL) sources.push('ARBindIDL.cpp')
// sources.push(MAIN_SOURCES)



console.log('sources: ' + sources);

var DEFINES = ' ';
if (HAVE_NFT) DEFINES += ' -D HAVE_NFT ';

var FLAGS = '' + OPTIMIZE_FLAGS;

var MEM = 256 * 1024 * 1024; // 64MB
FLAGS += ' -s TOTAL_MEMORY=' + MEM + ' ';
if (USE_EMBIND) FLAGS += ' --bind ';
if (USE_WEBIDL) FLAGS += ' --post-js glue.js '

// FLAGS += ' -s EMTERPRETIFY=1 ';
// FLAGS += ' -s EMTERPRETIFY_ASYNC=1 ';
// FLAGS += ' -s EMTERPRETIFY_WHITELIST="[\'_wildwebmidi\']" ';

/* DEBUG FLAGS */
var DEBUG_FLAGS = ' -g '; FLAGS += DEBUG_FLAGS;
// FLAGS += ' -s ASSERTIONS=2 '
// FLAGS += ' -s ASSERTIONS=1 '
// FLAGS += ' --profiling-funcs '
// FLAGS += ' -s EMTERPRETIFY_ADVISE=1 '
// FLAGS += ' -s ALLOW_MEMORY_GROWTH=1';
// FLAGS += '  -s DEMANGLE_SUPPORT=1 ';

// include/linux-i686/ macosx-universal linux-x86_64


var INCLUDES = [
	'include',
	'lib/SRC/KPM/FreakMatcher',
	'include/macosx-universal/',
	'../jpeg-6b',
	'emscripten'
].map(function(s) { return '-I' + s }).join(' ');

var EXPORTED_FUNCTIONS = JSON.stringify(
	[
		'setup',
		'process',
		'teardown',
		'setDebugMode',
		'setThreshold',
		'startSetupMarker'
	].map(function(x) { return '_' + x; })
);

var make_bindings = WEBIDL + ' arbindings.idl glue';


var compile_arlib = EMCC + ' ' + INCLUDES + ' '
	+ sources.join(' ')
	+ FLAGS + ' ' + DEFINES + ' -o libar.bc '
	+ ' -s EXPORTED_FUNCTIONS=\'' + EXPORTED_FUNCTIONS + '\'';

var compile_kpm = EMCC + ' ' + INCLUDES + ' '
	+ kpm_sources.join(' ')
	+ FLAGS + ' ' + DEFINES + ' -o libkpm.bc '
	+ ' -s EXPORTED_FUNCTIONS=\'' + EXPORTED_FUNCTIONS + '\'';


// Memory Allocations
// jmemansi.c
// jmemname.c jmemnobs.c jmemdos.c jmemmac.c
var jpegs = 'jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
        jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
        jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c \
        jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c \
        jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c \
        jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c \
        jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
        jquant2.c jutils.c jmemmgr.c \
        jmemname.c \
        jcapimin.c jcapistd.c jctrans.c jcparam.c \
        jdatadst.c jcinit.c jcmaster.c jcmarker.c jcmainct.c \
        jcprepct.c jccoefct.c jccolor.c jcsample.c jchuff.c \
        jcphuff.c jcdctmgr.c jfdctfst.c jfdctflt.c \
        jfdctint.c'.split(/\s+/).join(' ../jpeg-6b/')

// ./android/jni/OpenCV-2.4.3-android-sdk/sdk/native/3rdparty/libs/x86/liblibjpeg.a
// ./emscripten-libjpeg-turbo/libturbojpeg*.o
// + '../jpeg-9a/j*.o '
var compile_libjpeg = EMCC + ' ' + INCLUDES + ' '
	+ '../jpeg-6b/' +  jpegs
	+ FLAGS + ' ' + DEFINES + ' -o libjpeg.bc '
	+ ' -s EXPORTED_FUNCTIONS=\'' + EXPORTED_FUNCTIONS + '\'';

//  --preload-file bin



var compile_combine = EMCC + ' ' + INCLUDES + ' '
	+ ' libar.bc libjpeg.bc ' + MAIN_SOURCES
	+ FLAGS + ' ' + DEFINES + ' -o web/test.html '
	+ ' -s EXPORTED_FUNCTIONS=\'' + EXPORTED_FUNCTIONS + '\'';

// EMPP ARBindEM.cpp

var compile_all = EMCC + ' ' + INCLUDES + ' '
	+ sources.join(' ')
	+ FLAGS + ' ' + DEFINES + ' -o web/test.html '
	+ ' -s EXPORTED_FUNCTIONS=\'' + EXPORTED_FUNCTIONS + '\'';

var
	exec = require('child_process').exec,
	child;

function onExec(error, stdout, stderr) {
	if (stdout) console.log('stdout: ' + stdout);
	if (stderr) console.log('stderr: ' + stderr);
	if (error !== null) {
		console.log('exec error: ' + error);
	} else {
		nextJob();
	}
}

function nextJob() {
	if (!jobs.length) {
		console.log('Jobs completed');
		return;
	}
	var cmd = jobs.shift();
	console.log('\nRunning command: ' + cmd + '\n');
	exec(cmd, onExec);
}

var jobs = [
	// compile_all
	compile_arlib,
	compile_libjpeg,
	compile_combine,
];

if (USE_WEBIDL) jobs.unshift(make_bindings);

nextJob();
