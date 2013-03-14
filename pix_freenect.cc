	////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-2000 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM
//    Copyright (c) 2002 James Tittle & Chris Clepper
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

// -----------------------
//	pix_freenect
//	2011/12 by Matthias Kronlachner
// -----------------------

#include "pix_freenect.h"
#include "Gem/State.h"
#include "Gem/Exception.h"

CPPEXTERN_NEW_WITH_GIMME(pix_freenect);

//CPPEXTERN_NEW(pix_freenect)

/////////////////////////////////////////////////////////
//
// pix_freenect
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

freenect_context *f_ctx;
freenect_device *f_dev;

// 1: kinect_id/serial, rgb_on, depth_on
pix_freenect :: pix_freenect(int argc, t_atom *argv)
{ 
	post("pix_freenect 0.10 - 2011/12 by Matthias Kronlachner");
  rgb_width=640;
  rgb_height=480;
  depth_width=640;
  depth_height=480;
  x_angle = 0;
  x_led = 1;
  
	int rgb_on = 0;
	int depth_on = 0;
	int kinect_dev_nr = 0;
	t_symbol *serial;
	bool openBySerial=false;
	
	if (argc >= 1)
	{
		const char* test = "float";
		serial = atom_getsymbol(&argv[0]);
		
		if (!strncmp(serial->s_name,"float", 5))
		{
			kinect_dev_nr = (int)atom_getint(&argv[0]);
			openBySerial=false;
		} else {
			//post ("test: %s", (char*)serial->s_name);
			openBySerial=true;
		}
	}
	if (argc >= 2)
	{
		if (atom_getint(&argv[1]) != 0)
		{
			rgb_on = 1;
		}
	}
	if (argc >= 3)
	{
		if (atom_getint(&argv[2]) != 0)
		{
			depth_on = 1;
		}
	}
	

  
  if (freenect_init(&f_ctx, NULL) < 0) {
		throw(GemException("freenect_init() failed\n"));
  }else{
  	//post("freenect initiated\n");
  }

	freenect_set_log_level(f_ctx, FREENECT_LOG_ERROR); // LOG LEVEL
	freenect_select_subdevices(f_ctx, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));
	
	
  //int nr_devices = freenect_num_devices (f_ctx);
  freenect_device_attributes * devAttrib;
	int nr_devices =freenect_list_device_attributes(f_ctx, &devAttrib);
  post ("Number of devices found: %d", nr_devices);

	// display serial numbers
	const char* id;
	for(int i = 0; i < nr_devices; i++){
		id = devAttrib->camera_serial;
		devAttrib = devAttrib->next;
		post ("Device %d serial: %s", i, id);
	}
	
	// OPEN KINECT BY ID
	if (!openBySerial)
	{
	verbose(1, "trying to open Kinect device nr %i...", (int)kinect_dev_nr);
  if (freenect_open_device(f_ctx, &f_dev, kinect_dev_nr) < 0) {
  	throw(GemException("Could not open device! \n"));
  } else
		post("Kinect Nr %d opened", kinect_dev_nr);
	}
	
	// OPEN KINECT BY SERIAL
	if (openBySerial)
	{
	post("trying to open Kinect with serial %s...", (char*)serial->s_name);
  if (freenect_open_device_by_camera_serial(f_ctx, &f_dev, (char*)serial->s_name) < 0) {
  	throw(GemException("Could not open device! \n"));
  } else
		post("Kinect with serial %s opened!", (char*)serial->s_name);
	}
	// SET USER DATA FOR CALLBACKS
  freenect_set_user(f_dev, this);
	
	
	int res = pthread_create(&freenect_thread, NULL, freenect_thread_func, this);
	if (res) {
		throw(GemException("pthread_create failed\n"));
	}
	

 //create mutex  
  gl_backbuf_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  if(gl_backbuf_mutex){
    if ( pthread_mutex_init(gl_backbuf_mutex, NULL) < 0 ) {
      free(gl_backbuf_mutex);
      gl_backbuf_mutex=NULL;
    } 
  }

  gl_frame_cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(gl_frame_cond, NULL);
	
	// STARTUP FREENECT MODES
  rgb_format = FREENECT_VIDEO_RGB;
  req_rgb_format = FREENECT_VIDEO_RGB; //FREENECT_VIDEO_RGB
	depth_format = FREENECT_DEPTH_MM;
	req_depth_format= FREENECT_DEPTH_MM; //FREENECT_DEPTH_11BIT

	freenect_res = FREENECT_RESOLUTION_MEDIUM;
	req_freenect_res = FREENECT_RESOLUTION_MEDIUM;
	
  depth_mid = (uint16_t*)malloc(640*480*2);
  depth_front = (uint16_t*)malloc(640*480*2);

  rgb_back = (uint8_t*)malloc(1280*1024*3);
  rgb_mid = (uint8_t*)malloc(1280*1024*3);
  rgb_front = (uint8_t*)malloc(1280*1024*4);

  got_rgb = 0;
  got_depth = 0;
	
	rgb_started = 0;
	depth_started = 0;
	
	depth_output = 0;
	req_depth_output = 0;

	rgb_reallocate = false;
	
	destroy_thread = false;
	
	if (rgb_on)
	{
		rgb_wanted = true;
	} else {
		rgb_wanted = false;
	}
	
	if (depth_on)
	{
		depth_wanted = true;
	} else {
		depth_wanted = false;
	}
	
  
  m_depthoutlet = outlet_new(this->x_obj, 0);
  
  m_infooutlet  = outlet_new(this->x_obj, 0);
  
	m_depthinlet  = inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("gem_state"), gensym("depth_state"));
 
  m_rendering = false;
}



