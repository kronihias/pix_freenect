/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Adaptive threshold object

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

// -----------------------
//	pix_freenect
//	2011 by Matthias Kronlachner
// -----------------------

#ifndef INCLUDE_pix_freenect_H_
#define INCLUDE_pix_freenect_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <cmath>
#include <vector>


#include "libfreenect.h"
#ifndef __APPLE__
	#include "libfreenect-audio.h"
#endif

#include "Base/GemBase.h"
#include "Gem/Properties.h"
#include "Gem/Image.h"
#include "Gem/State.h"

#include "Base/GemPixDualObj.h"

#include <pthread.h>


/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_freenect
    

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_freenect : public GemBase
{
    CPPEXTERN_HEADER(pix_freenect, GemBase);

    public:

	    //////////
	    // Constructor
    	//pix_freenect(t_float kinect_device_id, t_float rgb_on, t_float depth_on, t_float audio_on);
    	pix_freenect(int argc, t_atom *argv);
		
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_freenect();

			virtual void	startRendering();
    	//////////
    	// Rendering 	
			virtual void 	render(GemState *state);
			
			virtual void 	postrender(GemState *state);
		
		  // Stop Transfer
			virtual void	stopRendering();
    	
	//////////
    	// Settings/Info
			void				floatResolutionMess(float resolution);
    	void				floatVideoModeMess(float videomode);
			void				floatDepthModeMess(float depthmode);
    	void	    	floatAngleMess(float angle);
    	void	    	floatLedMess(float led);
    	void	    	bangMess();
			void	    	infoMess();
    	void	    	accelMess();
			void				renderDepth(int argc, t_atom*argv);
			void				audioOutput();
			bool 				startRGB();
			bool 				stopRGB();
			bool 				startDepth();
			bool 				stopDepth();
			bool 				startAudio();
			bool 				stopAudio();	
			
			static void* freenect_thread_func(void*);
			static void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp);
			static void rgb_cb(freenect_device *dev, void *rgb, uint32_t timestamp);
		
		  static void in_callback(freenect_device* dev, int num_samples,
                 int32_t* mic1, int32_t* mic2,
                 int32_t* mic3, int32_t* mic4,
                 int16_t* cancelled, void *unknown);
                 
    	// Settings
      int x_angle;
      int x_led;
      
      bool rgb_started;
      bool depth_started;
      bool audio_started;
      
      bool rgb_wanted;
      bool depth_wanted;
      bool audio_wanted;
      
			int got_rgb;
			int got_depth;

			bool destroy_thread; // shutdown...
			        
      uint16_t t_gamma[10000];
			uint16_t t_gamma2[10000];
      
      uint16_t *depth_mid,  *depth_front;
			uint8_t *rgb_back, *rgb_mid, *rgb_front;

			pthread_cond_t  *gl_frame_cond;
			pthread_mutex_t *gl_backbuf_mutex;

      int kinect_dev_nr;
      	
			freenect_context *f_ctx;
			freenect_device *f_dev;	
  
			freenect_video_format rgb_format;
			freenect_video_format req_rgb_format;
			
			freenect_depth_format depth_format;
			freenect_depth_format req_depth_format;
			
			freenect_resolution freenect_res;
			freenect_resolution req_freenect_res;
			
			int	depth_output;
			int	req_depth_output;
			
			int 		rgb_width;
			int			rgb_height;
			int 		depth_width;
			int			depth_height;
			
			bool      m_rendering; // "true" when rendering is on, false otherwise
  		
			bool  rgb_reallocate; // reallocate memory after rgb resolution changed
    	//////////
    	// The pixBlock with the current image
    	pixBlock    	m_image;
    	pixBlock    	m_depth;
    	
    	GemState					*depth_state;
    	
    	pthread_mutex_t   *audio_mutex; /* mutex to lock buffers */
			
			float*x_buffer1;        /* audio buffers */
			float*x_buffer2;
			float*x_buffer3;
			float*x_buffer4;
			unsigned int x_bufsize; /* length of the buffer */
			unsigned int x_freenect_pos; /* buffer pos for callback */
			unsigned int x_num_samples; /* how many "active" samples in buffer */
			
    private:
    
    	//////////
    	// Static member functions
			static void			floatResolutionMessCallback(void *data, float resolution);
    	static void			floatVideoModeMessCallback(void *data, float videomode);
			static void			floatDepthModeMessCallback(void *data, float depthmode);
    	static void    	floatAngleMessCallback(void *data, float angle);
    	static void    	floatLedMessCallback(void *data, float led);
    	static void    	bangMessCallback(void *data);
    	static void    	accelMessCallback(void *data);
    	static void    	infoMessCallback(void *data);

    	static void    	floatRgbMessCallback(void *data, float rgb);
    	static void    	floatDepthMessCallback(void *data, float depth);
    	static void    	floatAudioMessCallback(void *data, float audio);
    	
    	static void    	floatDepthOutputMessCallback(void *data, float depth_output);
    	
    	static void    	renderDepthCallback(void *data, t_symbol*s, int argc, t_atom*argv);
    	
			pthread_t freenect_thread;
 
			t_outlet        *m_infooutlet;
			t_outlet        *m_mic1_outlet;
			t_outlet        *m_mic2_outlet;
			t_outlet        *m_mic3_outlet;
			t_outlet        *m_mic4_outlet;
			t_outlet        *m_depthoutlet; 
			t_inlet         *m_depthinlet; 
};

#endif	// for header file
