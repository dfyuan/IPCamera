#include "stdinc.h"
#include "utils.h"
#include "v4l2.h"
#include "webcam.h"
#include "tcputils.h"
#include "frame.h"

extern struct rect zc301_res[];
extern struct rect ov511_res[];
extern bool g_bcamoff;     //服务退出标识
extern v4l2_device_t g_vd; 

pthread_t g_cam_tid;     //采集线程
pthread_t g_ser_tid;     //服务器线程

pthread_mutex_t	g_cam_mutex;
pthread_cond_t 	g_cam_cond;

char   *g_video_dev_name;
struct rect *g_cur_res_list;  //当前分辨率列表
__u32  g_cur_res_no;         //当前选择的分辨率标号

__u8   *g_cur_frame = NULL;
__u32  g_cur_frame_len;
__u32  g_frame_nr = 0;

__s32  g_pixfmt;
__u16  g_ser_port;
int g_ser_sock;         //服务器 sock

void sig_handler(int signo);
int  cmdline_parse(int argc, char *argv[]);
void cam_server(void *args);
void adjust_devinfo(void);

int main(int argc, char *argv[])
{
	int i;
	int new_sock;         //新的客户端 sock
	int sin_size;
	
	struct sockaddr_in cli_addr;

	if (-1 == cmdline_parse(argc, argv)) 
		_exit(EXIT_FAILURE);
	
	if (-1 == camera_init(g_video_dev_name, 
	                       g_cur_res_list[g_cur_res_no].w, 
	                       g_cur_res_list[g_cur_res_no].h, 
						   g_pixfmt) ) 
		_exit(EXIT_FAILURE);		

    adjust_devinfo();

	signal(SIGINT, sig_handler);
	
	//启动采集线程
	pthread_mutex_init(&g_cam_mutex, NULL);
	pthread_cond_init(&g_cam_cond, NULL);
	pthread_create(&g_cam_tid, NULL, (void *)camera_run, NULL);
	
	//启动服务器
	if (-1 == (g_ser_sock=open_ser_sock(g_ser_port)))
		_exit(EXIT_FAILURE);
	
	sin_size = sizeof(struct sockaddr_in);
	
	while (!g_bcamoff) {
		
		if (-1 == (new_sock=accept(g_ser_sock, (struct sockaddr*)&cli_addr, 
		                            &sin_size))) {
			continue;
		}
		DBG("Got connection from %s\n", inet_ntoa(cli_addr.sin_addr));
		pthread_create(&g_ser_tid, NULL, (void *)cam_server, &new_sock);
	} 
		
	camera_stop();
	
	pthread_join(g_cam_tid, NULL);
    DBG("Cam Thread return!\n");
	pthread_mutex_destroy(&g_cam_mutex);
	pthread_cond_destroy(&g_cam_cond);	
	
	close(g_ser_sock);
	
	return 0;
}

/*
 * S/C 线程 
 */
