#include "stdinc.h"
#include "utils.h"
#include "v4l2.h"

#include "webcam.h"

extern int cbk_get_cur_frame(void *ptr, __u32 siz, void *args);

extern pthread_mutex_t	g_cam_mutex;
extern pthread_cond_t 	g_cam_cond;

v4l2_device_t g_vd; 

bool g_bcamrun = TRUE;    //是否正在采集标识
bool g_bcamoff = TRUE;    //是否关闭设备标识

//resolution of zc301	
struct rect zc301_res[]={
	320, 240, 	//0 -- default
	640, 480, 	//1
	0, 0
};	

struct rect ov511_res[]={
	320, 240, 	//0 -- default
	640, 480, 	//1
	0, 0
};
/*
 * 初始化摄像头
 * global var: g_vd、g_bcamoff
 * */
int camera_init(const char *dev, __u32 width, __u32 height, __u32 pixfmt)
{
	if (-1 == v4l2_open(dev, &g_vd)) 
		ERR_PRINT_RET(-1);

	if (-1 == v4l2_init(&g_vd, width, height, pixfmt)) 
		ERR_PRINT_RET(-1);	

	g_bcamoff = FALSE;
	
	return 0;
}

/*
 * 重启摄像头
 * global var: g_vd
 * */
int camera_restart(const char *dev, __u32 width, __u32 height, __u32 pixfmt)
{	
	g_bcamrun = FALSE;
	pthread_mutex_lock(&g_cam_mutex);
	
	if (-1 == v4l2_capture_stop(&g_vd)) 
		ERR_PRINT_RET(-1);		
		
	if (-1 == v4l2_uninit(&g_vd)) 
		ERR_PRINT_RET(-1);		
		
	if (-1 == v4l2_close(&g_vd)) 
		ERR_PRINT_RET(-1);		
		
	if (-1 == camera_init(dev, width, height, pixfmt)) 
		ERR_PRINT_RET(-1);			

	if (-1 == v4l2_capture_start(&g_vd)) 
		ERR_PRINT_RET(-1);		
	
	g_bcamrun = TRUE;
	pthread_cond_signal(&g_cam_cond);
	pthread_mutex_unlock(&g_cam_mutex);
	
	return 0;
}

/*
 * 关闭摄像头
 * global var: g_vd、g_bcamoff、g_bcamrun
 * */
int camera_stop(void)
{	
	g_bcamrun = FALSE;
	pthread_mutex_lock(&g_cam_mutex);
	pthread_cond_signal(&g_cam_cond);	
	
	if (-1 == v4l2_capture_stop(&g_vd)) 
		ERR_PRINT_RET(-1);		
		
	if (-1 == v4l2_uninit(&g_vd)) 
		ERR_PRINT_RET(-1);		
		
	if (-1 == v4l2_close(&g_vd)) 
		ERR_PRINT_RET(-1);			
	
	pthread_mutex_unlock(&g_cam_mutex);
	g_bcamoff = TRUE;
	
    DBG("Cam stop return!\n");
	return 0;
}

/*
 * 视频grab(capture)子进程
 * global var: gVSt、g_vd、g_cam_mutex、g_bcamoff
 *             gwin_draw_area、g_bcamrun
 * */
void* camera_run(void* args)
{
	v4l2_capture_setup(&g_vd, cbk_get_cur_frame, NULL);

	if (-1 == v4l2_capture_start(&g_vd)) 
		g_bcamoff = TRUE;		
	
	while (!g_bcamoff) {
		if (g_bcamrun) {	
			pthread_mutex_lock(&g_cam_mutex);
			if (g_bcamrun) {
				if (-1==v4l2_capture_process(&g_vd)) 
					break;
			}
			pthread_mutex_unlock(&g_cam_mutex);	
		} else {
			pthread_mutex_lock(&g_cam_mutex);
			while (!g_bcamrun) 
				pthread_cond_wait(&g_cam_cond, &g_cam_mutex);
			pthread_mutex_unlock(&g_cam_mutex);
		}
	}
    DBG("Cam run return!\n");
}





