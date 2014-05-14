/*
 * Video capture module utilising the GStreamer pipeline for AR Toolkit
 * 
 * (c) Copyrights 2003-2008 Hartmut Seichter <http://www.technotecture.com>
 * 
 * licensed under the terms of the LGPL v2
 *
 */

/* include AR Toolkit*/ 
#include <AR/config.h>
#include <AR/ar.h>
#include <AR/video.h>

/* include GLib for GStreamer */
#include <glib.h>

/* include GStreamer itself */
#include <gst/gst.h>

/* using memcpy */
#include <string.h>


#define GSTREAMER_TEST_LAUNCH_CFG "videotestsrc ! capsfilter caps=video/x-raw-rgb,bpp=24 ! identity name=artoolkit ! fakesink"


struct _AR2VideoParamT {
	
	/* size of the image */
	int	width, height;

	/* the actual video buffer */
    ARUint8             *videoBuffer;

	/* GStreamer pipeline */
	GstElement *pipeline;
	
	/* GStreamer identity needed for probing */
	GstElement *probe;

};


static AR2VideoParamT *gVid = NULL;

static gboolean
cb_have_data (GstPad    *pad,
	      GstBuffer *buffer,
	      gpointer   u_data)
{

 	const GstCaps *caps;
	GstStructure *str;
	
	gint width,height;
	gdouble rate;
	
	AR2VideoParamT *vid = (AR2VideoParamT*)u_data;

	if (vid == NULL) return FALSE;
	

	/* only do initialy for the buffer */
	if (vid->videoBuffer == NULL && buffer) 
	{
		g_print("libARvideo error! Buffer not allocated\n");		
	}

	if (vid->videoBuffer)
	{
		memcpy(vid->videoBuffer, buffer->data, buffer->size);		
	}
	
	return TRUE;
}


static
void video_caps_notify(GObject* obj, GParamSpec* pspec, gpointer data) {

	const GstCaps *caps;
	GstStructure *str;
	
	gint width,height;
	gdouble rate;
	
	AR2VideoParamT *vid = (AR2VideoParamT*)data;

	caps = gst_pad_get_negotiated_caps((GstPad*)obj);

	if (caps) {

		str=gst_caps_get_structure(caps,0);

		/* Get some data about the frame */
		gst_structure_get_int(str,"width",&width);
		gst_structure_get_int(str,"height",&height);
		gst_structure_get_double(str,"framerate",&rate);
		
		g_print("libARvideo: GStreamer negotiated %dx%d @%3.3fps\n", width, height,rate);

		vid->width = width;
		vid->height = height;

		g_print("libARvideo: allocating %d bytes\n",(vid->width * vid->height * AR_PIX_SIZE_DEFAULT));

		/* allocate the buffer */	
		arMalloc(vid->videoBuffer, ARUint8, (vid->width * vid->height * AR_PIX_SIZE_DEFAULT) );



	}
}


int
arVideoOpen( char *config ) {
   if( gVid != NULL ) {
        printf("Device has been opened!!\n");
        return -1;
    }
    gVid = ar2VideoOpen( config );
    if( gVid == NULL ) return -1;
}

int 
arVideoClose( void )
{
	return ar2VideoClose(gVid);
}

int
arVideoDispOption( void )
{
   return 0;
}

int
arVideoInqSize( int *x, int *y ) {
	
	ar2VideoInqSize(gVid,x,y);

	return 0;
}

ARUint8
*arVideoGetImage( void )
{
   return ar2VideoGetImage(gVid);  // address of your image data
}

int 
arVideoCapStart( void ) {

	ar2VideoCapStart(gVid);
	return 0;
}

int 
arVideoCapStop( void )
{
	ar2VideoCapStop(gVid);
	return 0;
}

int arVideoCapNext( void )
{
	return ar2VideoCapNext(gVid);;
}

/*---------------------------------------------------------------------------*/

