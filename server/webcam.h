#ifndef	__WEBCAM_H__
	#define __WEBCAM_H__
	
#include "utils.h"

#define DRV_OV511    "ov519"
#define DRV_ZC301    "zc3xx"

extern int camera_init(const char *dev, 
                       __u32 width, 
					   __u32 height, 
					   __u32 pixfmt);
extern int camera_restart(const char *dev, 
                          __u32 width, 
						  __u32 height, 
						  __u32 pixfmt);
extern int camera_stop(void);
extern void* camera_run(void* args);
extern int camera_take_picture(void);

	
#endif	//__WEBCAM_H__