void cam_server(void *args)
{
	int sock = *((int*)args);
	int i, ret;
	__u32 fnr = 0;              //帧编号
	__u8  fid[FRAME_ID_SIZ];    //帧ID
	__u32 cmd;
	struct frame_header   fhdr;
	struct ctrl_frame_hdr cfhdr;
	
	while (!g_bcamoff) {
		bzero(&fhdr, sizeof(struct frame_header));
        DBG("Wait for REQ\n");
		ret = read_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header));
		if (ret < 0) {
			DBG("Client vaporished\n");
			break;
		} else {
			bzero(fid, FRAME_ID_SIZ);
			strncpy(fid, fhdr.fid, FRAME_ID_SIZ);
			cmd   = fhdr.cmd;		
		}
        DBG("Get REQ\n");
		
		bzero(&fhdr, sizeof(struct frame_header));
		
		if (strncmp(fid, FRAME_ID, FRAME_ID_SIZ)) {
			//鉴别fid, 确定是否该处理该帧
			DBG("Receive Bad package with wrong FID\n");
			strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);
			fhdr.cmd = MKCMD(CMD_TYP(cmd), RQT_ACK, DIR_TOC, ACK_ERR);
			ret = write_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header));   
            if (ret < 0) 
                break;

            continue;
		}
				
		if (CMD_RQT(cmd) != RQT_REQ || CMD_DIR(cmd) != DIR_TOS) {
			DBG("Receive Bad package with wrong RQT or DIR\n");		
			strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);
			fhdr.cmd = MKCMD(CMD_TYP(cmd), RQT_ACK, DIR_TOC, ACK_ERR);
			ret = write_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header));   
            if (ret < 0) 
                break;
			
            continue;
		}
		
		switch (CMD_TYP(cmd)) {
		case TYP_INF:
			strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);
			fhdr.cmd  = MKCMD(CMD_TYP(cmd), RQT_ACK, DIR_TOC, ACK_OK);
            DBG("BUF LEN = %d\n", g_vd.buf[0].len);
			fhdr.fsiz = g_vd.buf[0].len;   //这里的fsiz表示的是
			                                //device为一帧分配的空间
			
			bzero(&cfhdr, sizeof(struct ctrl_frame_hdr));
			strncpy(cfhdr.name,  g_vd.name, 32);
			cfhdr.pixfmt   = g_pixfmt;     //g_vd.pfmt.pixelformat;
			cfhdr.nr_uctrl = g_vd.nr_uctrl;
			cfhdr.cres_no  = g_cur_res_no;
			cfhdr.nr_res   = get_res_nr(g_cur_res_list);
			                       						
			ret = write_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header));   
		    if (ret < 0) 
                goto quit;
				
			ret = write_sock(sock, (__u8*)&cfhdr, sizeof(struct ctrl_frame_hdr));   
            if (ret < 0)
                goto quit;

			ret = write_sock(sock, (__u8*)g_vd.uctrl, 
			                 cfhdr.nr_uctrl * sizeof(struct v4l2_usrctrl));   		
			if (ret < 0)
                goto quit;

			ret = write_sock(sock, (__u8*)g_cur_res_list, 
			                 cfhdr.nr_res * sizeof(struct rect));   					
		    if (ret < 0)
                goto quit;

			break;
		case TYP_SET:
			bzero(&cfhdr, sizeof(struct ctrl_frame_hdr));
			ret = read_sock(sock, (__u8*)&cfhdr, sizeof(struct ctrl_frame_hdr));
            if (ret < 0) 
                goto quit;
            
			
			//如果改变捕获图像的分辨率，则需要重启			
			if (cfhdr.cres_no != g_cur_res_no) {
				if (cfhdr.cres_no >= get_res_nr(g_cur_res_list)) {    
				    //请求的分辨率不被支持, 不用重启
					DBG("cfhdr.cres_no is out of range, set default!\n");						
				} else {
					g_cur_res_no = cfhdr.cres_no;
					
					if (-1 == camera_restart(g_video_dev_name, 
											 g_cur_res_list[g_cur_res_no].w, 
											 g_cur_res_list[g_cur_res_no].h, 
											 g_pixfmt)) 
						_exit(EXIT_FAILURE);
					
					struct rect res;
					res.w = g_vd.pfmt.width;
					res.h = g_vd.pfmt.height;
					g_cur_res_no = get_res_no(res, g_cur_res_list);
					g_pixfmt     = g_vd.pfmt.pixelformat;					
                    DBG("Camera restart ok, w = %d, h = %d!\n", 
                        res.w, res.h);
                    g_frame_nr = 0;
				}
			}
			
			//用接收到的参数设置设备
			for (i = 0; i < g_vd.nr_uctrl; i++) {
				struct v4l2_usrctrl uctrl;
				
				ret = read_sock(sock, (__u8*)&uctrl, 
                                sizeof(struct v4l2_usrctrl));
                if (ret < 0) 
                    goto quit;

				if (uctrl.val ==  g_vd.uctrl[i].val)
					continue;

				v4l2_set_uctrl(&g_vd, i, uctrl.val);
				g_vd.uctrl[i].val = uctrl.val;				
			}

			//应答, 并返回新的设备信息
			strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);
			fhdr.cmd = MKCMD(CMD_TYP(cmd), RQT_ACK, DIR_TOC, ACK_OK);
            DBG("BUF LEN = %d\n", g_vd.buf[0].len);
			fhdr.fsiz = g_vd.buf[0].len;   
			
			bzero(&cfhdr, sizeof(struct ctrl_frame_hdr));
			strncpy(cfhdr.name,  g_vd.name, 32);
			cfhdr.pixfmt   = g_pixfmt;      //g_vd.pfmt.pixelformat;
			cfhdr.nr_uctrl = g_vd.nr_uctrl;
			cfhdr.cres_no  = g_cur_res_no;
			
            DBG("setting Cam write_sock fhdr\n");
			ret = write_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header));   
			if (ret < 0)
                goto quit;

            DBG("setting Cam write_sock cfhdr, cres_no = %d\n", 
                g_cur_res_no);
			ret = write_sock(sock, (__u8*)&cfhdr, sizeof(struct ctrl_frame_hdr));   
            if (ret < 0)
                goto quit;

            DBG("setting Cam write_sock uctrl, nr_uctrl = %d\n", 
            cfhdr.nr_uctrl);
			ret = write_sock(sock, (__u8*)g_vd.uctrl, 
			                 cfhdr.nr_uctrl * sizeof(struct v4l2_usrctrl));   					  								
			if (ret < 0)
                goto quit;
            //sleep(1);  
            while (!g_frame_nr) 
                usleep(1000);

            break;
		case TYP_DAT:
			if (NULL == g_cur_frame) {
				DBG("Device is not ready\n");		
				strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);
				fhdr.cmd = MKCMD(CMD_TYP(cmd), RQT_ACK, DIR_TOC, ACK_BSY);
				ret = write_sock(sock, (__u8*)&fhdr, 
                                 sizeof(struct frame_header));
                if (ret < 0)
                    goto quit;
				
                break;
			}
			
			strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);			
			fhdr.cmd  = MKCMD(CMD_TYP(cmd), RQT_ACK, DIR_TOC, ACK_OK);		
			fhdr.fnr  = fnr++;
			//pthread_mutex_lock(&g_cam_mutex);
			fhdr.fsiz = g_cur_frame_len;
			
            __u8 *buf = g_cur_frame;
            DBG("real len = %d\n", g_cur_frame_len);
			
			ret = write_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header));
			if (ret < 0) 
                goto quit;
            
			ret = write_sock(sock, buf, fhdr.fsiz); 
		    if (ret < 0) 
                goto quit;
			
            //pthread_mutex_unlock(&g_cam_mutex);
			
            break;
		case TYP_QUI:
	        goto quit;
            break;
        default:
			DBG("Receive Bad package with wrong CMD Type\n");		
			strncpy(fhdr.fid, FRAME_ID, FRAME_ID_SIZ);
			fhdr.cmd = MKCMD(TYP_UDF, RQT_ACK, DIR_TOC, ACK_ERR);
			ret = write_sock(sock, (__u8*)&fhdr, sizeof(struct frame_header)); 		
			break;
		}
	}
