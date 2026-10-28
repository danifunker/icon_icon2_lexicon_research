#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/disk.h>
#include <limits.h>
#include <sys/fsys.h>

#define _USED			0x40
#define NCPFN 16
struct xtnt_hdr {
	long prev_xtnt;
	long next_xtnt;
	long size_xtnt;
	long bound_xtnt;
	};

struct dir_entry {
	char fstat;
	long ffirst_xtnt;
	long flast_xtnt;
	long fnum_blks;
	unsigned fnum_xtnt;
	unsigned char fowner;
	unsigned char fgroup;
	unsigned fnum_chars_free; /* No. unused chars in last block */
	long fseconds;
	unsigned char ftype;
	unsigned char fgperms;
	unsigned char fperms;
	unsigned char fattr;
	unsigned fdate[2];
	char fname[NCPFN + 1];
	};

/*
 * File format of directory
 */

struct dir_xtnt {		/* Top bit of dir_index is zero */
	unsigned parent_xtnt;
	unsigned dir_index;
	struct dir_entry directory[1];
	};

struct ext_dir_xtnt {	/* Top bit of dir_index is set */
	unsigned parent_xtnt;
	unsigned dir_index;
	unsigned ext_parent_xtnt;
	} ;

#define BUF_SIZE 4096
#define _DIRECTORY 0x20
struct dir_entry dir_buffer;
int My_disk;
int disk;
int debug;
long total;
long current_ptr;
long current_sector;
long current_size_xtnt;
long current_size_left;
long root_size;
long incore_sector;
int current_offset;
int current_bytes;
char *pgm_name;
char buf[1024];
char *pgm_name;
int dp;
struct first_sector {
	char stuff[68];
	struct dir_entry root;
	char stuff2[439];
};
struct sector_header {
	struct sector_header *snext;
	long ssector;
	long ssize_xtnt;
};
char buffer[BUF_SIZE];
struct sector_header *current_sp;
struct ext_dir_xtnt dir_header;
int fill_char;

struct sector_header *sector_open_file(long sector,long size);
int nblock_read(int dp,long l,void *buf);
struct sector_header *new_fopen(char *device,char *file,char *mode);
int get_dir_header(struct sector_header *sp);
int new_get_dir_entry(struct sector_header *sp,struct dir_entry *dir_entry);
long get_root(void);
sector_close_file(struct sector_header *sp);
int new_fget(void *buffer,unsigned size,struct sector_header *sp);
void show_new(struct sector_header *s);

