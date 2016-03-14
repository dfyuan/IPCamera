#ifndef	__FRAME_H__
	#define __FRAME_H__
	
#include <linux/types.h>	
#include <linux/videodev2.h>

#include "utils.h"
	
struct frame_header {
#define FRAME_ID_SIZ      12
#define FRAME_ID          "FID_GQCam"
	__u8  fid[FRAME_ID_SIZ];   //֡id, ���ڼ����Ƿ�ô����֡
	__u32 cmd;                 //��������	
	__u32 fnr;                 //֡���
	__u32 fsiz;                //֡��С
} __attribute__ ((packed));  
	
//ctrl_frame_hdr������֡�����豸��Ϣ��ͷ��	
//����֡�ṹ��
//| -- frame_header -- | -- ctrl_frame_hdr -- | -- user control list -- |
// ... | -- user control list -- | -- resolution list -- |
struct ctrl_frame_hdr {   
	__u8        name[32];   //�豸����
	__u32       pixfmt;     //���ظ�ʽ
	__u32       nr_uctrl;	//�û�������ĸ���
	__u32       cres_no;    //��ǰ�ֱ��ʱ��
	__u32       nr_res;     //�ܹ�֧�ֵķֱ���       
} __attribute__ ((packed));  	

//����֡�ṹ��
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
#define  TYP_INF    0x0    //��ȡ�豸��Ϣ
#define  TYP_SET    0x1    //�����豸����
#define  TYP_DAT    0x2    //��ȡ�ɼ�����
#define  TYP_QUI    0x10   //
#define  TYP_UDF    0xff   //δ֪
//#define  TYP_OFF    0xff  //Զ�̹ر��豸

// -------- Request Type ----
#define  RQT_REQ    0x0    //�����
#define  RQT_ACK    0x1    //Ӧ���
	  
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

