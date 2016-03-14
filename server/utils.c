#include "stdinc.h"

#include "utils.h"

/*
 * 捕获到的jpeg图片大小
 * */
int get_jpeg_size(void *ptr, __u32 siz)
{
	int i;
	
	__u8  *buf = (__u8 *)ptr;
	__u32 jpeg_size = siz>>2; 	
	
	for (i = 1024; i < jpeg_size; i++) {
		if ((buf[i] == 0xFF) && (buf[i+1] == 0xD9)) 
			break;
	}	
	
	return i+10;
}

double get_cur_ms(void)
{
	static struct timeval tod;
	gettimeofday(&tod, NULL);
	
	return ((double)tod.tv_sec*1000.0 + (double)tod.tv_usec/1000.0);
}

int get_res_nr(struct rect* res_list)
{
	int i = 0;
	while (res_list[i].w)
		i++;
		
	return i;
}	

int get_res_no(struct rect res, struct rect* res_list)
{
	int i = 0;
	while (res_list[i].w) {
		if (res.w == res_list[i].w && res.h == res_list[i].h)
			break;
		i++;
	}
		
	if (!res_list[i].w)
		return -1;
		
	return i;
}		

void print_res_list(struct rect* res_list)
{
	int i = 0;
	
	printf("The resolution list supportted by the device:");
	while (!res_list[i].w) {
		printf("[ %d x %d ]\n", res_list[i].w, res_list[i].h);	
		i++;
	}
}		
