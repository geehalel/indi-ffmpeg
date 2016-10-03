//#include <unistd.h>
//#include <memory>
//#include <signal.h>
#include <zlib.h>

#include "ffmpeg.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/dict.h"
#ifdef __cplusplus 
}
#endif
#include "config.h"

std::unique_ptr<FFMpeg> ffmpeg(new FFMpeg());

static void print_option(const AVClass *tclass, const AVOption *o, const char *prefix)
{
  fprintf(stderr,"%s  option %s type ", prefix, o->name);
    switch (o->type) {
    case AV_OPT_TYPE_BINARY:   fprintf(stderr,"hexadecimal string"); break;
    case AV_OPT_TYPE_STRING:   fprintf(stderr,"string");             break;
    case AV_OPT_TYPE_INT:
    case AV_OPT_TYPE_INT64:    fprintf(stderr,"integer");            break;
    case AV_OPT_TYPE_FLOAT:
    case AV_OPT_TYPE_DOUBLE:   fprintf(stderr,"float");              break;
    case AV_OPT_TYPE_RATIONAL: fprintf(stderr,"rational number");    break;
    case AV_OPT_TYPE_FLAGS:    fprintf(stderr,"flags");              break;
    default:                   fprintf(stderr,"value");              break;
    }
    fprintf(stderr," i/o ");

    if (o->flags & AV_OPT_FLAG_DECODING_PARAM) {
        fprintf(stderr,"input");
        if (o->flags & AV_OPT_FLAG_ENCODING_PARAM)
            fprintf(stderr,"/");
    }
    if (o->flags & AV_OPT_FLAG_ENCODING_PARAM)
        fprintf(stderr,"output");

    fprintf(stderr,"\n");
    if (o->help)
      fprintf(stderr,"%s    %s\n", prefix, o->help);

    if (o->unit) {
        const AVOption *u = NULL;
        fprintf(stderr,"%s    Possible values:\n", prefix);

        while ((u = av_opt_next(&tclass, u)))
            if (u->type == AV_OPT_TYPE_CONST && u->unit && !strcmp(u->unit, o->unit))
	      fprintf(stderr,"%s      %s(%s)\n", prefix, u->name, u->help ? u->help : "N/A");
        //fprintf(stderr,"\n");
    }
}

static void show_opts(const AVClass *tclass, const char *prefix)
{
    const AVOption *o = NULL;

    fprintf(stderr,"%sAVOption for class %s\n", prefix, tclass->class_name);
    while ((o = av_opt_next(&tclass, o)))
        if (o->type != AV_OPT_TYPE_CONST)
	  print_option(tclass, o, prefix);
    fprintf(stderr,"%send Option\n", prefix);
}

static void show_format_opts(void)
{
    AVInputFormat *iformat = NULL;
    AVOutputFormat *oformat = NULL;

    fprintf(stderr,"@section Generic format AVOptions\n");
    show_opts(avformat_get_class(), "");

    fprintf(stderr,"@section Format-specific AVOptions\n");
    while ((iformat = av_iformat_next(iformat))) {
        if (!iformat->priv_class)
            continue;
        fprintf(stderr,"@subsection %s AVOptions\n", iformat->priv_class->class_name);
        show_opts(iformat->priv_class, "  ");
    }
    while ((oformat = av_oformat_next(oformat))) {
        if (!oformat->priv_class)
            continue;
        fprintf(stderr,"@subsection %s AVOptions\n", oformat->priv_class->class_name);
        show_opts(oformat->priv_class, "  ");
    }
}

static void show_codec_opts(void)
{
    AVCodec *c = NULL;

    fprintf(stderr,"@section Generic codec AVOptions\n");
    show_opts(avcodec_get_class(), "");

    fprintf(stderr,"@section Codec-specific AVOptions\n");
    while ((c = av_codec_next(c))) {
        if (!c->priv_class)
            continue;
        fprintf(stderr,"@subsection %s AVOptions\n", c->priv_class->class_name);
        show_opts(c->priv_class, "  ");
    }
}


