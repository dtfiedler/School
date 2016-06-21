#include "udp.c"
#include "mfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/select.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int init(int prot_num, const char *image_file_name);
void worker();
int mfs_lookup(int pinum, char *name, response* res);
int mfs_stat(int inum, response* res);
int mfs_write(int inum, int blocknum, char* block, response* res);
int mfs_read(int inum, int blocknum, response* res);
int mfs_creat(int pinum, int type, char* name, response* res);
int mfs_unlink(int pinum, char* name, response* res);
int mfs_shutdown();
void set_bit(int bit, int val);

int connection_fd;
int disk_img;
char meta_blocks[3*BSIZE];
struct superblock *sb = (struct superblock *)&meta_blocks[0*BSIZE];
struct dinode *inodes = (struct dinode *)&meta_blocks[1*BSIZE];
char *bitmap = &meta_blocks[2*BSIZE];

int 
Math_Pow(int base, int exp){
        if(exp < 0){
                printf("Not Supported\n");
                return -1;
        }
        int result = 1;
        int i;
        for(i=0;i<exp;i++)
                result *= exp;
        return result;
}

int
main(int argc, char *argv[]){
	if(argc !=3){
		fprintf(stderr,"Usage: prompt> server [portnum] [file-system-image]\n");
		exit(1);
	}
	init(atoi(argv[1]), argv[2]);
	worker();	
	return 0;
}
int
init(int port_num, const char *image_file_name){
	int rc;
	connection_fd = UDP_Open(port_num);
	assert(connection_fd >= 0);
	disk_img = open(image_file_name,O_CREAT|O_RDWR,0777);
	//printf("Oh dear, something went wrong with open()! %s\n", strerror(errno));
	assert(disk_img >= 0);
	// read the meta data
	rc = pread(disk_img,meta_blocks,3*BSIZE,1*BSIZE);
	assert(rc >= 0);
//	printf("Read %d bytes of meta data\n",rc);
	//if the image is newly created, then do some init
	if(rc == 0){
		//printf("This is a new disk img, init in progress\n");
		sb->size = (1024 + 4);
		sb->nblocks = 1024;
		sb->ninodes = 64;
		rc = pwrite(disk_img,sb,1*BSIZE,1*BSIZE);
		assert(rc == 4096);
		
		//init inode
		inodes[0].type = MFS_DIRECTORY;
		inodes[0].size = BSIZE;
		inodes[0].addrs[0] = BSIZE * 4;
		int i;
		for(i=1; i< (NDIRECT+1); i++){
			//mark data blk addr to be all one 
			inodes[0].addrs[i] = 0xFFFFFFFF;
		}
		rc = pwrite(disk_img,inodes,1*BSIZE,2*BSIZE);
		assert(rc == 4096);
		//init bitmap
		set_bit(0,1);		
		//init data blk
		MFS_DirEnt_t entries[64];
		snprintf(entries[0].name,60,".");
		entries[0].inum = 0;
		snprintf(entries[1].name,60,"..");
		entries[1].inum = 0;
		for(i=2; i<64; i++){
			entries[i].inum = -1;
		}
		rc = pwrite(disk_img,entries,1*BSIZE,4*BSIZE);
		assert(rc == 4096);
//		printf("done with writing the super block, %d bytes has been written to the image\n",rc);
		assert(fsync(disk_img) >= 0);
		rc = pread(disk_img,meta_blocks,3*BSIZE,1*BSIZE);
//		printf("After init, Read %d bytes of meta data\n",rc);	
	}
//	printf("Yep, it is a valid img, and it has %d blocks and %d inodes, size is %d\n",sb->nblocks,sb->ninodes,sb->size);
	return 0;	
}
void
worker(){
	while(1){
		struct sockaddr_in client_addr;
		message req;
		response res;
		UDP_Read(connection_fd,&client_addr,(char*)&req,sizeof(req));
		//Construct Response
		//printf("Request Cmd is %d, related file name is %s, parent inodenmu is %d",req.cmd,req.name,req.pinum);
		int rc;
		switch(req.cmd){
			case LOOKUP :
				rc = mfs_lookup(req.pinum,req.name,&res);	
				break;
			case STAT :
				rc = mfs_stat(req.inum,&res);
				break;
			case WRITE :
				rc = mfs_write(req.inum,req.blocknum,req.block,&res);
				break;
			case READ :
				rc = mfs_read(req.inum,req.blocknum,&res);
				break;
			case CREAT :
				rc = mfs_creat(req.pinum,req.type,req.name,&res);
				break;
			case UNLINK :
				rc = mfs_unlink(req.pinum,req.name,&res);
				break;
			case SHUTDOWN :
				rc = mfs_shutdown();
				res.rc = 0;
				UDP_Write(connection_fd,&client_addr,(char*)&res,sizeof(res));
				close(connection_fd);
				exit(0);
				break;
			default :
				rc = -1;
		}
		assert(rc >= 0);
		//Send Response
		UDP_Write(connection_fd,&client_addr,(char*)&res,sizeof(res));

	}
}

