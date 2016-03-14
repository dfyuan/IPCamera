#include "stdinc.h"
#include "utils.h"

#include "v4l2.h"

/*
 * 打开设备
 * */
int v4l2_open(const __u8 *dev, v4l2_device_t *vd)
{
#define DEFAULT_DEVICE "/dev/video0"
	if (!dev)
		dev = DEFAULT_DEVICE;
	if ((vd->fd=open(dev, O_RDWR))<0)  
		ERR_PRINT_RET(-1);
	return 0;
#undef DEFAULT_DEVICE
}

/*
 * 关闭设备
 * */
int v4l2_close(v4l2_device_t *vd)
{
	return close(vd->fd);
}

/*
 * 初始化用户控制项
 * */
int v4l2_uctrl_init(v4l2_device_t *vd)
{
	int i = 0;
	struct v4l2_queryctrl qctrl;
	struct v4l2_control ctrl;
	
	vd->nr_uctrl = 0;
	vd->uctrl = NULL;

	//enumerate 
	bzero(&qctrl, sizeof(qctrl));
	for (qctrl.id = V4L2_CID_BASE; qctrl.id < V4L2_CID_LASTP1; qctrl.id++) {
		if (-1 == ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl)) {
			if (errno == EINVAL)
				continue;
			else
				ERR_PRINT_RET(-1);
		}
			
		if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
			continue;
		//bzero (&ctrl, sizeof(ctrl));
		ctrl.id = qctrl.id;
		if (-1 == ioctl(vd->fd, VIDIOC_G_CTRL, &ctrl)) 
			ERR_PRINT_RET(-1);

		//setup
		vd->uctrl = realloc(vd->uctrl, (vd->nr_uctrl+1)*sizeof(struct v4l2_usrctrl));
		vd->uctrl[vd->nr_uctrl].id   = qctrl.id;
		vd->uctrl[vd->nr_uctrl].type = qctrl.type;
		vd->uctrl[vd->nr_uctrl].val  = ctrl.value;
		vd->uctrl[vd->nr_uctrl].def_val = qctrl.default_value;
		vd->uctrl[vd->nr_uctrl].min  = qctrl.minimum;
		vd->uctrl[vd->nr_uctrl].max  = qctrl.maximum;
		strcpy(vd->uctrl[vd->nr_uctrl].name, qctrl.name);
		//debug
		DBG("user control: id = 0x%x, type = %d, val = %d,\n"
			"def_val = %d, min = %d, max = %d, name = %s, idx = %d\n", 
			vd->uctrl[vd->nr_uctrl].id, vd->uctrl[vd->nr_uctrl].type,
			vd->uctrl[vd->nr_uctrl].val, vd->uctrl[vd->nr_uctrl].def_val,
			vd->uctrl[vd->nr_uctrl].min, vd->uctrl[vd->nr_uctrl].max,
			vd->uctrl[vd->nr_uctrl].name, vd->nr_uctrl);
		//debug ... end	
		vd->nr_uctrl++;
	}
	return 0;
}

/*
 * 设置用户控制项
 * @nr -- 设备支持的第几个用户控制项
 * @ 先使用VIDIOC_S_CTRL设置，再用VIDIOC_G_CTRL回读确认
 * 并将结果记录到vd->uctrl[nr].val中。
 * */
int v4l2_set_uctrl(v4l2_device_t *vd, __u32 nr, __s32 val)
{
	struct v4l2_control ctrl;
	
	if (nr > vd->nr_uctrl-1) {
		printf("%s(%d): Not support the user control!\n", __func__, __LINE__);
		return -1;
	}
	
	if (val > vd->uctrl[nr].max || val < vd->uctrl[nr].min) {
		printf("%s(%d): @val is out of range!\n", __func__, __LINE__);
		return -1;	
	}
	
	ctrl.id    = vd->uctrl[nr].id;
	ctrl.value = val;
	if (-1 == ioctl (vd->fd, VIDIOC_S_CTRL, &ctrl)) 
		ERR_PRINT_RET(-1);

	bzero(&ctrl, sizeof(ctrl));
	ctrl.id = vd->uctrl[nr].id;
	if (-1 == ioctl (vd->fd, VIDIOC_G_CTRL, &ctrl)) 
		ERR_PRINT_RET(-1);
	
	vd->uctrl[nr].val = ctrl.value;
	
	return 0;
}

/*
 * 设备buf的映射
 * */
int v4l2_mmap_init(v4l2_device_t *vd)
{
	struct v4l2_requestbuffers req;

	req.count  = NR_REQBUF;    		//缓存队列长度
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	
	if (-1 == v4l2_ioctl(vd->fd, VIDIOC_REQBUFS, &req)) 
		ERR_PRINT_RET(-1);

	if (req.count < 2) {
		printf("%s(%d): Insufficient buffer memory!\n", __func__, __LINE__);
		return -1;		
	}

	vd->buf = calloc(req.count, sizeof(struct v4l2_buf));
	if (!vd->buf) {
		printf("%s(%d): Out of memory!\n", __func__, __LINE__);
		return -1;	
	}
	
	
	int i; 
	for (i = 0; i < req.count; i++) {
		struct v4l2_buffer buf;
		bzero(&buf, sizeof(buf));

		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;

		if (-1 == v4l2_ioctl (vd->fd, VIDIOC_QUERYBUF, &buf)) 
			ERR_PRINT_RET(-1);

		vd->buf[i].len   = buf.length;
		vd->buf[i].start = mmap(NULL, buf.length, 
									PROT_READ | PROT_WRITE,
									MAP_SHARED, vd->fd, 
									buf.m.offset);

		if (MAP_FAILED == vd->buf[i].start) 
			ERR_PRINT_RET(-1);
	}	
	vd->nr_buf = req.count;

	return 0;
}