quit:
	close_sock(sock);
    DBG("Client disconnect!\n");
	pthread_exit(NULL);	
}

int get_frame_size(void *ptr, __u32 siz)
{
    int fsiz = 0;
    switch (g_pixfmt) {
    case V4L2_PIX_FMT_JPEG:
        fsiz = get_jpeg_size(ptr, siz);
        break;
    default:
        break;
    }
    return fsiz;
}

/*
 * global var: g_cur_frame, g_cur_frame_len
 */
int cbk_get_cur_frame(void *ptr, __u32 siz, void *args)
{
	g_cur_frame     = (__u8 *)ptr;
	g_cur_frame_len = get_frame_size(ptr, siz);
    g_frame_nr++;	

	return 0;
}

/*
 * SIGINT 信号处理函数
 * global var: g_bcamoff
 */
void sig_handler(int signo)
{
    DBG("Signal Int!\n");
    close(g_ser_sock);
	g_bcamoff = TRUE;
}

/*
 * global var: g_cur_res_list, g_vd
 */
void adjust_devinfo(void)
{
    if (strcmp(g_vd.drv, DRV_OV511)) {
        g_cur_res_list = ov511_res;
    } else if (strcmp(g_vd.drv, DRV_ZC301)) {
        g_cur_res_list = zc301_res;
    }
    // else if ...
    g_pixfmt = g_vd.pfmt.pixelformat;
}