int 
read_bit(int bit){
	return bitmap[bit/8] & (1 << (7 - bit % 8));
}
void
set_bit(int bit, int val){
	//TODO
	if(val == 0){
		bitmap[bit/8] = bitmap[bit/8] & (Math_Pow(2,8) -1 - Math_Pow(2,(8 - bit%8 +1)));
	}
	else if(val == 1){
		bitmap[bit/8] = bitmap[bit/8] |  (1 << (7 - bit % 8));
	}
	//push the update to the disk image
	
	int rc = pwrite(disk_img,bitmap,1*BSIZE,3*BSIZE);
	assert(rc >=0);
}
int
mfs_lookup(int pinum, char *name, response* res){
	if(pinum > 63 || pinum < 0 ){
		res->rc = -1;
		return 0;
	}
	
	struct dinode inode = inodes[pinum];

	if(inode.type != MFS_DIRECTORY){
		res->rc = -1;
		return 0;
	}

	//go through the data block of the directory
	int rc;
	//	printf("The address of %dth data block is %u\n",i,inode.addrs[i]); 
	//read the valid data block
	MFS_DirEnt_t entries[64];
	rc = pread(disk_img,entries,1*BSIZE,inode.addrs[0]);
	assert(rc == 4096);
	//go through the dir entries inside that data block
	int j;
	for(j=0; j< 64; j++){
		//if it is a valid entry and it contains the desire name
		//then fill up the response and return 
		if(entries[j].inum != -1){
			//printf("This is %dth entry, name is %s, inum is %d\n",j,entries[j].name,entries[j].inum);
			if(strcmp(name,entries[j].name) == 0){
				res->rc = entries[j].inum;
				return 0;
			}
		}
	}
	res->rc = -1;	
	return 0;
}

int
mfs_stat(int inum, response* res){
	//TODO
	if(inum > 63 || inum < 0){
		res->rc = -1;
		return 0;
	}
	
	struct dinode inode = inodes[inum];
	
	if(inode.type == 0){
		res->rc = -1;
		return 0;
	}
	
	res->stat.size = inode.size;
	res->stat.type = inode.type;
	res->rc = 0;
	return 0;
}