void ISInit()
{
    static int isInit =0;
    if (isInit == 1)
        return;
     isInit = 1;
     if(ffmpeg.get() == 0) ffmpeg.reset(new FFMpeg());
}
void ISGetProperties(const char *dev)
{
         ISInit();
         ffmpeg->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
         ISInit();
         ffmpeg->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText( const char *dev, const char *name, char *texts[], char *names[], int num)
{
         ISInit();
         ffmpeg->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
         ISInit();
         ffmpeg->ISNewNumber(dev, name, values, names, num);
}
void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
   INDI_UNUSED(dev);
   INDI_UNUSED(name);
   INDI_UNUSED(sizes);
   INDI_UNUSED(blobsizes);
   INDI_UNUSED(blobs);
   INDI_UNUSED(formats);
   INDI_UNUSED(names);
   INDI_UNUSED(n);
}
void ISSnoopDevice (XMLEle *root)
{
     ISInit();
     ffmpeg->ISSnoopDevice(root);
}
FFMpeg::FFMpeg()
{
  setVersion(FFMPEG_VERSION_MAJOR, FFMPEG_VERSION_MINOR);
  //fprintf(stderr, "FFMpeg Build Information: %s", cv::getBuildInformation().c_str());
  pInFmt = NULL;
  pFormatCtx = NULL;
  pCodecCtx = NULL;
  pCodec = NULL;
  optionsDict=NULL;
  pFrame = NULL; 
  pFrameRGB = NULL;
  av_register_all();
  avdevice_register_all();
  AVInputFormat * d=NULL;
  show_opts(avformat_get_class(), "");
  fprintf(stderr, "Available Video Input devices\n");
  while (d = av_input_video_device_next(d)) {
    fprintf(stderr, "%s: %s\n", d->name, d->long_name);
    if (d->priv_class) show_opts(d->priv_class, "  ");
    struct AVDeviceInfoList *devlist = NULL;
    int nbdev=0;
    /*
    struct AVDictionary *options=NULL;
    AVDictionaryEntry *pelem = NULL;


    fprintf(stderr, "  Options AVDictionary\n");
    while (pelem=av_dict_get(options, "", pelem, AV_DICT_IGNORE_SUFFIX)) {
      fprintf(stderr, "    %s : %s\n", pelem->key, pelem->value);
    }
    */
    fprintf(stderr, "  Input sources\n");
    if (!d->get_device_list) {
      fprintf(stderr, "Can not list sources. Not implemented.\n");
      continue;
    }
    nbdev=avdevice_list_input_sources(d, NULL, NULL, &devlist);
    int i;
    if (nbdev <0) {
      fprintf(stderr, "    Can not list sources\n");
      avdevice_free_list_devices(&devlist);
      continue;
    }
    for (i = 0; i < devlist->nb_devices; i++) {
      fprintf(stderr, "    %s %s [%s]\n", devlist->default_device == i ? "*" : " ",
	     devlist->devices[i]->device_name, devlist->devices[i]->device_description);
    }
    fprintf(stderr, "  Found %d devices\n", devlist->nb_devices);
    avdevice_free_list_devices(&devlist);
  }
  fprintf(stderr, "\n");
  /*
  AVInputFormat *p = NULL;
  fprintf(stderr, "Registered Input formats\n");
  while (p = av_iformat_next(p)) {
    //fprintf(stderr, "%s: %s,", (p->name?p->name:"null"), (p->long_name?p->long_name:"null"));
    fprintf(stderr, "  %s: ", (p->name?p->name:"null"));

    if (p->priv_class) {
      AVClassCategory c=p->priv_class->category;
      if (c)
	fprintf(stderr, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 
	    (c&AV_CLASS_CATEGORY_INPUT)?'I':'-',
	    (c&AV_CLASS_CATEGORY_OUTPUT)?'O':'-',
	    (c&AV_CLASS_CATEGORY_MUXER)?'M':'-',
	    (c&AV_CLASS_CATEGORY_DEMUXER)?'D':'-',
	    (c&AV_CLASS_CATEGORY_ENCODER)?'E':'-',
	    (c&AV_CLASS_CATEGORY_DECODER)?'d':'-',
	    (c&AV_CLASS_CATEGORY_FILTER)?'F':'-',
	    (c&AV_CLASS_CATEGORY_BITSTREAM_FILTER)?'B':'-',
	    (c&AV_CLASS_CATEGORY_SWSCALER)?'S':'-',
	    (c&AV_CLASS_CATEGORY_SWRESAMPLER)?'R':'-',
	    (c&AV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT)?'V':'-',
	    (c&AV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT)?'V':'-',	    
	    (c&AV_CLASS_CATEGORY_DEVICE_AUDIO_INPUT)?'A':'-',
	    (c&AV_CLASS_CATEGORY_DEVICE_AUDIO_OUTPUT)?'A':'-',
	    (c&AV_CLASS_CATEGORY_DEVICE_INPUT)?'I':'-',
	    (c&AV_CLASS_CATEGORY_DEVICE_OUTPUT)?'O':'-'	    
	    );
    }
    fprintf(stderr, "\n");
    }
  */

  
  compressedFrame=(unsigned char *)malloc(1);
  join_thread=false;
  
}