/*
 * global var: g_video_dev_name, g_cur_res_no, g_ser_port
 *             g_cur_res_list, g_pixfmt
 */
int cmdline_parse(int argc, char *argv[])
{
	int i;
	char *res_str = NULL;   //分辨率格式字符串，如320x240
	char *separator;
	struct rect res;
	
	//以下为默认设置
	g_video_dev_name = "/dev/video0";
	g_cur_res_list   = zc301_res;
	g_cur_res_no     = 0;
	g_ser_port       = SER_PORT;
	g_pixfmt         = V4L2_PIX_FMT_JPEG;
	
	//解析命令
	for (i = 1; i < argc; i++) {  //跳过无效参数
		if (argv[i] == NULL || *argv[i] == 0 || *argv[i] != '-') 
			continue;
		
		if (0 == strcmp(argv[i], "-d")) {
			if (i+1 >= argc) {
				printf("%s(%d): No parameter specified with -d\n", 
				       __func__, __LINE__);
				return -1;				
			}
			g_video_dev_name = strdup(argv[i+1]);			
		} else if (0 == strcmp (argv[i], "-s")) {
			if (i+1 >= argc) {
				printf("%s(%d): No parameter specified with -d\n", 
				       __func__, __LINE__);
				return -1;				
			}

			res_str = strdup(argv[i+1]);

			res.w = strtoul (res_str, &separator, 10);
			if (*separator != 'x') {
				printf("%s(%d): Error in size use -s widthxheight\n", 
				       __func__, __LINE__);
				return -1;	
			} else {
				++separator;
				res.h = strtoul (separator, &separator, 10);
			}
			if (-1 == get_res_no(res, g_cur_res_list)) {
				printf("%s(%d): Not support this kind of resolution!\n",
				       __func__, __LINE__);
				print_res_list(g_cur_res_list);
				return -1;
			}
		} else if (0 == strcmp(argv[i], "-p")) {
			if (i+1 >= argc) {
				printf("%s(%d): No parameter specified with -p\n", 
				       __func__, __LINE__);
				return -1;		
			}
			g_ser_port = (__u16)atoi(argv[i+1]);
			if (g_ser_port < 1024) {
				printf ("Port should be between 1024 to 65536"
				        " set default %d!\n", SER_PORT);
				g_ser_port = SER_PORT;
			}
		} else if (strcmp (argv[i], "-h") == 0) {
			printf ("usage: %s [-h -d -g ]\n", argv[0]);
			printf ("-h	print this message \n");
			printf ("-d	/dev/videoX  -- use videoX device\n");
			printf ("-s	widthxheight -- use specified resolution\n");
			printf ("-p	port         -- use specified server port \n");

			return -1;
		}
		// else if .... //待扩展
    }
	
	DBG("dev name: %s\n", g_video_dev_name);
	DBG("Resolution: %d x %d\n", g_cur_res_list[g_cur_res_no].w, 
	                             g_cur_res_list[g_cur_res_no].h);
	
	return 0;
}



