/*
 * fid.c
 *
 *  Created on: Jul 20, 2015
 *      Author: kelghamrawy
 */
#include "fid.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

struct fid_list *create_fid_list(){
	fid_list *flist;
	flist = (fid_list *) malloc(sizeof(fid_list));
	flist -> head = NULL;
	flist -> tail = NULL;
	return flist;
}

void add_fid_node(struct fid_list *flist, uint32_t fid, char *path){
	fid_node *fnode;
	fnode = create_fid_node(fid, path);
	if(flist -> head == NULL){
		flist -> head = flist -> tail = fnode;
	}
	else{
		flist -> tail -> next = fnode;
	}
}

/* on success, element is removed and 0 is returned
 * on failure (element not found), -1 is returned
 */

int remove_fid_from_list(struct fid_list *flist, uint32_t fid){
	if(flist == NULL || flist -> head == NULL) return -1;
	fid_node *temp;
	/* if it is the first element of the list. remove it and change the flist head */
	if(flist -> head -> fid == fid){
		temp = flist -> head;
		flist -> head = flist -> head -> next;
		if(flist -> head == NULL) flist -> tail = NULL;
		free(temp);
		return 0;
	}
	fid_node *current;
	fid_node *prev;
	prev = flist -> head;
	current = flist -> head -> next;
	while(current != NULL){
		if(current -> fid == fid){
			prev -> next = current -> next;
			free(current);
			return 0;
		}
		current = current -> next;
		prev = prev -> next;
	}
	return -1;
}

fid_node *find_fid_node_in_list(struct fid_list *flist, uint32_t fid){
	if(flist == NULL || flist -> head == NULL) return NULL;
	fid_node *current = flist -> head;
	while(current != NULL){
		if(current -> fid == fid) return current;
	}
	return NULL;
}

fid_node *create_fid_node(uint32_t fid, char* path){
	fid_node *fnode;
	fnode = (fid_node *) malloc (sizeof(fid_node));
	fnode -> fid = fid;
	fnode -> path = path;
	fnode -> fd = -1;
	fnode -> dd = 0;
	fnode -> next = NULL;
	return fnode;
}

/* the fid_table functions */

/* creates and initializes the fid_table */
fid_list **fid_table_init(){
	fid_list  **fid_table = (fid_list **) malloc(HTABLE_SIZE * sizeof(fid_list *));
	for(int i = 0; i < HTABLE_SIZE; i++){
		fid_table[i] = NULL;
	}
	return fid_table;
}
void fid_table_add_fid(fid_list **fid_table, uint32_t fid, char* path){
	if(fid_table == NULL){
		perror("Attempting to add to a null pointer fid_table\n");
		exit(1);
	}
	int entry = fid % HTABLE_SIZE;
	if(fid_table[entry] != NULL)printf("entry is %d\n", entry);
	/* handling the case when there is no fid_list in the entry */


	if(fid_table[entry] == NULL){
		fid_list *flist = create_fid_list(flist);
		add_fid_node(flist, fid, path);
		fid_table[entry] = flist;
	}

	else{
		add_fid_node(fid_table[entry], fid, path);
	}
}

struct fid_node *fid_table_find_fid(fid_list **fid_table, uint32_t fid){
	int entry = fid % HTABLE_SIZE;
	if(fid_table[entry] == NULL) return NULL;
	return find_fid_node_in_list(fid_table[entry], fid);
}

/* returns 0 on success. -1 on failure or if element is not found */
/* This should also close any open file/directory opened by this fid */
int fid_table_remove_fid(fid_list **fid_table, uint32_t fid){
	int entry = fid % HTABLE_SIZE;
	if(fid_table[entry] == NULL) return -1;
	fid_node *fnode = find_fid_node_in_list(fid_table[entry], fid);
	if(fnode == NULL) return -1;
	struct stat s;
	if(stat(fnode -> path, &s) == 0){
		if (S_ISDIR(s.st_mode)){
			if(fnode -> dd != 0){
				closedir(fnode -> dd);
				fnode -> dd = 0;
			}
		}
		else if(S_ISREG(s.st_mode)){
			if(fnode -> fd != -1){
				close(fnode -> fd);
				fnode -> fd = -1;
			}
		}
	}
	return remove_fid_from_list(fid_table[entry], fid);
}