void *pix_freenect::freenect_thread_func(void*target)
{
	pix_freenect *me = (pix_freenect*) target;

	freenect_set_led(me->f_dev,LED_BLINK_GREEN );
	freenect_set_depth_callback(me->f_dev, me->depth_cb);
	freenect_set_video_callback(me->f_dev, me->rgb_cb);
	freenect_set_video_mode(me->f_dev, freenect_find_video_mode(me->freenect_res, me->rgb_format));
	freenect_set_depth_mode(me->f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, me->depth_format));
	freenect_set_video_buffer(me->f_dev, me->rgb_back);
  //freenect_set_depth_buffer(me->f_dev, depth_back);
  
	int accelCount = 0;
	int status = 0;
	
  // define timeout 2 sec
  struct timeval timeout;
  timeout.tv_sec = 2;
  
  while (!me->destroy_thread) {
	//me->post ("thread start");
		//status = freenect_process_events(me->f_ctx);
    	status = freenect_process_events_timeout(me->f_ctx,&timeout);
    
		// Start/Stop Streams if user changed request or started Rendering
		if (me->m_rendering)
		{
			if (me->rgb_wanted && !me->rgb_started)
			{
				me->startRGB();
			}
			if (!me->rgb_wanted && me->rgb_started)
			{
				me->stopRGB();
			}
			
			if (me->depth_wanted && !me->depth_started)
			{
				me->startDepth();
			}
			if (!me->depth_wanted && me->depth_started)
			{
				me->stopDepth();
			}
			
		// check if rgb format changed
		
			if ((me->req_rgb_format != me->rgb_format) || (me->req_freenect_res != me->freenect_res)) {
				if (me->rgb_started)
				{
					me->stopRGB();
				}
				freenect_set_video_mode(me->f_dev, freenect_find_video_mode(me->req_freenect_res, me->req_rgb_format));
				if (me->rgb_started)
				{
					me->startRGB();
				}
				me->rgb_format = me->req_rgb_format;
				me->freenect_res = me->req_freenect_res;
				me->rgb_reallocate = true;
			}

			// check if depth format changed
			if (me->req_depth_format != me->depth_format) {
				if (me->depth_started)
				{
					me->stopDepth();
					//freenect_stop_depth(me->f_dev);
				}
				freenect_set_depth_mode(me->f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, me->req_depth_format)); // just medium resolution available
				if (me->depth_started)
				{
					me->startDepth();
					//freenect_start_depth(me->f_dev);
				}
				me->depth_format = me->req_depth_format;
			}
		}
	//me->post ("thread end");	
	}
  me->post ("freenect thread killed");
	return 0;
}

