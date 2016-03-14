#ifndef	__V4L2_H__
	#define __V4L2_H__

#include <linux/videodev2.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/ioctl.h>

//采集缓冲队列长度
#define NR_REQBUF 4 

//mmap或userptr用到的缓冲区结构
struct v4l2_buf {
	void   *start;
	__u32 len;
};

//v4l2 用户控制项结构
struct v4l2_usrctrl {
	__u32                id;            //用户控制项id
	enum v4l2_ctrl_type	 type;          //控制项类型：整数、布尔等
	__u8                 name[32];      //名控制项称
	__s32                val;           //控制项当前值
	__s32                def_val;       //默认值
	__s32                min;           //最大值
	__s32                max;           //最小值
};

//采集处理回调函数
//参数1、2、3分别表示：
//帧缓冲首地址、图形大小最大值、v4l2_capture_setup传入参数
typedef int (*capture_cbk_t)(void *, __u32, void *);
//自定义结构体包含  <linux/videodev2.h> 中的数据结构
typedef struct v4l2_device {
	int                     fd;         //设备文件描述符               	
	__u8                    name[32];   //设备名称
	__u8                    drv[16];    //driver名称
	capture_cbk_t           cbk;        //采集处理回调函数
	void                    *args;      //cbk传入参数
	struct v4l2_pix_format  pfmt;       //图像格式：长宽、数据格式等
	struct v4l2_usrctrl     *uctrl;     //用户控制项
	__u32                   nr_uctrl;	//用户控制项的个数
	struct v4l2_buf         *buf;       //缓冲区
	__u32                   nr_buf;     //缓冲区个数
} v4l2_device_t;

/*
 * 为v4l2封装ioctl
 * */
static inline int v4l2_ioctl(int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl (fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

/*
 * 获得用户控制项
 * @nr -- 设备支持的第几个用户控制项
 * */
static inline __s32 v4l2_get_uctrl(v4l2_device_t *vd, __u32 nr)
{
	return nr > vd->nr_uctrl-1 ? -1 : vd->uctrl[nr].val;
}

/*
 * 装载采集处理回调函数
 * */
static inline capture_cbk_t v4l2_capture_setup(v4l2_device_t *vd, 
                                               capture_cbk_t cbk, 
											   void* args)  
{
	capture_cbk_t sav_cbk = vd->cbk;
	vd->cbk  = cbk;
	vd->args = args;
	
	return sav_cbk;
}

//函数声明
extern int v4l2_open(const __u8 *dev, v4l2_device_t *vd);
extern int v4l2_close(v4l2_device_t *vd);
extern int v4l2_uctrl_init(v4l2_device_t *vd);
extern int v4l2_set_uctrl(v4l2_device_t *vd, __u32 nr, __s32 val);
extern int v4l2_mmap_init(v4l2_device_t *vd);
extern int v4l2_pfmt_init(v4l2_device_t *vd, __u32 width, 
                          __u32 height, __u32 pixfmt);
extern int v4l2_init(v4l2_device_t *vd, __u32 width, 
                     __u32 height, __u32 pixfmt);
extern int v4l2_uninit(v4l2_device_t *vd);
extern int v4l2_capture_start(v4l2_device_t *vd);
extern int v4l2_capture_process(v4l2_device_t *vd);
extern int v4l2_capture_stop(v4l2_device_t *vd);

#endif	//__V4L2_H__