FFMpeg::~FFMpeg()
{

}


/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool FFMpeg::Connect()
{
    bool rc=false;

    if(isConnected()) return true;

    rc=Connect(CaptureDeviceT[0].text);

    return rc;
}

bool FFMpeg::Connect(char *device)
{
  int i;
  if(isConnected()) return true;

    ISwitchVectorProperty *connect=getSwitch("CONNECTION");
    if (connect) {
      connect->s=IPS_BUSY;
      IDSetSwitch(connect,"Connecting to device %s", device);
    }
    /*
    pInFmt = av_find_input_format("video4linux2");
    if( !pInFmt ){
      DEBUG(INDI::Logger::DBG_SESSION,"Failed to find video4llinux2 input format!");
      return false;
    }
    */
    if (avformat_open_input(&pFormatCtx, device, NULL, NULL)!=0) {
      DEBUG(INDI::Logger::DBG_SESSION,"Failed to open the video device, video file or image sequence!");
      return false;
    }
    
    AVDeviceCapabilitiesQuery *caps=NULL;
    fprintf(stderr, "Getting capabilities\n");
    if (avdevice_capabilities_create(&caps, pFormatCtx, NULL) >=0) {
      fprintf(stderr, "Printing capabilities\n");
      show_opts(caps->av_class, "  ");
      avdevice_capabilities_free(&caps, pFormatCtx);
    }
  
    /*
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
      DEBUG(INDI::Logger::DBG_SESSION,"Failed to get stream information!");
      return false;
    }
    */
    av_dump_format(pFormatCtx, 0, device, 0);
    
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
      if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
	videoStream=i;
	fprintf(stderr, "stream %d\n", i);
	//break;
      }
    if(videoStream==-1) {
      DEBUG(INDI::Logger::DBG_SESSION,"Failed to get a video stream!");
      return false;
    }
 
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
      DEBUG(INDI::Logger::DBG_SESSION,"Unsupported codec!");
      return false;      
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0) {
      DEBUG(INDI::Logger::DBG_SESSION,"Failed to open codec!");
      return false;
    }

    show_opts(pCodecCtx->av_class,"");
    if (pCodec->priv_class)
      show_opts(pCodec->priv_class,"");
    /*const AVOption *o = NULL;
    fprintf(stderr, "Options for ACodecContext\n");
    while (o = av_opt_next(pCodecCtx, o))
      fprintf(stderr, "  %s: %s\n", o->name, o->help); 
    */
    fprintf(stderr, "\n");
    /*struct video_data *s = (struct video_data *)pFormatCtx->priv_data;
    fprintf(stderr,"Device descriptor is %d\n", s->fd);
    */
    DEBUGF(INDI::Logger::DBG_SESSION, "Successfully connected to %s.", device);
    return true;
    
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool FFMpeg::Disconnect()
{
    if (isConnected()) {
      // Close the codecs
      avcodec_close(pCodecCtx);

      // Close the video file
      avformat_close_input(&pFormatCtx);
      
      DEBUG(INDI::Logger::DBG_SESSION,"FFMpeg disconnected successfully!");
    }
    return true;
}
/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char * FFMpeg::getDefaultName()
{
    return "FFMpeg";
}
/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool FFMpeg::initProperties()
{
    DEBUG(INDI::Logger::DBG_SESSION, "initProperties");
    // Must init parent properties first!
    INDI::CCD::initProperties();

    /* Capture Device */
    IUFillText(&CaptureDeviceT[0], "FFMPEG_INPUT_FILE", "Input", "/dev/video0");
    IUFillText(&CaptureDeviceT[1], "FFMPEG_INPUT_OPTS", "Input options", "-f v4l2");
    IUFillTextVector(&CaptureDeviceTP, CaptureDeviceT, NARRAY(CaptureDeviceT), getDeviceName(), "FFMPEG_INPUT", "FFMPEG", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    /* FFMpeg Controls */  
    //IUFillNumberVector(&FFMpegControlsNP, NULL, 0, getDeviceName(), "FFMPEG_CONTROLS", "FFMpeg Controls", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);


    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();
    SetCCDCapability(CCD_HAS_STREAMING);
    return true;
}

void FFMpeg::ISGetProperties(const char *dev)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "isGetProperties connected=%s",(isConnected()?"True":"False"));
    INDI::CCD::ISGetProperties(dev);

    defineText(&CaptureDeviceTP);
        
    if (isConnected())
    {
      defineSwitch(VideoStreamSP);
      //defineNumber(&FFMpegControlsNP);
    }    

}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool FFMpeg::updateProperties()
{

    // Call parent update properties first
    INDI::CCD::updateProperties();
    DEBUGF(INDI::Logger::DBG_SESSION, "updateProperties connected=%s", (isConnected()?"True":"False"));
    if (isConnected())
    {
      //buildSkeleton("indi_ffmpeg_sk.xml"); // defined only VIDEO_STREAM
      //VideoStreamSP=getSwitch("VIDEO_STREAM"); // replaced here with indi ccd streaming
      VideoStreamSP=getSwitch("CCD_VIDEO_STREAM");
      
      StreamFrameBP=getBLOB("CCD1");
      StreamFrame=StreamFrameBP->bp;
      
      defineSwitch(VideoStreamSP);
      
      //addFFMpegControls();
      //defineNumber(&FFMpegControlsNP);

    }
    else
    {
    // We're disconnected
      deleteProperty(VideoStreamSP->name);
      //deleteProperty(FFMpegControlsNP.name);
      //free(FFMpegControlsNP.np);
      //FFMpegControlsNP.nnp=0;
    }
     return true;
}