bool pix_freenect::startRGB()
{
	int res;
	res = freenect_start_video(f_dev);
  sleep(1);
	if (res == 0)
	{
		post ("RGB started");
		rgb_started=true;
		freenect_update_tilt_state(f_dev); // trick to wake up thread
		return true;
		
	} else {
		post ("Could not start RGB - error code: %i", res);
		return false;
	}
}

bool pix_freenect::stopRGB()
{
	int res;
	res = freenect_stop_video(f_dev);
	if (res == 0)
	{
		post ("RGB stoped");
		rgb_started=false;
		return true;

	} else {
		post ("Could not stop RGB - error code: %i", res);
		return false;
	}
}

bool pix_freenect::startDepth()
{
	int res;
	res = freenect_start_depth(f_dev);
    sleep(1); // wait 1s for depth stream to start streaming - delays startup but is necessary!!
	if (res == 0)
	{
		post ("Depth started");
		freenect_update_tilt_state(f_dev); // trick to wake up thread
		depth_started=true;
		return true;

	} else {
		post ("Could not start Depth - error code: %i", res);
		return false;
	}
}

bool pix_freenect::stopDepth()
{
	int res;
	res = freenect_stop_depth(f_dev);
	if (res == 0)
	{
		post ("Depth stoped");
		depth_started=false;
		return true;
	} else {
		post ("Could not stop Depth - error code: %i", res);
		return false;
	}
}

void pix_freenect::depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
	pix_freenect *me = (pix_freenect*)freenect_get_user(dev);
	
	int i;
	uint16_t *depth = (uint16_t*)v_depth;

	pthread_mutex_lock(me->gl_backbuf_mutex);

	for (i=0; i<640*480; i++) {		
		me->depth_mid[i] = depth[i];
	}

	me->got_depth++;
	pthread_cond_signal(me->gl_frame_cond);
	pthread_mutex_unlock(me->gl_backbuf_mutex);
}

