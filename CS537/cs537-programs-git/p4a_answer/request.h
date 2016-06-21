#ifndef __REQUEST_H__

struct request_complex{
        int fd;
        int filesize;
        int is_static;
	struct stat sbuf;
	char filename[8192], cgiargs[8192];
	//other
};
void requestHandle(struct request_complex rc);
struct request_complex pre_handle(int fd);
#endif
