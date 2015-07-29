#ifndef	NP_H
#define NP_H

#include <stdint.h>
/*
#define DEBUG 	1
*/
#define P9_NOTAG	(u16)(~0)
#define P9_NOFID	(u32)(~0)
#define P9_MAXWELEM	16
#define QTDIR 		0x80
#define QTAPPEND	0x40
#define QTEXCL		0x20
#define QTMOUNT		0x10
#define QTAUTH		0x08
#define QTTMP		0x04
#define QTSYMLINK	0x02
#define QTFILE		0x00
#define DMDIR		0x80000000
#define DMAPPEND	0x40000000
#define DMEXCL		0x20000000
#define DMMOUNT		0x10000000
#define DMAUTH		0x08000000
#define DMTMP		0x04000000
#define DMSYMLINK	0x02000000
#define DMDEVICE	0x00800000
#define DMNAMEDPIPE 0x00200000
#define DMSOCKET	0x00100000
#define DMSETUID	0x00080000
#define DMSETGID	0x00040000
#define DMREAD		0x4
#define DMWRITE		0x2
#define DMEXEC		0x1

typedef struct wname_node{
	char *wname;

} wname_node;

/* this data structure holds the qid */
typedef struct qid_t{
	uint8_t type; /* the type of the file */
	uint32_t version; /* version number for a given path */
	uint64_t path; /* file server unique identification of the path */
} qid_t;

/* this data structure hold the stat of a file object */
typedef struct stat_t{
	uint16_t type;
	uint32_t dev;
	qid_t *qid;
	uint32_t mode;
	uint32_t atime;
	uint32_t mtime;
	uint64_t length; //length of file in bytes
	char *name; //filename: must be / if it is the root directory
	char *uid;
	char *gid;
	char *muid;
} stat_t;

typedef enum {
	P9_TVERSION = 100,
	P9_RVERSION,
	P9_TAUTH = 102,
	P9_RAUTH,
	P9_TATTACH = 104,
	P9_RATTACH,
	P9_TERROR = 106,
	P9_RERROR,
	P9_TFLUSH = 108,
	P9_RFLUSH,
	P9_TWALK = 110,
	P9_RWALK,
	P9_TOPEN = 112,
	P9_ROPEN,
	P9_TCREATE = 114,
	P9_RCREATE,
	P9_TREAD = 116,
	P9_RREAD,
	P9_TWRITE = 118,
	P9_RWRITE,
	P9_TCLUNK = 120,
	P9_RCLUNK,
	P9_TREMOVE = 122,
	P9_RREMOVE,
	P9_TSTAT = 124,
	P9_RSTAT,
	P9_TWSTAT = 126,
	P9_RWSTAT,
} p9_msg_t;

typedef struct p9_obj_t{
	uint32_t size;
	p9_msg_t type;
	uint16_t tag;
	uint32_t msize;
	uint16_t version_len;
	char*	 version;
	uint32_t afid;
	uint16_t uname_len;
	char*    uname;
	uint16_t aname_len;
	char*    aname;
	char*    name;
	uint16_t name_len;
	uint32_t perm;
	uint16_t ename_len;
	char*    ename;
	uint16_t oldtag;
	uint32_t fid;
	uint32_t newfid;
	uint16_t nwname;
	uint16_t nwqid;
	uint8_t  mode;
	uint32_t iounit;
	uint64_t offset;
	uint32_t count;
	uint8_t*  data;
	uint16_t stat_len;
	stat_t *stat;
	qid_t *qid;
	qid_t *aqid;
	qid_t **wqid; /* this is a pointer to pointers of qid_ts */
	wname_node *wname_list;
} p9_obj_t;

void int_to_buffer_bytes(uint64_t n, uint8_t *msg_buffer, int start_idx, int length);

void string_to_buffer_bytes(char* s, uint8_t* msg_buffer, int start_idx, int length);

unsigned long long buffer_bytes_to_int(uint8_t *msg_buffer, int start_idx, int length);

void buffer_bytes_to_string(uint8_t *msg_buffer, int start_idx, int length, char *s);

/* this gives back the stat length as it would appear on wire */
unsigned long long get_stat_length(stat_t *stat);

/* encode a qid data structure to a message buffer */
void encode_qid(qid_t *qid, uint8_t *msg_buffer, int start_idx, int length);

/* encode a stat data structure to a message buffer */
void encode_stat(stat_t *stat, uint8_t *msg_buffer, int start_idx, int length);

qid_t *decode_qid(uint8_t *msg_buffer, int start_idx, int length);
qid_t **decode_wqid(uint8_t *msg_buffer, int nwqid, int start_idx);
stat_t *decode_stat(uint8_t *msg_buffer, int start_idx, int length);

int compare_9p_obj(p9_obj_t *p1, p9_obj_t *p2);

/* create a message buffer based on p9 object to be transported on wire */
uint8_t* encode(p9_obj_t *p9_obj);

/* populate a p9_obj with the data in the message buffer. You can only decode Tmessages */
void     decode(uint8_t* msg_buffer, p9_obj_t *p9_obj);

/* print the p9 object in a user friendly way */
void	 print_p9_obj(p9_obj_t	*p9_obj);
#endif /* NP_H */
