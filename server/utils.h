#ifndef	__UTILS_H__
	#define __UTILS_H__
		
#include <linux/types.h>	

//DEBUG¿ª¹Ø
#ifndef DEBUG
	#define DBG(fmt, args...)\
		do{} while(0)
#else
	#define DBG(fmt, args...)\
	printf("[%s:%d]"fmt, __func__, __LINE__, ##args)
#endif 

#define ERR_PRINT_RET(ret) \
		{\
			fprintf(stderr, "%s(%d):%s\n", \
		            __func__, __LINE__, strerror(errno));\
			return ret;\
		}	
		
struct rect {
	__u32 w;
	__u32 h;
};	
	
extern int    get_jpeg_size (void *ptr, __u32 siz);	

extern double get_cur_ms    (void);	

extern int    get_res_nr    (struct rect* res_list);

extern int    get_res_no    (struct rect res, struct rect* res_list);

extern void   print_res_list(struct rect* res_list);
	
#endif	//__UTILS_H__