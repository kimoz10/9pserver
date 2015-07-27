/*
 * fid.h
 *
 *  Created on: Jul 20, 2015
 *      Author: kelghamrawy
 */

#ifndef SRC_FID_H_
#define SRC_FID_H_

#define HTABLE_SIZE 1000
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>

/* define the fid hash table here */
struct fid_list **fid_table;
struct fid_node{
	uint32_t fid;
	char *path;
	int fd; /* UNIX file descriptor */
	DIR *dd; /* UNIX directory descriptor */
	struct fid_node *next;
};

struct fid_list{
	struct fid_node *head;
	struct fid_node *tail;
};

typedef struct fid_node fid_node;
typedef struct fid_list fid_list;

/* define fid_list functions */
struct fid_list *create_fid_list();
void add_fid_node(struct fid_list *flist, uint32_t fid, char *path);
int remove_fid_from_list(struct fid_list *flist, uint32_t fid);
struct fid_node *create_fid_node(uint32_t fid, char* path);
struct fid_node *find_fid_node_in_list(struct fid_list *flist, uint32_t fid);
/* define the fid table functions */

fid_list **fid_table_init();
void fid_table_add_fid(struct fid_list **fid_table, uint32_t fid, char* path);
struct fid_node *fid_table_find_fid(struct fid_list **fid_table, uint32_t fid);
int fid_table_remove_fid(struct fid_list **fid_table, uint32_t fid);

#endif /* SRC_FID_H_ */
