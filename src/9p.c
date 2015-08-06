#include "9p.h"
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>

unsigned long long int_pow(int x, int y){
	if(y == 0) return 1;
	else return x * int_pow(x, y - 1);
}

void int_to_buffer_bytes(uint64_t n, uint8_t *msg_buffer, int start_idx, int length){
	int i;
	for(i = 0; i < length; i++) {
		msg_buffer[start_idx + i] = (n >> (8 * i)) & 255ULL;
	}
}

void string_to_buffer_bytes(char* s, uint8_t* msg_buffer, int start_idx, int length){
	memcpy(msg_buffer + start_idx, s, length);
}

unsigned long long buffer_bytes_to_int(uint8_t *msg_buffer, int start_idx, int length){
	unsigned long long n;
	int i;
	n = 0;
	for(i = 0; i < length; i++){
		n += msg_buffer[i + start_idx] * int_pow(256, i);
	}
	return n;
}

void buffer_bytes_to_string(uint8_t *msg_buffer, int start_idx, int length, char *s){
	bzero(s, length + 1);
	memcpy(s, msg_buffer + start_idx, length);
}

void encode_qid(qid_t *qid, uint8_t *msg_buffer, int start_idx, int length){
	msg_buffer[start_idx] = qid->type;
	int_to_buffer_bytes(qid->version, msg_buffer, start_idx + 1, 4);
	int_to_buffer_bytes(qid->path, msg_buffer, start_idx + 5, 8);
}

void encode_stat(stat_t *stat, uint8_t* msg_buffer, int start_idx, int length){
	int name_len;
	int uid_len;
	int gid_len;
	int muid_len;
	int_to_buffer_bytes(length, msg_buffer, start_idx, 2);
	int_to_buffer_bytes(stat->type, msg_buffer, start_idx + 2, 2);
	int_to_buffer_bytes(stat->dev, msg_buffer, start_idx + 4, 4);
	encode_qid(stat->qid, msg_buffer, start_idx + 8, 13);
	int_to_buffer_bytes(stat->mode, msg_buffer, start_idx + 21, 4);
	int_to_buffer_bytes(stat->atime, msg_buffer, start_idx + 25, 4);
	int_to_buffer_bytes(stat->mtime, msg_buffer, start_idx + 29, 4);
	int_to_buffer_bytes(stat->length, msg_buffer, start_idx + 33, 8);
	name_len = strlen(stat->name);
	int_to_buffer_bytes(name_len, msg_buffer, start_idx + 41, 2);
	string_to_buffer_bytes(stat->name, msg_buffer, start_idx + 43, name_len);
	uid_len = strlen(stat->uid);
	int_to_buffer_bytes(uid_len, msg_buffer, start_idx + name_len + 43, 2);
	string_to_buffer_bytes(stat->uid, msg_buffer, start_idx + name_len + 45, uid_len);
	gid_len = strlen(stat->gid);
	int_to_buffer_bytes(gid_len, msg_buffer, start_idx + name_len + uid_len + 45, 2);
	string_to_buffer_bytes(stat->gid, msg_buffer, start_idx + name_len + uid_len + 47, gid_len);
	muid_len = strlen(stat->muid);
	int_to_buffer_bytes(muid_len, msg_buffer, start_idx + name_len +uid_len + 47 + gid_len, 2);
	string_to_buffer_bytes(stat->muid, msg_buffer, start_idx + name_len +uid_len + 49 + gid_len, muid_len);
}

qid_t *decode_qid(uint8_t *msg_buffer, int start_idx, int length){
	/* length is always 13. Should take it away later */
	qid_t *qid;
	qid = (qid_t *) malloc(sizeof(qid_t));
	qid -> type = msg_buffer[start_idx];
	qid -> version = buffer_bytes_to_int(msg_buffer, start_idx + 1, 4);
	qid -> path = buffer_bytes_to_int(msg_buffer, start_idx + 5, 8);
	return qid;
}

qid_t **decode_wqid(uint8_t *msg_buffer, int nwqid, int start_idx){
	int i;
	qid_t **wqid;
	wqid = (qid_t **)malloc(nwqid * sizeof(qid_t *));
	for(i = 0; i < nwqid; i++){
		wqid[i] = decode_qid(msg_buffer, start_idx + 13 * i, 13);
	}
	return wqid;
}