void FFMpeg::addFFMpegControls()
{

}

bool FFMpeg::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
      /* ignore if not ours */
    if (dev && strcmp (getDeviceName(), dev))
      return true;
    DEBUGF(INDI::Logger::DBG_SESSION, "Setting number %s", name);
    
  
    /* FFMpeg Controls */
    /*if (!strcmp (FFMpegControlsNP.name, name))
      {      
	FFMpegControlsNP.s = IPS_IDLE;
	
	if (IUUpdateNumber(&FFMpegControlsNP, values, names, n) < 0)
	  return false;


	FFMpegControlsNP.s = IPS_OK;
	IDSetNumber(&FFMpegControlsNP, NULL);
	return true;
      }
    */
    return INDI::CCD::ISNewNumber(dev,name,values,names,n);
}

bool FFMpeg::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
    /* ignore if not ours */
    if (dev && strcmp (getDeviceName(), dev))
      return true;

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting switch %s", name);
    /* Video Stream */
    // use constant prop name here as VideoStreamSP is not defined on connection !!
    /*
      if (!strcmp(name, "CCD_VIDEO_STREAM")) {
      // called by kstars when closing window AND after disconnect
      IUResetSwitch(VideoStreamSP);
      IUUpdateSwitch(VideoStreamSP, states, names, n);
    
      if (VideoStreamSP->sp[0].s == ISS_ON) {
	if ((!is_streaming)) { 
	  VideoStreamSP->s  = IPS_BUSY; 
	  DEBUG(INDI::Logger::DBG_SESSION, "Starting the video stream.");
	  start_streaming();
	}
      } else {
	VideoStreamSP->s = IPS_IDLE;       
	if (is_streaming) {
	  DEBUG(INDI::Logger::DBG_SESSION, "The video stream has been disabled.");
	  stop_streaming();
	}
      }
      IDSetSwitch(VideoStreamSP, NULL);
      return true;
    }
    */

    return INDI::CCD::ISNewSwitch(dev,name,states,names,n);
}