int
mfs_write(int inum, int blocknum, char* block, response* res){
	//TODO
	//every after update data, also needs to re read metadata;
	
	if(inum > 63 || inum < 0){
		res->rc = -1;
		return 0;
	}
	
	struct dinode inode = inodes[inum];
	
	if(inode.type != MFS_REGULAR_FILE){
		res->rc = -1;
		return 0;
	}
	
	if( blocknum < 0 || blocknum > 13){
		res->rc = -1;
		return 0;
	}
	
	//case 1 update a block
	if(inode.addrs[blocknum] != 0xFFFFFFFF){
		int rc = pwrite(disk_img,block,1*BSIZE,inode.addrs[blocknum]);
		assert(rc >= 0);
		res->rc = 0;
		return 0;
	}
	
	//case 2 write to a new block
	//if the previous blocks has not been write yet
	//the gap between original EOF and new file should be fill and update size accordly

	//simple case now
	//TODO fill the gap will be done if time allow

	if(blocknum != 0 && inode.addrs[blocknum-1] == 0xFFFFFFFF){
		int k;
		for(k=0; k < 14; k++){
			if(inode.addrs[k] == 0xFFFFFFFF){
				break;
			}
		}
		for(k=k; k < blocknum; k++){	
			int l;
			for(l=0; l<sb->nblocks; l++){
				if(read_bit(l) == 0){
					break;		
				}
			}
			
			unsigned int new_block_addr = 4*BSIZE +  l*BSIZE;
			set_bit(l,1);
			inodes[inum].size += BSIZE;
			inodes[inum].addrs[k] = new_block_addr;
			assert(pwrite(disk_img,inodes,1*BSIZE,2*BSIZE) == BSIZE);
			char empty[BSIZE];
			bzero(empty,BSIZE);
			int rc = pwrite(disk_img,empty,1*BSIZE,inodes[inum].addrs[blocknum]);
			assert(rc >=0);
		}
	}
	//find a new data block that is avaible
	int i;
	for(i=0; i<sb->nblocks; i++){
		if(read_bit(i) == 0){
			break;		
		}
	}

	unsigned int new_block_addr = 4*BSIZE +  i*BSIZE;
	
	//update bitmap and inode, write new inode block back to the image
	set_bit(i,1); 
	inodes[inum].size += BSIZE;
	inodes[inum].addrs[blocknum] = new_block_addr;
	assert(pwrite(disk_img,inodes,1*BSIZE,2*BSIZE) == BSIZE);
	//write the data to the disk img
	int rc = pwrite(disk_img,block,1*BSIZE,inodes[inum].addrs[blocknum]);
	assert(rc >= 0);
	res->rc = 0;			
	return 0;
}

int
mfs_read(int inum, int blocknum, response* res){
	if(inum > 63 || inum < 0){
		res->rc = -1;
		return 0;
	}
	
	struct dinode inode = inodes[inum];
	
	if(inode.type == 0){
		res->rc = -1;
		return 0;
	}
	
	if( blocknum < 0 || blocknum > 13){
		res->rc = -1;
		return 0;
	}
	if(inode.addrs[blocknum] == 0xFFFFFFFF){
		res->rc = -1;
		return 0;
	}
	int rc = pread(disk_img,res->block,1*BSIZE,inode.addrs[blocknum]);
	assert(rc >=0);
	res->rc = 0;
	return 0;
}

int
mfs_creat(int pinum, int type, char* name, response* res){
	//everytime after update data, also needs to re read metadata
	//check if the file already exist

	if(pinum > 63 || pinum < 0 ){
		res->rc = -1;
		return 0;
	}    
	struct dinode inode = inodes[pinum];
	if(inode.type != MFS_DIRECTORY){
		res->rc = -1;
		return 0;
	}
	response tmp;
	mfs_lookup(pinum,name,&tmp);
	if(tmp.rc != -1){
		res->rc =0;
		return 0;
	}
		
	//alloc a new inode for it
	int i;
	for(i=0; i<sb->ninodes; i++){
		if(inodes[i].type == 0)
			break;
	}
	
	//case 1 creat a file
	if(type == MFS_REGULAR_FILE){
		//fill the inode for this new file
		inodes[i].type = MFS_REGULAR_FILE;
		inodes[i].size = 0;
		int j;
		for(j=0; j< (NDIRECT+1); j++){
			//mark data blk addr to be all one 
			inodes[i].addrs[j] = 0xFFFFFFFF;
		}
		//push the new inode to the disk_img
		int rc = pwrite(disk_img,inodes,1*BSIZE,2*BSIZE);
		assert(rc == 4096);
		
		//pull the directory entries and update it
		MFS_DirEnt_t entries[64];
		rc = pread(disk_img,entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);
		//inside data block find a spot for this new entry
		for(j=0; j<64; j++){
			if(entries[j].inum == -1)
				break;
		}		
		entries[j].inum = i;
		snprintf(entries[j].name,60,name);
		
		//push the update back
		rc = pwrite(disk_img,entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);	
		res->rc=0;
		return 0;
		
	}
	//case 2 ceat a directory
	else if(type == MFS_DIRECTORY){
		//fill the inode for this new directory
		inodes[i].type = MFS_DIRECTORY;
		inodes[i].size = BSIZE;
		//find a new data block that is avaible for its directory entries
		int j;
		for(j=0; j<sb->nblocks; j++){
			if(read_bit(j) == 0){
				break;		
			}
		}
		unsigned int new_block_addr = 4*BSIZE +  j*BSIZE;
		inodes[i].addrs[0] = new_block_addr;
		set_bit(j,1);
		for(j=1; j< (NDIRECT+1); j++){
			//mark data blk addr to be all one 
			inodes[0].addrs[j] = 0xFFFFFFFF;
		}
		int rc = pwrite(disk_img,inodes,1*BSIZE,2*BSIZE);
		assert (rc == 4096);
		
		MFS_DirEnt_t entries[64];
		snprintf(entries[0].name,60,".");
		entries[0].inum = i;
		snprintf(entries[1].name,60,"..");
		entries[1].inum = pinum;
		for(j=2; j<64; j++){
			entries[j].inum = -1;
		}
		rc = pwrite(disk_img,entries,1*BSIZE,inodes[i].addrs[0]);
		assert(rc == 4096);
		
		//pull the directory entries and update it
		MFS_DirEnt_t p_entries[64];
		rc = pread(disk_img,p_entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);
		//inside data block find a spot for this new entry
		for(j=0; j<64; j++){
			if(p_entries[j].inum == -1)
				break;
		}		
		p_entries[j].inum = i;
		snprintf(p_entries[j].name,60,name);
		
		//push the update back
		rc = pwrite(disk_img,p_entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);	
		res->rc=0;
		return 0;

	}
	fprintf(stderr,"MFS_Creat Error, Unknown Type\n");
	return -1;
}