nblock_read(dp,l,number,buf)
int dp;
long l;
int number;
char *buf;
{
	int ret;
	int i;

	if (debug) 
		fprintf(stderr,"Read Disk blk %ldh\n",l);
	for(i=0;i<100;i++) {
		ret = block_read(dp,l,number,buf);
		if (ret == number)
			return(0);
		fprintf(stderr,"%d\r",i);
	}
	fprintf(stderr,"Disk blk %ldh Read Failure\n",l);
	return(-1);
}
/*
#define block_read nblock_read
*/
main(argc,argv)
int argc;
char *argv[];
{                                 	
	struct sector_header *new_fp;
	FILE *fp;
	int ret;
	char *file;
	int i;

	fill_char = 0xff;
	debug = 0;
	pgm_name = "q2files";

	if (argc < 3) {
		fprintf(stderr,"use: %s device directory\n",pgm_name);
		fprintf(stderr,"\tdirectory must include full path from root\n");
		fprintf(stderr,"\nEg. q2files /dev/hd0t7 /user/source\n");
		fprintf(stderr,"\nPublic Domain Software by Mitchell Schoenbrun dba Micro Business Applications\n");
		exit(-1);
	}

	incore_sector = -1;
	file = argv[2];
	new_fp = new_fopen(argv[1],file,"r");
	if (!new_fp) {
		fprintf(stderr,"%s: can't open %s\n",pgm_name,argv[2]);
		exit(-1);
	}

	fp = stdout;
	for(i=2;i<argc;i++) {
		if (toupper(argv[i][0]) == 'F' &&
			argv[i][1] == '=') {
			fill_char = atoh(argv[i]);
		}
		else
		if (toupper(argv[i][0]) == 'O' &&
			argv[i][1] == '=') {
			fp = fopen(argv[3]+2,"w");
			if (!fp) {
				fprintf(stderr,"%s: can't open %s\n",argv[2],argv[3]+2);
				exit(-1);
			}
		}
		else
		if (toupper(argv[i][1]) == 'D' &&
			argv[i][0] == '+') {
			debug = 1;
		}
	}


	if (get_dir_header(new_fp)) {
		fprintf(stderr,"Directory is empty\n");
		exit(-1);
	}

	while(1) {
		ret = new_fget((char *) &dir_buffer,sizeof(dir_buffer),new_fp);
		if (ret != sizeof(dir_buffer)) 
			break;
		if ((dir_buffer.fstat & _USED) == 0)
			continue;
		if (dir_buffer.fattr & _DIRECTORY) 
			printf("%.17s+\n",dir_buffer.fname);
		else
			printf("%.17s\n",dir_buffer.fname);
	}
}
struct sector_header *new_fopen(device,file,mode)
char *device;
char *file;
char *mode;
{
	long sector;
	long get_root();
	long size;
	struct sector_header *sp;
	char dir_name[17];
	int i;
	int ret;
	struct dir_entry dir_entry;

	if (strcmp(mode,"r"))
		return(0);
	
	if (*file == '[') {
		while(*file != ']')
			file++;
		file++;
	}

	if (*file != '/') {
		while(*file != '/')
			file++;
	}
	file++;

	dp = open(device, O_RDONLY);
	if (dp == -1) {
		return(0);
	}
	
	sector = get_root();
	size = root_size;
	if (sector == 0L) {
		return(0);
	}

	while(1) {
		for(i=0;i<16;i++)
			if (*file == 0) {
				dir_name[i] = 0;
				break;
			}
			else
			if (*file == '/') {
				dir_name[i] = 0;
				file++;
				break;
			}
			else
				dir_name[i] = *file++;

		if (*file == 0)
			break;

		if (i == 16) {
			return(0);
		}
		
		sp = sector_open_file(sector,size);
		if (get_dir_header(sp)) {
			return(0);
		}
		while(1) {
			ret = new_get_dir_entry(sp,&dir_entry);
			if (ret) {
				return(0);
			}
			if ((dir_entry.fstat & _USED) == 0)
				continue;
			if (!strcmp(dir_entry.fname,dir_name))
				break;
		}
		if (!(dir_entry.fattr & _DIRECTORY)) {
			return(0);
		}
		sector_close_file(sp);
		sector = dir_entry.ffirst_xtnt;
		size = (dir_entry.fnum_blks * 512L) + 512L - (dir_entry.fnum_chars_free & 0x1ff)
					- (dir_entry.fnum_xtnt * 16);
	}
	/* Search for file in it's directory */	
	sp = sector_open_file(sector,size);
	if (get_dir_header(sp)) {
		return(0);
	}
	while(1) {
		ret = new_get_dir_entry(sp,&dir_entry);
		if (ret) {
			return(0);
		}
		if ((dir_entry.fstat & _USED) == 0)
			continue;
		if (!strcmp(dir_entry.fname,dir_name))
			break;
	}
	sector_close_file(sp);
	sector = dir_entry.ffirst_xtnt;
	size = (dir_entry.fnum_blks * 512L) + 512L - (dir_entry.fnum_chars_free & 0x1ff)
					- (dir_entry.fnum_xtnt * 16);
	sp = sector_open_file(sector,size);
	return(sp);
}
int get_dir_header(sp)
struct sector_header *sp;
{
	if (new_fget((void *) &dir_header,4,sp) != 4)
		return(0);
	if (dir_header.dir_index & 0x8000) {
		if (new_fget((void *) &dir_header.ext_parent_xtnt,2,sp) != 2)
			return(-1);
	}
	return(0);
}
new_get_dir_entry(sp,dir_entry)
struct sector_header *sp;
struct dir_entry *dir_entry;
{
	if (new_fget((void *) dir_entry,sizeof(struct dir_entry),sp) != sizeof(struct dir_entry)) {
		return(-1);
	}
	return(0);
}
long get_root()
{
	int ret;
	struct first_sector *f;

	if (incore_sector != 1L) {
		ret = block_read(dp,incore_sector = 1L,1,buf);
		if (ret != 1) {
			return(0L);
		}
	}
	
	f = (struct first_sector *) buf;
	
	root_size = (f->root.fnum_blks * 512L) + 512L - (f->root.fnum_chars_free & 0x1ff)
					- (f->root.fnum_xtnt * 16);
	return(f->root.ffirst_xtnt);
}
struct sector_header *sector_open_file(sector,size)
long sector;
long size;
{
	struct xtnt_hdr *x;
	struct sector_header *s;
	struct sector_header *ts;
	long next;
	int ret;

	total = 0L;
	current_offset = 16;
	current_bytes = 512-16;
	current_size_left = size;
	current_ptr = 0;
	current_sector = sector;
	if (incore_sector != sector) {
		ret = block_read(dp,incore_sector = sector,1,buf);
		if (ret != 1)
			return(0);
	}
	x = (struct xtnt_hdr *) buf;
	s = (struct sector_header *) malloc(sizeof(struct sector_header));
	if (!s)
		return(0);
	current_sp = s;
	
	s->snext = 0;
	s->ssize_xtnt = x->size_xtnt;
	s->ssector = sector;
	current_size_xtnt = s->ssize_xtnt;
	next = x->next_xtnt;
	ts = s;
	total += x->size_xtnt;
	
	while(next != 0L) {
		if (incore_sector != next) {
			ret = block_read(dp,incore_sector = next,1,buf);
			if (ret != 1)
				return(0);
		}
		ts->snext = (struct sector_header *) malloc(sizeof(struct sector_header));		if (!ts->snext)			return(0);
		ts = ts->snext;
		ts->snext = 0;
		ts->ssize_xtnt = x->size_xtnt;
		ts->ssector = next;
		total += x->size_xtnt;
		next = x->next_xtnt;
	}
	return(s);
}
void sector_close_file(sp)
struct sector_header *sp;
{
	struct sector_header *ts;