void pix_freenect::rgb_cb(freenect_device *dev, void *rgb, uint32_t timestamp)
{
	pix_freenect *me = (pix_freenect*)freenect_get_user(dev);

	pthread_mutex_lock(me->gl_backbuf_mutex);

	// swap buffers
	//assert (rgb_back == rgb);
	me->rgb_back = me->rgb_mid;
	freenect_set_video_buffer(dev, me->rgb_back);
	me->rgb_mid = (uint8_t*)rgb;

	me->got_rgb=1;
	pthread_cond_signal(me->gl_frame_cond);
	pthread_mutex_unlock(me->gl_backbuf_mutex);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_freenect :: ~pix_freenect()
{ 
	freenect_set_led(f_dev,LED_RED );
	destroy_thread=true;

	pthread_mutex_destroy(gl_backbuf_mutex);
  
  pthread_detach(freenect_thread);

  //pthread_exit(&freenect_thread); // it will hang...?

	if (depth_started)
		freenect_stop_depth(f_dev);
	if (rgb_started)
		freenect_stop_video(f_dev);
	
	freenect_close_device(f_dev);
	freenect_shutdown(f_ctx);
	

  
	free(depth_mid);
	outlet_free(m_infooutlet);
  outlet_free(m_depthoutlet);
  
  inlet_free(m_depthinlet);
  
	post("shutdown kinect nr %i...", kinect_dev_nr);

}


/////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////

void pix_freenect :: startRendering(){
	
  m_image.image.xsize = rgb_width;
  m_image.image.ysize = rgb_height;
  m_image.image.setCsizeByFormat(GL_RGBA);
  m_image.image.reallocate();
  
  m_depth.image.xsize = depth_width;
  m_depth.image.ysize = depth_height;
  if (req_depth_output == 0)
  {
		m_depth.image.setCsizeByFormat(GL_RGBA);
	}
	if (req_depth_output == 1)
  {
		m_depth.image.setCsizeByFormat(GL_YCBCR_422_GEM);
	}

  m_depth.image.reallocate();
  
  if (rgb_wanted && !rgb_started)
			{
				startRGB();
			}
			if (!rgb_wanted && rgb_started)
			{
				stopRGB();
			}
			
			if (depth_wanted && !depth_started)
			{
				startDepth();
			}
			if (!depth_wanted && depth_started)
			{
				stopDepth();
			}

  m_rendering=true;
}

/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
	//static std::vector<uint8_t> depth(640*480*4);
	//static std::vector<uint8_t> rgb(640*480*4);
	
void pix_freenect :: render(GemState *state)
{
	if (!m_rendering)
	{
		startRendering();
	} else 	{

		if (rgb_reallocate)
		{
			switch((int)freenect_res)
			{
				case 0: 
				rgb_width = 320;
				rgb_height = 240;
				break;
				case 1: 
				rgb_width = 640;
				rgb_height = 480;
				break;
				case 2: 
				rgb_width = 1280;
				rgb_height = 1024;
				break;
			}

			m_image.image.xsize = rgb_width;
			m_image.image.ysize = rgb_height;
			if ((int)req_rgb_format == 0)
			{
				m_image.image.setCsizeByFormat(GL_RGBA);
			}
			if ((int)req_rgb_format == 2)
			{
				m_image.image.setCsizeByFormat(GL_LUMINANCE);
			}
			verbose(1, "REALLOCATING....");
			m_image.image.reallocate();
			rgb_reallocate = false;
		}

		if (rgb_wanted && rgb_started) //RGB OUTPUT
		{
			if (got_rgb) // True if new image
			{
        int pixnum = m_image.image.xsize * m_image.image.ysize;
        
        uint8_t *tmp;
        
        // swap buffer
        pthread_mutex_lock(gl_backbuf_mutex);
        tmp = rgb_front;
        rgb_front = rgb_mid;
        rgb_mid = tmp;
        pthread_mutex_unlock(gl_backbuf_mutex);
        
        uint8_t *rgb_pixel = rgb_front;
        unsigned char *pixels=m_image.image.data;
        
				if ((int)rgb_format==0)
				{
					while (pixnum--) {

						pixels[chRed]=rgb_pixel[0];
						pixels[chGreen]=rgb_pixel[1];
						pixels[chBlue]=rgb_pixel[2];
						pixels[chAlpha]=255;

						rgb_pixel+=3;
						pixels+=4;
					}
				} else if ((int)rgb_format==2) { // IR MODE -> greyscale
					m_image.image.data=rgb_pixel;
				}
				
			m_image.newimage = 1;
			m_image.image.notowned = true;
			m_image.image.upsidedown=true;
			state->set(GemState::_PIX, &m_image);
			got_rgb=0;
		} else {
			m_image.newimage = 0;
			state->set(GemState::_PIX, &m_image);
		}
	}

}
}

void pix_freenect :: renderDepth(int argc, t_atom*argv)
{
	if (!m_rendering)
	{
		startRendering();
	} else 	{

		if (argc==2 && argv->a_type==A_POINTER && (argv+1)->a_type==A_POINTER) // is it gem_state?
		{
			depth_state =  (GemState *) (argv+1)->a_w.w_gpointer;

			if (depth_wanted && depth_started) //DEPTH OUTPUT
			{
			// check if depth output request changed -> reallocate image_struct
				if (req_depth_output != depth_output)
				{
					if (req_depth_output == 0)
					{
						m_depth.image.setCsizeByFormat(GL_RGBA);
					}
					if (req_depth_output == 1)
					{
						m_depth.image.setCsizeByFormat(GL_YCBCR_422_GEM);
					}
					m_depth.image.reallocate();
					depth_output=req_depth_output;
				}

				if (got_depth)
				{
					pthread_mutex_lock(gl_backbuf_mutex);
					//uint8_t *tmp;
					//tmp = depth_front;
					depth_front = depth_mid;
					//depth_mid = tmp;

					pthread_mutex_unlock(gl_backbuf_mutex);

					if (depth_output == 0) // RAW RGBA
					{
						uint8_t *pixels = m_depth.image.data;

						uint16_t *depth_pixel = (uint16_t*)depth_front;

						for(int y = 0; y < (640*480); y++) {
							// RGBA
							pixels[chRed]=(uint8_t)(*depth_pixel >> 8);
							pixels[chGreen]=(uint8_t)(*depth_pixel & 0xff);
							pixels[chBlue]=0;
							pixels[chAlpha]=255;
							pixels+=4;
							depth_pixel++;
						}
					}

					if (depth_output == 1) // RAW YUV
					{
						m_depth.image.data= (unsigned char*)&depth_front[0];
					}

					m_depth.newimage = 1;
					m_depth.image.notowned = true;
					m_depth.image.upsidedown=true;
					depth_state->set(GemState::_PIX, &m_depth);
					got_depth=0;

					t_atom ap[2];
					ap->a_type=A_POINTER;
					ap->a_w.w_gpointer=(t_gpointer *)m_cache;  // the cache ?
					(ap+1)->a_type=A_POINTER;
					(ap+1)->a_w.w_gpointer=(t_gpointer *)depth_state;
					outlet_anything(m_depthoutlet, gensym("gem_state"), 2, ap);

				} else {
					m_depth.newimage = 0;
					depth_state->set(GemState::_PIX, &m_depth);
					t_atom ap[2];
					ap->a_type=A_POINTER;
					ap->a_w.w_gpointer=(t_gpointer *)m_cache;  // the cache ?
					(ap+1)->a_type=A_POINTER;
					(ap+1)->a_w.w_gpointer=(t_gpointer *)depth_state;
					outlet_anything(m_depthoutlet, gensym("gem_state"), 2, ap);
					return;
				}
			}

		}
	}
}
	
///////////////////////////////////////
// POSTRENDERING -> Clear
///////////////////////////////////////

void pix_freenect :: postrender(GemState *state)
{
  m_image.newimage = 0;
  state->set(GemState::_PIX, static_cast<pixBlock*>(NULL));

}

///////////////////////////////////////
// STOPRENDERING -> Stop Transfer
///////////////////////////////////////

void pix_freenect :: stopRendering(){
	m_rendering=false;
	
	if(rgb_started) 
		stopRGB();
	if(depth_started)
		stopDepth();

}

//////////////////////////////////////////
// Messages - Settings
//////////////////////////////////////////

void pix_freenect :: floatResolutionMess (float resolution)
{
	if ( ( (int)resolution>=0 ) && ( (int)resolution<=2 ) )
	{
		req_freenect_res = (freenect_resolution)(int)resolution;
		switch((int)resolution)
		{
			case 0: post("RGB Resolution set to LOW");
				break;
			case 1: post("RGB Resolution set to MEDIUM");
				break;
			case 2: post("RGB Resolution set to HIGH");
				break;
		}
	} else {
		post("Not Valid Number for resolution! (0-2)");
}
}

void pix_freenect :: floatVideoModeMess (float videomode)
{
	if ( ( (int)videomode>=0 ) && ( (int)videomode<=1 ) ) 
	{
		if ((int)videomode==0) {
			// videomode = 0;
			post("Video mode set to FREENECT_VIDEO_RGB");
		}
		if ((int)videomode==1) {
			videomode = 2;
			post("Video mode set to FREENECT_VIDEO_IR_8BIT");
		}
		req_rgb_format = (freenect_video_format)(int)videomode;
	} else {
		post("Not Valid Number for video_mode! (0-1)");
	}
}

void pix_freenect :: floatDepthModeMess (float depthmode)
{
	if ( ( (int)depthmode>=0 ) && ( (int)depthmode<=2 ) ) 
	{
		if ((int)depthmode==0)
		{
			depthmode = 5; // FREENECT_DEPTH_MM
			post("Depth mode set to FREENECT_DEPTH_MM");
		}
		if ((int)depthmode==1)
		{
			depthmode = 4; // FREENECT_DEPTH_REGISTERED
			post("Depth mode set to FREENECT_DEPTH_REGISTERED");
		}
		if ((int)depthmode==2)
		{
			depthmode = 0; // FREENECT_DEPTH_11BIT
			post("Depth mode set to FREENECT_DEPTH_11BIT");
		}
		req_depth_format = (freenect_depth_format)(int)depthmode;
	} else {
		post("Not Valid Number for depth_mode! (0-2)");
	}
}


void pix_freenect :: floatAngleMess (float angle)
{
  x_angle = (int)angle;
  if ( angle<-30.0 ) x_angle = -30;
  if ( angle>30.0 ) x_angle = 30;
  //post("Angle %d", x_angle);
  freenect_set_tilt_degs(f_dev,angle);
}

void pix_freenect :: floatLedMess (float led)
{
	//post("LED %f", led);

  if ( ( (int)led>=0 ) && ( (int)led<=5 ) ) x_led = (int)led;
  	if (x_led == 1) {
		freenect_set_led(f_dev,LED_GREEN);
	}
	if (x_led == 2) {
		freenect_set_led(f_dev,LED_RED);
	}
	if (x_led == 3) {
		freenect_set_led(f_dev,LED_YELLOW);
	}
	if (x_led == 4) {
		freenect_set_led(f_dev,LED_BLINK_GREEN);
	}
	if (x_led == 5) {
		freenect_set_led(f_dev,LED_BLINK_RED_YELLOW);
	}
	if (x_led == 0) {
		freenect_set_led(f_dev,LED_OFF);
	}
}

void pix_freenect :: accelMess ()
{
	freenect_raw_tilt_state* state;
	freenect_update_tilt_state(f_dev);
	state = freenect_get_tilt_state(f_dev);
	double dx,dy,dz;
	freenect_get_mks_accel(state, &dx, &dy, &dz);
	//post("\r mks acceleration: %4f %4f %4f", dx, dy, dz);
	//post("%d", state->tilt_angle);
	t_atom ap[4];
  t_atom ap2[1];
	
  SETFLOAT(ap+0, dx);
  SETFLOAT(ap+1, dy);
  SETFLOAT(ap+2, dz);

	SETFLOAT(ap2, state->tilt_status);
  outlet_anything(m_infooutlet, gensym("tilt_status"), 1, ap2);

  SETFLOAT(ap2, state->tilt_angle);
  outlet_anything(m_infooutlet, gensym("tilt_angle"), 1, ap2);

  outlet_anything(m_infooutlet, gensym("accel"), 3, ap);
}

void pix_freenect :: infoMess ()
{
	post ("\n::freenect status::");
  freenect_device_attributes * devAttrib;
	int nr_devices = freenect_list_device_attributes(f_ctx, &devAttrib);
  post ("Number of devices found: %d", nr_devices);
	
	// display serial numbers
	const char* id;
	int i = 0;
	for(i=0; i < nr_devices; i++){
		id = devAttrib->camera_serial;
		devAttrib = devAttrib->next;
		post ("Device %d serial: %s", i, id);
	}
	freenect_free_device_attributes(devAttrib);
	
	int ret=freenect_supported_subdevices();
  
	
	if (ret & (1 << 0))
	{
		post ("libfreenect supports FREENECT_DEVICE_MOTOR (%i)", ret);
	}
	if (ret & (1 << 1))
	{
		post ("libfreenect supports FREENECT_DEVICE_CAMERA (%i)", ret);
	}
	if (ret & (1 << 2))
	{
		post ("libfreenect supports FREENECT_DEVICE_AUDIO (%i)", ret);
	}
}
/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_freenect :: obj_setupCallback(t_class *classPtr)
{
	class_addmethod(classPtr, (t_method)&pix_freenect::floatResolutionMessCallback,
  		  gensym("resolution"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::floatVideoModeMessCallback,
  		  gensym("video_mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::floatDepthModeMessCallback,
  		  gensym("depth_mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::floatAngleMessCallback,
  		  gensym("angle"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::floatLedMessCallback,
  		  gensym("led"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::bangMessCallback,
  		  gensym("bang"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::accelMessCallback,
  		  gensym("accel"), A_NULL);

  class_addmethod(classPtr, (t_method)&pix_freenect::floatRgbMessCallback,
  		  gensym("rgb"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_freenect::floatDepthMessCallback,
  		  gensym("depth"), A_FLOAT, A_NULL);
  
  class_addmethod(classPtr, (t_method)&pix_freenect::floatDepthOutputMessCallback,
  		  gensym("depth_output"), A_FLOAT, A_NULL);
  		  
  		   		  
  class_addmethod(classPtr, (t_method)(&pix_freenect::renderDepthCallback),
                  gensym("depth_state"), A_GIMME, A_NULL);
 
	class_addmethod(classPtr, (t_method)(&pix_freenect::infoMessCallback),
									gensym("info"), A_NULL);
 }

void pix_freenect :: floatResolutionMessCallback(void *data, t_floatarg resolution)
{
  GetMyClass(data)->floatResolutionMess((float)resolution);
}

void pix_freenect :: floatVideoModeMessCallback(void *data, t_floatarg videomode)
{
  GetMyClass(data)->floatVideoModeMess((float)videomode);
}

void pix_freenect :: floatDepthModeMessCallback(void *data, t_floatarg depthmode)
{
  GetMyClass(data)->floatDepthModeMess((float)depthmode);
}

void pix_freenect :: floatAngleMessCallback(void *data, t_floatarg angle)
{
  GetMyClass(data)->floatAngleMess((float)angle);
}

void pix_freenect :: floatLedMessCallback(void *data, t_floatarg led)
{
  GetMyClass(data)->floatLedMess((float)led);
}

void pix_freenect :: bangMessCallback(void *data)
{
  
}

void pix_freenect :: accelMessCallback(void *data)
{
  GetMyClass(data)->accelMess();
}

void pix_freenect :: floatRgbMessCallback(void *data, t_floatarg rgb)
{
  pix_freenect *me = (pix_freenect*)GetMyClass(data);
  if ((int)rgb == 0)
		me->rgb_wanted=false;
  if ((int)rgb == 1)
	{
		me->rgb_wanted=true;
		freenect_update_tilt_state(me->f_dev); // trick to wake up thread
	}
		
}

void pix_freenect :: floatDepthMessCallback(void *data, t_floatarg depth)
{
  pix_freenect *me = (pix_freenect*)GetMyClass(data);
  //me->post("daa %i", (int)depth);
  if ((int)depth == 0)
		me->depth_wanted=false;
  if ((int)depth == 1)
	{
		me->depth_wanted=true;
		freenect_update_tilt_state(me->f_dev); // trick to wake up thread
	}
}


void pix_freenect :: floatDepthOutputMessCallback(void *data, t_floatarg depth_output)
{
  pix_freenect *me = (pix_freenect*)GetMyClass(data);
  if ((depth_output >= 0) && (depth_output) <= 1)
		me->req_depth_output=(int)depth_output;
}

void pix_freenect :: renderDepthCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
	GetMyClass(data)->renderDepth(argc, argv);
}

void pix_freenect :: infoMessCallback(void *data)
{
  GetMyClass(data)->infoMess();
}