stat_t *decode_stat(uint8_t *msg_buffer, int start_idx, int length){
	stat_t *s;
	int name_len, uid_len, gid_len, muid_len;
	s = (stat_t *) malloc(sizeof(stat_t));
	buffer_bytes_to_int(msg_buffer, start_idx, 2);
	//printf("stat_len %d\n", length);
	//printf("size field %d\n", size);
	s->type = buffer_bytes_to_int(msg_buffer, start_idx + 2, 2);
	s->dev = buffer_bytes_to_int(msg_buffer, start_idx + 4, 4);
	s -> qid = decode_qid(msg_buffer, start_idx + 8, 13);
	s->mode = buffer_bytes_to_int(msg_buffer, start_idx + 21, 4);
	s->atime = buffer_bytes_to_int(msg_buffer, start_idx + 25, 4);
	s->mtime = buffer_bytes_to_int(msg_buffer, start_idx + 29, 4);
	s->length = buffer_bytes_to_int(msg_buffer, start_idx + 33, 8);
	name_len = buffer_bytes_to_int(msg_buffer, start_idx + 41, 2);
	s->name = (char *) malloc(name_len * sizeof(char) + 1);
	bzero(s->name, name_len + 1);
	buffer_bytes_to_string(msg_buffer, start_idx+43, name_len, s->name);
	uid_len = buffer_bytes_to_int(msg_buffer, start_idx + name_len + 43, 2);
	s->uid = (char *)malloc(uid_len + 1);
	buffer_bytes_to_string(msg_buffer, start_idx + name_len + 45, uid_len, s->uid);
	gid_len = buffer_bytes_to_int(msg_buffer, start_idx + name_len + uid_len + 45, 2);
	s->gid = (char *) malloc(gid_len + 1);
	buffer_bytes_to_string(msg_buffer, start_idx + name_len + uid_len + 47, gid_len, s-> gid);
	muid_len = buffer_bytes_to_int(msg_buffer, start_idx + name_len + uid_len + gid_len + 47, 2);
	if(muid_len != 0){
		s->muid = (char *) malloc(muid_len + 1);
		buffer_bytes_to_string(msg_buffer, start_idx + name_len + uid_len + gid_len + 49, muid_len, s -> muid);
	}
	return s;
}

void destroy_stat(stat_t *s){
	if(s -> qid != NULL){
		free(s -> qid);
		s -> qid = NULL;
	}
	if(s -> gid != NULL){
		free(s -> gid);
		s -> gid = NULL;
	}
	if(s -> uid != NULL){
		free(s -> uid);
		s -> uid = NULL;
	}
	if(s -> name != NULL){
		free(s -> name);
		s->name = NULL;
	}
	free(s);
}

int compare_9p_obj(p9_obj_t *p1, p9_obj_t *p2){
	assert(p1->size == p2->size);
	assert(p1->tag == p2 -> tag);
	assert(p1->type == p2->type);
	if(p1 -> type == P9_TVERSION || p1->type == P9_RVERSION){
		assert(p1->msize == p2->msize);
		assert(p1->version_len == p2->version_len);
		assert(strcmp(p1->version, p2->version) == 0);
	}
	/* neglecting TAUTH and RAUTH for now
	 *
	 */
	if(p1 -> type == P9_RERROR){
		assert(p1 -> ename_len == p2 -> ename_len);
		assert(strcmp(p1->ename, p2->ename) == 0);
	}

	/* neglecting TFLUSH and RFLUSH for now */
	if(p1 -> type == P9_RSTAT){
		stat_t *s1;
		stat_t *s2;
		qid_t *q1;
		qid_t *q2;
		assert(p1->stat_len == p2->stat_len);
		s1 = p1->stat;
		s2 = p2->stat;
		assert(s1->atime == s2->atime);
		assert(s1->dev == s2->dev);
		assert(strcmp(s1->gid, s2->gid) == 0);
		assert(s1->length == s2-> length);
		assert(s1->mode == s2->mode);
		assert(s1->mtime == s1->mtime);
		assert(strcmp(s1->uid, s2->uid) == 0);
		assert(strcmp(s1->name, s2->name) == 0);
		assert(s1->type == s2->type);
		q1 = s1->qid;
		q2 = s2->qid;
		assert(q1->path == q2->path);
		assert(q1->version == q2->version);
		assert(q1->type == q2->type);
	}

	if(p1 -> type == P9_RREAD){
		int i;
		assert(p1->count == p2->count);
		for(i = 0; i < p1->count; i++){
			assert(p1->data[i] == p2->data[i]);
		}
	}
	if(p1->type == P9_ROPEN){
		qid_t *q1;
		qid_t *q2;
		q1 = p1->qid;
		q2 = p2->qid;
		assert(q1->path == q2->path);
		assert(q1->version == q2->version);
		assert(q1->type == q2->type);
		assert(p1->iounit == p2->iounit);
	}
	if(p1->type == P9_RWALK){
		int i;
		assert(p1->nwqid == p2->nwqid);
		for(i = 0; i < p1->nwqid; i++){
			qid_t *q1;
			qid_t *q2;
			q1 = p1->wqid[i];
			q2 = p2->wqid[i];
			assert(q1->path == q2->path);
			assert(q1->version == q2->version);
			assert(q1->type == q2->type);
		}
	}
	return 1;
}

