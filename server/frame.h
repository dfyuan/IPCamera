#ifndef	__FRAME_H__
	#define __FRAME_H__
	
#include <linux/types.h>	
#include <linux/videodev2.h>

#include "utils.h"
	
struct frame_header {
#define FRAME_ID_SIZ      12
#define FRAME_ID          "FID_GQCam"
	__u8  fid[FRAME_ID_SIZ];   //帧id, 用于鉴定是否该处理该帧
	__u32 cmd;                 //命令类型	
	__u32 fnr;                 //帧编号
	__u32 fsiz;                //帧大小
} __attribute__ ((packed));  
	
//ctrl_frame_hdr：控制帧返回设备信息的头部	
//控制帧结构：
//| -- frame_header -- | -- ctrl_frame_hdr -- | -- user control list -- |
// ... | -- user control list -- | -- resolution list -- |
struct ctrl_frame_hdr {   
	__u8        name[32];   //设备名称
	__u32       pixfmt;     //像素格式
	__u32       nr_uctrl;	//用户控制项的个数
	__u32       cres_no;    //当前分辨率编号
	__u32       nr_res;     //总共支持的分辨率       
} __attribute__ ((packed));  	

//数据帧结构：
//| -- frame_header -- | -- data -- |

/*
 * CMD structure:
 *  ------------------------------------------------------------------
 * | -- 8bit -- |   6bit   | 2bit |   6bit   | 2bit |   6bit   | 2bit | 
 * |  cmd type  | reserved | rqt  | reserved | dir  | reserved | ack  |
 *  ------------------------------------------------------------------
 */	
#define  CMD_TYP(cmd)     (((cmd)>>24) & 0xff)
#define  CMD_RQT(cmd)     (((cmd)>>16) & 0x3)
#define  CMD_DIR(cmd)     (((cmd)>>8)  & 0x3)
#define  CMD_ACK(cmd)     ((cmd) & 0x3)

#define  MKCMD(type, req, dir, ack)  \
    ( (((type) & 0xff)<<24) | (((req) & 0x3)<<16) | \
	  (((dir) & 0x3)<<8) | ((ack) & 0x3) )

// -------- CMD Type --------
#define  TYP_INF    0x0    //获取设备信息
#define  TYP_SET    0x1    //设置设备属性
#define  TYP_DAT    0x2    //获取采集数据
#define  TYP_QUI    0x10   //
#define  TYP_UDF    0xff   //未知
//#define  TYP_OFF    0xff  //远程关闭设备

// -------- Request Type ----
#define  RQT_REQ    0x0    //请求包
#define  RQT_ACK    0x1    //应答包
	  
// -------- Direction -------
#define  DIR_TOS    0x0 
#define  DIR_TOC    0x1 

// -------- Answer bACK -----
#define  ACK_OK     0x0    //ack ok
#define  ACK_ERR    0x1    //fatal error
#define  ACK_BSY    0x2    //that means device is busy now, 
                           //wait for a moment and try again
//#define  ACK_RAQ    0x3    //req again

//C -> S req info
//S -> C ack ok, return device info 
//S -> C ack err 

//C -> S req setting, send device info
//S -> C ack ok, return device info 
//S -> C ack err

//C -> S req data
//S -> C ack ok, return video data 
//S -> C ack err

	
#endif	//__FRAME_H__

