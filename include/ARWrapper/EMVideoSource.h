#ifdef __EMSCRIPTEN__

class EmVideoSource : public VideoSource {

private:

	bool newFrameArrived;
    ARUint8 *localFrameBuffer;
    size_t frameBufferSize;

    static void getVideoReadyAndroidCparamCallback(const ARParam *cparam_p, void *userdata);
    bool getVideoReadyAndroid2(const ARParam *cparam_p);

protected:

    // AR2VideoParamT *gVid;
    // int gCameraIndex;
    // bool gCameraIsFrontFacing;

public:

	EmVideoSource();

	virtual bool open();

    /**
     * Returns the size of current frame.
     * @return		Size of the buffer containing the current video frame
     */
    size_t getFrameSize();

	void acceptImage(ARUint8* ptr);

	virtual bool captureFrame();

	virtual bool close();

	virtual const char* getName();

};

#endif

#endif // __EMSCRIPTEN__