/* decodes a Tmessage or an Rmessage */
void decode(uint8_t* msg_buffer, p9_obj_t *p9_obj){
	char *uname;
	char *aname;
	char *version;
	char *name;
	uint8_t *data;
	p9_obj -> size = (uint32_t) buffer_bytes_to_int(msg_buffer, 0, 4);
	p9_obj -> type = msg_buffer[4];
	p9_obj -> tag  = (uint16_t) buffer_bytes_to_int(msg_buffer, 5, 2);
	switch(p9_obj -> type){
	    case P9_RVERSION:
		case P9_TVERSION:
			p9_obj -> msize = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> version_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 11, 2);
			version = (char *) malloc(p9_obj -> version_len + 1);
			buffer_bytes_to_string(msg_buffer, 13, p9_obj -> version_len, version);
			p9_obj -> version = version;
			break;
		case P9_TAUTH:
			p9_obj -> afid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> uname_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 11, 2);
			uname = (char*) malloc(p9_obj -> uname_len + 1);
			buffer_bytes_to_string(msg_buffer, 13, p9_obj -> uname_len, uname);
			p9_obj -> uname = uname;
			p9_obj -> aname_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 13 + p9_obj -> uname_len, 2);
			aname = (char *) malloc(p9_obj -> aname_len + 1);
			buffer_bytes_to_string(msg_buffer, 15 + p9_obj -> uname_len, p9_obj -> aname_len, aname);
			p9_obj -> aname = aname;
			break;
		case P9_TATTACH:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> afid = (uint32_t) buffer_bytes_to_int(msg_buffer, 11, 4);
			p9_obj -> uname_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 15, 2);
			uname = (char *) malloc(p9_obj -> uname_len + 1);
			buffer_bytes_to_string(msg_buffer, 17, p9_obj -> uname_len, uname);
			p9_obj -> uname = uname;
			p9_obj -> aname_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 17 + p9_obj -> uname_len , 2);
			aname = (char *) malloc(p9_obj -> aname_len + 1);
			buffer_bytes_to_string(msg_buffer, 19 + p9_obj -> uname_len, p9_obj -> aname_len, aname);
			p9_obj -> aname = aname;
			break;	
		case P9_RATTACH:
			p9_obj -> qid = decode_qid(msg_buffer, 7, 13); //returns a qid object
			break;
		case P9_RERROR:
			p9_obj -> ename_len = buffer_bytes_to_int(msg_buffer, 7, 2);
			//fprintf(stderr, "ename_len %d\n", p9_obj -> ename_len);//
			p9_obj -> ename = (char *) malloc(p9_obj -> ename_len * sizeof(char) + 1);
			bzero(p9_obj -> ename, p9_obj -> ename_len + 1);
			buffer_bytes_to_string(msg_buffer, 9, p9_obj -> ename_len, p9_obj -> ename);
			break;
		case P9_TERROR:
			break;
		case P9_TFLUSH:
			p9_obj -> oldtag = (uint16_t) buffer_bytes_to_int(msg_buffer, 7, 2);
			break;
		case P9_TWALK:{
			int buffer_index;
			int i, wname_len;
			buffer_index = 17;
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> newfid = (uint32_t) buffer_bytes_to_int(msg_buffer, 11, 4);
			p9_obj -> nwname = (uint16_t) buffer_bytes_to_int(msg_buffer, 15, 2);
			p9_obj -> wname_list = (wname_node *) malloc (sizeof(wname_node) * p9_obj -> nwname);
			for(i = 0; i < p9_obj -> nwname; i++){
				wname_len = buffer_bytes_to_int(msg_buffer, buffer_index, 2);
				((p9_obj -> wname_list) + i) -> wname  = (char *)malloc(wname_len * sizeof(char) + 1);
				buffer_bytes_to_string(msg_buffer, buffer_index + 2, wname_len, ((p9_obj -> wname_list) + i)->wname);
				buffer_index += (2 + wname_len);
			}
			break;
			}//end scope
		case P9_RWALK:
			p9_obj -> nwqid = buffer_bytes_to_int(msg_buffer, 7, 2);
			p9_obj -> wqid = decode_wqid(msg_buffer, p9_obj -> nwqid, 9); //returns a pointer to a pointer to qid
			break;
		case P9_TOPEN:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> mode = msg_buffer[11];
			break;
		case P9_RCREATE:
		case P9_ROPEN:
			p9_obj -> qid = decode_qid(msg_buffer, 7, 13);
			p9_obj -> iounit = buffer_bytes_to_int(msg_buffer, 20, 4);
			break;
		case P9_TCREATE:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> name_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 11, 2);
			name = (char *) malloc(p9_obj -> name_len + 1);
			buffer_bytes_to_string(msg_buffer, 13, p9_obj -> name_len, name);
			p9_obj -> name = name;
			p9_obj -> perm = (uint32_t) buffer_bytes_to_int(msg_buffer, 13 + p9_obj -> name_len, 4);
			p9_obj -> mode = msg_buffer[17 + p9_obj -> name_len];
			break;
		case P9_TREAD:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> offset = (uint64_t) buffer_bytes_to_int(msg_buffer, 11, 8);
			p9_obj -> count = (uint32_t) buffer_bytes_to_int(msg_buffer, 19, 4);
			break;
		case P9_RREAD:
			p9_obj -> count = buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> data = (uint8_t *) malloc(p9_obj -> count * sizeof(uint8_t));
			memcpy(p9_obj -> data, msg_buffer + 11, p9_obj -> count);

			break;
		case P9_TWRITE:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> offset = (uint64_t) buffer_bytes_to_int(msg_buffer, 11, 8);
			p9_obj -> count = (uint32_t) buffer_bytes_to_int(msg_buffer, 19, 4);
			data = (uint8_t *) malloc(p9_obj -> count);
			memcpy(data, msg_buffer + 23, p9_obj -> count);
			p9_obj -> data = data;
			break;
		case P9_RWRITE:
			// will handle later
			break;
		case P9_TCLUNK:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			break;
		case P9_RCLUNK:
			break;
		case P9_TREMOVE:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
            break;
		case P9_RREMOVE:
			// will handle later
			break;
		case P9_TSTAT:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
            break;
		case P9_RSTAT:
			p9_obj -> stat_len = buffer_bytes_to_int(msg_buffer, 7, 2);
			p9_obj -> stat = decode_stat(msg_buffer, 9, p9_obj -> stat_len);
			break;
		case P9_TWSTAT:
			p9_obj -> fid = (uint32_t) buffer_bytes_to_int(msg_buffer, 7, 4);
			p9_obj -> stat_len = (uint16_t) buffer_bytes_to_int(msg_buffer, 11, 2);
			p9_obj -> stat = decode_stat(msg_buffer, 13, p9_obj -> stat_len);
			break;
		case P9_RWSTAT:
			// handle later
			break;
		default:
			break;
	}
}