	while(sp) {
		ts = sp;
		sp = sp->snext;
		free(ts);
	}
}
new_fget(buffer,size,sp)
char *buffer;
unsigned size;
struct sector_header *sp;
{
	int count;
	int ret;
	char *p;

	count = 0;

	if (current_size_left == 0) {
		return(0);
	}

	while(1) {
		if (incore_sector != current_sector) {
			ret = block_read(dp,incore_sector = current_sector,1,buf);
			if (ret != 1) {
				fprintf(stderr,"%s: Disk read error sector %lxh (2)\n",pgm_name,current_sector);
				return(-1);
			}
		}
		p = buf + current_offset;
		while(size && current_bytes && (current_size_left != 0) && (current_size_xtnt != 0)) {
			*buffer++ = *p++;
			size--;
			count++;
			current_bytes--;
			current_size_xtnt--;
			current_size_left--;
			current_ptr++;
			current_offset++;
		}
		if (size == 0 || current_size_left == 0) {
			return(count);
		}

		if (current_size_xtnt != 0) {
			current_sector++;
			current_bytes = 512;
			current_offset = 0;
		}
		else {
			/* Next xtnt */
			current_sp = current_sp->snext;
			if (current_sp == 0) {
				return(count);
			}
			current_sector = current_sp->ssector;
			current_size_xtnt = current_sp->ssize_xtnt;
			current_bytes = 512 - 16;
			current_offset = 16;
		}
	}
}
void show_new(s)
struct sector_header *s;
{
	while(s) {
		fprintf(stderr,"Sector %lx Size %lx\n",s->ssector,s->ssize_xtnt);
		s = s->snext;
	}
	getchar();
}