bool FFMpeg::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
    IText *tp;
  
    /* ignore if not ours */
    if (dev && strcmp (getDeviceName(), dev))
      return true;
    
    if (!strcmp(name, CaptureDeviceTP.name) )
      {
	CaptureDeviceTP.s = IPS_OK;
	tp = IUFindText( &CaptureDeviceTP, names[0] );	  
	if (!tp)
	  return false;
	IUSaveText(tp, texts[0]);
	tp = IUFindText( &CaptureDeviceTP, names[1] );	  
	if (!tp)
	  return false;
	IUSaveText(tp, texts[1]);	
	IDSetText (&CaptureDeviceTP, NULL);
	return true;
      }

    return INDI::CCD::ISNewText(dev,name,texts,names,n);
}

bool FFMpeg::StartStreaming() {
  start_streaming();
  return true;
}

bool FFMpeg::StopStreaming() {
  stop_streaming();
  return true;
}

void FFMpeg::start_capturing()
{
    if (is_capturing) return;
    is_capturing=true;
    if (join_thread) {
      capture_thread.join();
      join_thread=false;
    }
    capture_thread=std::thread(RunCaptureThread, this);
}

void FFMpeg::stop_capturing()
{
    if (!is_capturing) return;
    is_capturing=false;
    if (std::this_thread::get_id() != capture_thread.get_id())
      capture_thread.join();
    else
      join_thread=true;
}

void FFMpeg::start_streaming()
{
    if (is_streaming) return;
    if (!is_capturing) start_capturing();
    is_streaming=true;
}

void FFMpeg::stop_streaming()
{
    if (!is_streaming) return;
    stop_capturing();
    is_streaming=false;
}


void FFMpeg::RunCaptureThread(FFMpeg *ffmpeg)
{
  fprintf(stderr, "Start RunCaptureThread\n");
  ffmpeg->run_capture();
  fprintf(stderr, "End RunCaptureThread\n");
}