unsigned long long get_stat_length(stat_t *stat){
	assert(stat->name);
	assert(stat->uid);
	assert(stat->gid);
	assert(stat->muid);
	return (47 + strlen(stat->name) + strlen(stat->uid) + strlen(stat->gid) + strlen(stat->muid)); //without including the size field (very important)
}

/* This converts an RMessage p9 object into a message buffer that can be streamed back to the client */

uint8_t *encode(p9_obj_t *p9_obj){
	uint8_t *msg_buffer;
	int i;
	msg_buffer = (uint8_t *) malloc(p9_obj -> size);
	int_to_buffer_bytes(p9_obj -> size, msg_buffer, 0, 4);
	msg_buffer[4] = p9_obj -> type;
	int_to_buffer_bytes(p9_obj -> tag, msg_buffer, 5, 2);
	switch(p9_obj -> type){
		case P9_RVERSION:
			int_to_buffer_bytes(p9_obj -> msize, msg_buffer, 7, 4);
			int_to_buffer_bytes(p9_obj -> version_len, msg_buffer, 11, 2);
			string_to_buffer_bytes(p9_obj -> version, msg_buffer, 13, p9_obj -> version_len);
			break;
		case P9_RAUTH:
			encode_qid(p9_obj -> aqid, msg_buffer, 7, 13);
			break;
		case P9_RERROR:
			//fprintf(stderr, "before encoding ename_len is %d\n", p9_obj -> ename_len);
			int_to_buffer_bytes(p9_obj -> ename_len, msg_buffer, 7, 2);
			string_to_buffer_bytes(p9_obj -> ename, msg_buffer, 9, p9_obj -> ename_len);
			break;
		case P9_RFLUSH:
			break;
		case P9_RATTACH:
			assert(p9_obj -> qid);
			encode_qid(p9_obj -> qid, msg_buffer, 7, 13);
			break;
		case P9_RWALK:
			int_to_buffer_bytes(p9_obj -> nwqid, msg_buffer, 7, 2);
			for(i = 0; i < p9_obj -> nwqid; i++){
				qid_t *qid;
				assert(p9_obj -> wqid[i] != NULL);
				qid = p9_obj -> wqid[i];
				assert(qid != NULL);
				encode_qid(qid, msg_buffer, 9 + 13*i, 13);
			}
			break;
		case P9_ROPEN:
			encode_qid(p9_obj -> qid, msg_buffer, 7, 13);
			int_to_buffer_bytes(p9_obj -> iounit, msg_buffer, 20, 4);
			break;
		case P9_RCREATE:
			encode_qid(p9_obj -> qid, msg_buffer, 7, 13);
			int_to_buffer_bytes(p9_obj -> iounit, msg_buffer, 20, 4);
			break;
		case P9_RREAD:
			int_to_buffer_bytes(p9_obj -> count, msg_buffer, 7, 4);
			memcpy(msg_buffer + 11, p9_obj -> data, p9_obj -> count);
			break;
		case P9_RWRITE:
			int_to_buffer_bytes(p9_obj -> count, msg_buffer, 7, 4);
			break;
		case P9_RCLUNK:
			break;
		case P9_RREMOVE:
			break;
		case P9_RSTAT:
			assert(p9_obj -> stat);
			int_to_buffer_bytes(p9_obj -> stat_len, msg_buffer, 7, 2);
			encode_stat(p9_obj -> stat, msg_buffer, 9, p9_obj -> stat_len - 2);
			break;
		case P9_RWSTAT:
			break;
		default:
			break;
	}
	return msg_buffer;	
}