int
mfs_unlink(int pinum, char* name, response* res){
	//TODO
	//every after update data, also needs to re read metadata;
	if(pinum > 63 || pinum < 0 ){
		res->rc = -1;
		return 0;
	}    
	struct dinode inode = inodes[pinum];
	if(inode.type != MFS_DIRECTORY){
		res->rc = -1;
		return 0;
	}
	
	//check for special case that is the file or directory has been alrady unlinked
 	response tmp;
	mfs_lookup(pinum,name,&tmp);
	if(tmp.rc == -1){
		res->rc = 0;
		return 0;
	} 
		
	if(inodes[tmp.rc].type == MFS_REGULAR_FILE){
		//mark bitmap as 0
		int i;
		for(i=0; i<14; i++){
			if(inodes[tmp.rc].addrs[i] != 0xFFFFFFFF){
				set_bit((inodes[tmp.rc].addrs[i]-4*BSIZE)/BSIZE,0);
			}
		}
		//free the inode and push the changes to disk_img
		inodes[tmp.rc].type = 0;
		int rc = pwrite(disk_img,inodes,1*BSIZE,2*BSIZE);
		assert(rc ==4096);
		//free the directory entry
		MFS_DirEnt_t entries[64];
		rc = pread(disk_img,entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);
		for(i=0; i<64; i++){
			if(entries[i].inum == tmp.rc)
				break;
		}		
		entries[i].inum = -1;
		//push the update back
		rc = pwrite(disk_img,entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);	
		res->rc=0;
		return 0;
	}
	else if(inodes[tmp.rc].type == MFS_DIRECTORY){
		//check if the directory is empty
		MFS_DirEnt_t entries[64];
		int rc = pread(disk_img,entries,1*BSIZE,inodes[tmp.rc].addrs[0]);
		assert(rc == 4096);
		int i;
		for(i=2; i<64; i++){
			if(entries[i].inum != -1){
				res->rc = -1;
				return 0;
			}
		}
		//mark the corresponding bit to 0
		set_bit((inodes[tmp.rc].addrs[0]-4*BSIZE)/BSIZE,0);
		//free the inode and push it to the disk img
		inodes[tmp.rc].type = 0;
                rc = pwrite(disk_img,inodes,1*BSIZE,2*BSIZE);
                assert(rc ==4096);

		//free the directory entry
		MFS_DirEnt_t p_entries[64];
		rc = pread(disk_img,p_entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);
		for(i=0; i<64; i++){
			if(p_entries[i].inum == tmp.rc)
				break;
		}		
		p_entries[i].inum = -1;
		//push the update back
		rc = pwrite(disk_img,p_entries,1*BSIZE,inode.addrs[0]);
		assert(rc == 4096);	
		res->rc=0;
		return 0;
		
	}

	fprintf(stderr,"MFS_Unlink Error, Unknown Type\n");
        return -1;
}
int
mfs_shutdown(){
	assert(fsync(disk_img) >= 0);
	return 0;
}