void FFMpeg::run_capture()
{
  uint8_t *buffer = NULL;
  int numBytes;
  struct SwsContext *sws_ctx = NULL;
  int frameFinished;
  AVPacket packet;
  
  // Allocate video frame
  pFrame=av_frame_alloc();
  // Allocate an AVFrame structure
  pFrameRGB=av_frame_alloc();
  if(pFrameRGB==NULL)
    return ;
  // Determine required buffer size and allocate buffer
  //numBytes=avpicture_get_size(PIX_FMT_RGBA, pCodecCtx->width,
  //                          pCodecCtx->height);
  //numBytes=av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width,
  //    pCodecCtx->height, 1);
  numBytes=av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width,
				    pCodecCtx->height, 1);
  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  //avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGBA,
  //		 pCodecCtx->width, pCodecCtx->height);
  av_image_fill_arrays (pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGBA,
  			pCodecCtx->width, pCodecCtx->height, 1);
  //av_image_fill_arrays (pFrameRGB->data, pFrameRGB->linesize, buffer, pCodecCtx->pix_fmt,
  //		pCodecCtx->width, pCodecCtx->height, 1);

  //streamer->setPixelFormat(pCodecCtx->pix_fmt);
  //streamer->setRecorderSize(w,h);


  // initialize SWS context for software scaling
  /*sws_ctx = sws_getContext(pCodecCtx->width,
			   pCodecCtx->height,
			   pCodecCtx->pix_fmt,
			   pCodecCtx->width,
			   pCodecCtx->height,
			   AV_PIX_FMT_RGBA,
			   SWS_BILINEAR,
			   NULL,
			   NULL,
			   NULL
			   );
  */
  ISwitch *compress=IUFindSwitch(getSwitch("CCD_COMPRESSION"), "CCD_COMPRESS");

  PrimaryCCD.setFrameBufferSize(numBytes);
  PrimaryCCD.setFrame(0, 0, pCodecCtx->width, pCodecCtx->height);
  PrimaryCCD.setResolution(pCodecCtx->width, pCodecCtx->height);

  PrimaryCCD.setBPP(8);

  PrimaryCCD.setNAxis(2);

  //PrimaryCCD.setNAxis(3);

  while (is_capturing) {

    if (av_read_frame(pFormatCtx, &packet)<0) { // stream stopped
      DEBUG(INDI::Logger::DBG_SESSION, "Device has been disconnected.");
      is_capturing=false; is_streaming=false; 
      // TODO close nicely
      break;
    }
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
    
      // Did we get a video frame?
      if(frameFinished) {
	// Convert the image from its native format to RGB
        /*sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
		  pFrame->linesize, 0, pCodecCtx->height,
		  pFrameRGB->data, pFrameRGB->linesize);
	*/
	if (is_streaming) {
	  uLong totalBytes=0;
	  uLongf compressedBytes=0;
	  int r;
    
	  // use indi streamer class
	  streamer->newFrame((uint8_t *)pFrame->data[0]);
	  // totalBytes=numBytes;
	  // if (compress->s == ISS_ON) {
	  //   compressedFrame = (unsigned char *) realloc (compressedFrame, sizeof(unsigned char) * totalBytes + totalBytes / 64 + 16 + 3);
	  //   compressedBytes = sizeof(unsigned char) * totalBytes + totalBytes / 64 + 16 + 3;
	  //   //r = compress2(compressedFrame, &compressedBytes, pFrameRGB->data[0], totalBytes, 4);
	  //   r = compress2(compressedFrame, &compressedBytes, pFrame->data[0], totalBytes, 4);
	  //   if (r != Z_OK) {
	  //     /* this should NEVER happen */
	  //     DEBUGF(INDI::Logger::DBG_WARNING,"internal error - compression failed: %d\n", r);
	  //     //return;
	  //   }
	  //   /* Send it compressed */
	  //   StreamFrame->blob = compressedFrame;
	  //   StreamFrame->bloblen = compressedBytes;
	  //   StreamFrame->size = totalBytes;
	  //   strcpy(StreamFrame->format, ".stream.z");
	    
	  // } else {
	  //   //StreamFrame->blob = pFrameRGB->data[0];
	  //   StreamFrame->blob = pFrame->data[0];
	  //   StreamFrame->bloblen = totalBytes;
	  //   StreamFrame->size = totalBytes;
	  //   strcpy(StreamFrame->format, ".stream");
	  // }
	  StreamFrameBP->s = IPS_OK;
	  IDSetBLOB (StreamFrameBP, NULL);
	} // end streaming
      } // end frameFinished
    } // packet videoStream
    //av_free_packet(&packet);
    av_packet_unref(&packet);    
  } // end is_capturing
  
  // Free the RGB image
  av_free(buffer);
  av_free(pFrameRGB);

  // Free the YUV frame
  av_free(pFrame);

  DEBUG(INDI::Logger::DBG_SESSION,"Capture thread releasing device.");
}


