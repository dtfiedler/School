#include "udp.c"
#include "mfs.h"
#include "string.h"
#include <sys/select.h>
int connection_fd;
struct sockaddr_in server_addr;
struct sockaddr_in remote_addr;
void send_request(message *req, response *res);

int 
MFS_Init(char *hostname, int port){
	if((connection_fd = UDP_Open(5103)) < 0)
		return -1;
	if(UDP_FillSockAddr(&server_addr,hostname,port) < 0)
		return -1;
	return 0;
}

void
send_request(message *req, response *res){
	struct timeval tv;
	fd_set fds;
	do{
		UDP_Write(connection_fd,&server_addr,(char*)req,sizeof(*req));
		tv.tv_sec = 5;
		tv.tv_usec = 0;	
		FD_ZERO(&fds);
		FD_SET(connection_fd,&fds);
	}while(select(connection_fd+1,&fds,NULL,NULL,&tv) < 1);
	
	UDP_Read(connection_fd,&remote_addr,(char*)res,sizeof(*res));
}
int 
MFS_Lookup(int pinum, char *name){
	message req;
	response res;
	
	req.cmd = LOOKUP;
	req.pinum = pinum;
	strncpy(req.name,name,60);

	send_request(&req,&res);

	return res.rc;	
}

int 
MFS_Stat(int inum, MFS_Stat_t *m){
	message req;
	response res;
	
	req.cmd = STAT;
	req.inum = inum;
	
	send_request(&req,&res);

	if(res.rc == -1)
		return -1;

	m->type = res.stat.type;
	m->size = res.stat.size;
		
	return 0;
}
int
MFS_Write(int inum, char *buffer, int blocknum){
	message req;
	response res;

	req.cmd = WRITE;
	req.inum = inum;
	req.blocknum = blocknum;
	memcpy(req.block,buffer,4096);
	
	send_request(&req,&res);

	return res.rc;
}
int
MFS_Read(int inum, char *buffer, int blocknum){
	message req;
	response res;
	
	req.cmd = READ;
	req.inum = inum;
	req.blocknum = blocknum;
	
	send_request(&req,&res);

	if(res.rc == -1)
		return -1;
	
	memcpy(buffer,res.block,4096);

	return 0;
}
int
MFS_Creat(int pinum, int type, char *name){
	message req;
	response res;
	
	req.cmd = CREAT;
	req.pinum = pinum;
	req.type = type;
	if(strlen(name) > 59)
		return -1;
	strncpy(req.name,name,60);

	send_request(&req,&res);
	
	return res.rc;
}
int
MFS_Unlink(int pinum, char *name){
	message req;
	response res;
	
	req.cmd = UNLINK;
	req.pinum = pinum;
	strncpy(req.name,name,60);

	send_request(&req,&res);

	return res.rc;
} 
int 
MFS_Shutdown(){
	message req;
	response res;

	req.cmd = SHUTDOWN;
	
	send_request(&req,&res);

	UDP_Close(connection_fd);

	return res.rc;
}
