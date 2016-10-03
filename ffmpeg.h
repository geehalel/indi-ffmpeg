#ifndef FFMPEG_H
#define FFMPEG_H

#include <indiccd.h>

#ifdef __linux__
#include <stream_recorder.h>
#else
class StreamRecorder {};
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>  
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
  
#ifdef __cplusplus 
}
#endif
//#include <ctime>
#include <thread>

class FFMpeg : public INDI::CCD
{
public:
    FFMpeg();
    ~FFMpeg();
    void ISGetProperties(const char *dev);
    virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);
    bool StartStreaming();
    bool StopStreaming();
protected:
    // General device functions
    bool Connect();
    bool Connect(char *device);
    bool Disconnect();
    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();

    void addFFMpegControls();
    
private:
    IText CaptureDeviceT[2];
    ISwitchVectorProperty *VideoStreamSP;
    ITextVectorProperty CaptureDeviceTP;
    INumberVectorProperty FFMpegControlsNP;

    IBLOB *StreamFrame;
    IBLOBVectorProperty *StreamFrameBP;


    std::thread capture_thread;
    bool join_thread;
    static void RunCaptureThread(FFMpeg *ffmpeg);
    void run_capture();
    bool is_capturing;
    bool is_streaming;
    void start_capturing();
    void stop_capturing();    
    void start_streaming();
    void stop_streaming();

    AVInputFormat   *pInFmt;
    AVFormatContext *pFormatCtx;
    int              videoStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVDictionary    *optionsDict;
    AVFrame         *pFrame; 
    AVFrame         *pFrameRGB;
    
    unsigned char *compressedFrame;

};
#endif // FFMPEG_H