/* this function prints the qid in a friendly way */
void print_qid(qid_t *qid){
	/* should print the qid here */
	printf("qid type: %d\n", qid -> type);
	printf("qid version: %d\n", qid -> version);
	printf("qid path: %"PRIu64"\n", qid -> path);
}

void print_stat(stat_t *s){
	printf("stat structure\n");
	printf("atime %d\n", s->atime);
	printf("mtime %d\n", s->mtime);
	printf("name %s\n", s->name);
	printf("gid %s\n", s->gid);
	printf("uid %s\n", s->uid);
	printf("muid %s\n", s->muid);
	printf("mode %"PRIu32"\n", s->mode);
	printf("type: %"PRIu16"\n", s->type);
	printf("length: %"PRIu64"\n", s->length);
	print_qid(s->qid);
}

/* This function prints out information about a p9 object */
/* useful for debugging									  */

void print_p9_obj(p9_obj_t *p9_obj){
	int i;
	printf("**************************************************\n");
	printf("size: %u\n", p9_obj -> size);
	printf("message type: %u\n", p9_obj -> type);
	printf("tag: %u\n", p9_obj -> tag);
	
	switch(p9_obj -> type){
		case P9_RVERSION:
		case P9_TVERSION:
			printf("max size: %u\n", p9_obj -> msize);
			printf("version length: %u\n", p9_obj -> version_len);
			printf("version: %s\n", p9_obj -> version);
			break;
		case P9_TAUTH:
			printf("afid: %u\n", p9_obj -> afid);
			printf("uname_len: %u\n", p9_obj -> uname_len);
			printf("uname: %s\n", p9_obj -> uname);
			printf("aname_len: %u\n", p9_obj -> aname_len);
			printf("aname: %s\n", p9_obj -> aname);
			break;
		case P9_RAUTH:
			printf("aqid: ");
			print_qid(p9_obj -> aqid); /* prints a friendly qid */
			printf("\n");
			break;
		case P9_RERROR:
			printf("ename_len: %u\n", p9_obj -> ename_len);
			printf("ename: %s\n", p9_obj -> ename);
			break;
		case P9_TFLUSH:
			printf("old tag: %u\n", p9_obj -> oldtag);
			break;
		case P9_RFLUSH:
			break;
		case P9_TATTACH:
			printf("fid: %u\n", p9_obj -> fid);
			printf("afid: %u\n", p9_obj -> afid);
			printf("uname_len: %u\n", p9_obj -> uname_len);
			printf("uname: %s\n", p9_obj -> uname);
			printf("aname_len: %u\n", p9_obj -> aname_len);
			printf("aname: %s\n", p9_obj -> aname);
			break;
		case P9_RATTACH:
			printf("qid: ");
			print_qid(p9_obj -> qid);
			printf("\n");
			break;
		case P9_TWALK:
			printf("fid: %u\n", p9_obj -> fid);
			printf("newfid: %u\n", p9_obj -> newfid);
			printf("nwname: %u\n", p9_obj -> nwname);
			for(i = 0; i < p9_obj -> nwname; i++){
				printf("wname: %s\n", (p9_obj->wname_list +i)->wname);
			}
			break;
		case P9_RWALK:
			printf("nwqid: %u\n", p9_obj -> nwqid);
			/* wqid missing here */
			break;
		case P9_TOPEN:
			printf("fid: %u\n", p9_obj -> fid);
			printf("mode: %u\n", p9_obj -> mode);
			break;
		case P9_ROPEN:
			printf("qid: ");
			print_qid(p9_obj -> qid);
			printf("\n");
			printf("iounit: %u\n", p9_obj -> iounit);
			break;
		case P9_TCREATE:
			printf("fid: %u\n", p9_obj -> fid);
			printf("name_len: %u\n", p9_obj -> name_len);
			printf("name: %s\n", p9_obj -> name);
			printf("perm: %u\n", p9_obj -> perm);
			printf("mode: %u\n", p9_obj -> mode);
			break;
		case P9_RCREATE:
			printf("qid: ");
			print_qid(p9_obj -> qid);
			printf("\n");
			printf("iounit: %u\n", p9_obj -> iounit);
			break;
		case P9_TREAD:
			printf("fid: %u\n", p9_obj -> fid);
			printf("offset: %"PRIu64"\n", p9_obj -> offset);
			printf("count: %u\n", p9_obj -> count);
			break;
		case P9_RREAD:
			printf("count: %u\n", p9_obj -> count);
			/* may be print data for sanity check? */
			break;
		case P9_TWRITE:
			printf("fid: %u\n", p9_obj -> fid);
			printf("offset: %"PRIu64"\n", p9_obj -> offset);
			printf("count: %u\n", p9_obj -> count);
			/* maybe print the data */
			break;
		case P9_RWRITE:
			printf("count: %u\n", p9_obj -> count);
			break;
		case P9_TCLUNK:
			printf("fid: %u\n", p9_obj -> fid);
			break;
		case P9_RCLUNK:
			break;
		case P9_TREMOVE:
			printf("fid: %u\n", p9_obj -> fid);
			break;
		case P9_RREMOVE:
			break;
		case P9_TSTAT:
			printf("fid: %u\n", p9_obj -> fid);
			break;
		case P9_RSTAT:
			printf("stat_len: %u\n", p9_obj -> stat_len);
			print_stat(p9_obj -> stat);
			break;
		case P9_TWSTAT:
		case P9_RWSTAT:
			break;
		default:
			break;	
	}
}