AR2VideoParamT* 
ar2VideoOpen(char *config_in ) {

	AR2VideoParamT *vid = 0;
	GError *error = 0;
	int i;
	GstPad *pad, *peerpad;
	GstXML *xml;
	GstStateChangeReturn _ret;
	int is_live;
	char *config;

	/* If no config string is supplied, we should use the environment variable, otherwise set a sane default */
	if (!config_in || !(config_in[0])) {
		/* None suppplied, lets see if the user supplied one from the shell */
		char *envconf = getenv ("ARTOOLKIT_CONFIG");
		if (envconf && envconf[0]) {
			config = envconf;
			g_printf ("Using config string from environment [%s].\n", envconf);
		} else {
			config = NULL;

			g_printf ("Warning: no video config string supplied, using default!.\n");

			/* setting up defaults - we fall back to the TV test signal simulator */
			config = GSTREAMER_TEST_LAUNCH_CFG;					
				
		}

	} else {
		config = config_in;
		g_printf ("Using supplied video config string [%s].\n", config_in);
	}

	/* initialise GStreamer */
	gst_init(0,0);	
	
	/* init ART structure */
    arMalloc( vid, AR2VideoParamT, 1 );

	/* initialise buffer */
	vid->videoBuffer = NULL;
	
	/* report the current version and features */
	g_print ("libARvideo: %s\n", gst_version_string());

#if 0	
	xml = gst_xml_new();
	
	/* first check if config contains an xml file */
	if (gst_xml_parse_file(xml,config,NULL)) 
	{
		/* parse the pipe definition */
		
	} else 
	{
		vid->pipeline = gst_xml_get_element(xml,"pipeline");
	}
	
#endif

	vid->pipeline = gst_parse_launch (config, &error);
	
	if (!vid->pipeline) {
		g_print ("Parse error: %s\n", error->message);
		return 0;
	};

	/* get the video sink */
	vid->probe = gst_bin_get_by_name(GST_BIN(vid->pipeline), "artoolkit");

	if (!vid->probe) {
		g_print("Pipeline has no element named 'artoolkit'!\n");
		return 0;	
	};

	/* get the pad from the probe (the source pad seems to be more flexible) */	
	pad = gst_element_get_pad (vid->probe, "src");

	/* get the peerpad aka sink */
	peerpad = gst_pad_get_peer(pad);

	/* install the probe callback for capturing */	
	gst_pad_add_buffer_probe (pad, G_CALLBACK (cb_have_data), vid);	

	g_signal_connect(pad, "notify::caps", G_CALLBACK(video_caps_notify), vid);

	/* Needed to fill the information for ARVidInfo */
	gst_element_set_state (vid->pipeline, GST_STATE_READY);

	/* wait until it's up and running or failed */
	if (gst_element_get_state (vid->pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
    	g_error ("libARvideo: failed to put GStreamer into READY state!\n");
    } else {

		is_live = (_ret == GST_STATE_CHANGE_NO_PREROLL) ? 1 : 0;			
    	g_print ("libARvideo: GStreamer pipeline is READY!\n");
    }

	/* Needed to fill the information for ARVidInfo */
	_ret = gst_element_set_state (vid->pipeline, GST_STATE_PAUSED);

	is_live = (_ret == GST_STATE_CHANGE_NO_PREROLL) ? 1 : 0;

	/* wait until it's up and running or failed */
	if (gst_element_get_state (vid->pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {		
    	g_error ("libARvideo: failed to put GStreamer into PAUSED state!\n");
    } else {
    	g_print ("libARvideo: GStreamer pipeline is PAUSED!\n",is_live);
    }
	
	/* dismiss the pad */
	gst_object_unref (pad);

	/* dismiss the peer-pad */
	gst_object_unref (peerpad);

	/* now preroll for live sources */
	if (is_live) {

		g_print ("libARvdeo: need special prerolling for live sources\n"); 

		/* set playing state of the pipeline */
		gst_element_set_state (vid->pipeline, GST_STATE_PLAYING);
		
		/* wait until it's up and running or failed */
		if (gst_element_get_state (vid->pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
	    	g_error ("libARvideo: failed to put GStreamer into PLAYING state!\n");
	    } else {
	    	g_print ("libARvideo: GStreamer pipeline is PLAYING!\n");
	    }
		
		/* set playing state of the pipeline */
		gst_element_set_state (vid->pipeline, GST_STATE_PAUSED);
		
		/* wait until it's up and running or failed */
		if (gst_element_get_state (vid->pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
	    	g_error ("libARvideo: failed to put GStreamer into PAUSED state!\n");
	    } else {
	    	g_print ("libARvideo: GStreamer pipeline is PAUSED!\n");
	    }
	}
		
#if 0
	/* write the bin to stdout */
	gst_xml_write_file (GST_ELEMENT (vid->pipeline), stdout);
#endif
	
	/* return the video handle */
	return vid;
};


int 
ar2VideoClose(AR2VideoParamT *vid) {

	/* stop the pipeline */
	gst_element_set_state (vid->pipeline, GST_STATE_NULL);
	
	/* free the pipeline handle */
	gst_object_unref (GST_OBJECT (vid->pipeline));

	return 0;
}


ARUint8* 
ar2VideoGetImage(AR2VideoParamT *vid) {
	/* just return the bare video buffer */
	return vid->videoBuffer;
}

int 
ar2VideoCapStart(AR2VideoParamT *vid) 
{
	GstStateChangeReturn _ret;

	/* set playing state of the pipeline */
	_ret = gst_element_set_state (vid->pipeline, GST_STATE_PLAYING);

	if (_ret == GST_STATE_CHANGE_ASYNC) 
	{

		/* wait until it's up and running or failed */
		if (gst_element_get_state (vid->pipeline, 
				NULL, NULL, GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE) 
		{
    		g_error ("libARvideo: failed to put GStreamer into PLAYING state!\n");    	
    		return 0;
  
        } else {
			g_print ("libARvideo: GStreamer pipeline is PLAYING!\n");
		} 
	}
	return 1; 
}

int 
ar2VideoCapStop(AR2VideoParamT *vid) {
	/* stop pipeline */
	return gst_element_set_state (vid->pipeline, GST_STATE_PAUSED);
}

int 
ar2VideoCapNext(AR2VideoParamT *vid)
{
	return 0;
}

int
ar2VideoInqSize(AR2VideoParamT *vid, int *x, int *y ) 
{

   *x = vid->width; // width of your static image
   *y = vid->height; // height of your static image

}