/*
 * 采集像素格式初始化
 * */
int v4l2_pfmt_init(v4l2_device_t *vd, __u32 width, __u32 height, __u32 pixfmt)
{
	struct v4l2_format fmt;
	
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width; 
	fmt.fmt.pix.height      = height;
	fmt.fmt.pix.pixelformat = pixfmt;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
	
	if (-1 == v4l2_ioctl (vd->fd, VIDIOC_S_FMT, &fmt)) 
		ERR_PRINT_RET(-1);
	//debug
	DBG("pixel format: width = %d, height = %d,\n"
		"pixfmt = 0x%x, bytesperline = %d, sizeimage = %d\n", 
		fmt.fmt.pix.width, fmt.fmt.pix.height,
		fmt.fmt.pix.pixelformat, fmt.fmt.pix.bytesperline,
		fmt.fmt.pix.sizeimage);
	//debug ... end		
	vd->pfmt = fmt.fmt.pix;
	
	return 0;
}

/*
 * 初始化设备，完成以下工作(注意这个顺序，特别是最后两个步骤是必要的)：
 * @ 判断设备类型是否是video capture device
 * @ 获取设备名、装载用户控制项
 * @ 设置采集格式
 * @ 设备缓冲区映射
 * */
int v4l2_init(v4l2_device_t *vd, __u32 width, __u32 height, __u32 pixfmt) 
{
	struct v4l2_capability cap;
	
	if (-1 == v4l2_ioctl(vd->fd, VIDIOC_QUERYCAP, &cap)) 
		ERR_PRINT_RET(-1);

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("%s(%d): Not a video capture device!\n", __func__, __LINE__);
		return -1;
	}	
	
	bzero(vd->name, sizeof(vd->name));
	strcpy(vd->name, cap.card);
	bzero(vd->drv, sizeof(vd->drv));
	strcpy(vd->drv, cap.driver);

    DBG("name: %s, drv: %s\n", vd->name, vd->drv);

	if (-1 == v4l2_uctrl_init(vd)) 
		return -1;

	if (-1 == v4l2_pfmt_init(vd, width, height, pixfmt)) 
		return -1;	
	
	if (-1 == v4l2_mmap_init(vd)) 
		return -1;	

	return 0;
}

/*
 * 反初始化设备
 * */
int v4l2_uninit(v4l2_device_t *vd) 
{	

	int i;

	for (i = 0; i < vd->nr_buf; i++) {
		if (-1 == munmap(vd->buf[i].start, vd->buf[i].len)) 
			ERR_PRINT_RET(-1);
	}
	
	free(vd->buf);
	free(vd->uctrl);
	
	return 0;
}

/*
 * 开启捕获图形，完成以下工作：
 * @ 将缓冲区加入队列
 * @ 启动捕获
 * */
int v4l2_capture_start(v4l2_device_t *vd)
{
	int i;
	enum v4l2_buf_type type;
	
	for (i = 0; i < vd->nr_buf; i++) {
		struct v4l2_buffer buf;

		bzero(&buf, sizeof(buf));

		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;

		if (-1 == v4l2_ioctl(vd->fd, VIDIOC_QBUF, &buf)) 
			ERR_PRINT_RET(-1);
	}
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == v4l2_ioctl (vd->fd, VIDIOC_STREAMON, &type)) 
		ERR_PRINT_RET(-1);
	
	return 0;
}

/*
 * 处理捕获到的一帧图形
 * @ 使用select
 * @ 从队列中取出一个buf->执行回调函数->送回队列
 * */
int v4l2_capture_process(v4l2_device_t *vd)
{
	fd_set fds;
	struct timeval tv;	
	struct v4l2_buffer buf;
	int ret;
	
	FD_ZERO(&fds);
    FD_SET(vd->fd, &fds);
	
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	ret = select(vd->fd+1, &fds, NULL, NULL, &tv);
	if (-1 == ret) {
		ERR_PRINT_RET(-1);
	} else if (0 == ret) {
		printf("%s(%d): select timeout!\n", __func__, __LINE__);
		return -1;
	} else {  //有数据可读
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;	
		//从队列中取出一个buf
		if (-1 == v4l2_ioctl(vd->fd, VIDIOC_DQBUF, &buf)) 
			ERR_PRINT_RET(-1);

		//执行回调函数
		vd->cbk(vd->buf[buf.index].start, 
				vd->pfmt.width * vd->pfmt.height, vd->args);
		//送回队列
		if (-1 == v4l2_ioctl(vd->fd, VIDIOC_QBUF, &buf)) 
			ERR_PRINT_RET(-1);
	}
	
	return 0;
}

/*
 * 终止捕获图形，完成以下工作：
 * */
int v4l2_capture_stop(v4l2_device_t *vd)
{
	enum v4l2_buf_type type;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == v4l2_ioctl (vd->fd, VIDIOC_STREAMOFF, &type)) 
		ERR_PRINT_RET(-1);
	
	return 0;
}







