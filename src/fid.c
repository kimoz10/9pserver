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
#include <string.h>
struct fid_list *create_fid_list(){
	fid_list *flist;
	flist = (fid_list *) malloc(sizeof(fid_list));
	flist -> head = NULL;
	flist -> tail = NULL;
	return flist;
}

void fid_list_destroy(fid_list *flist){
    fid_node *current;
	current = flist -> head;
	//printf("destroying list\n");
	while(current != NULL){
		fid_node *temp;
		temp = current;
		current = current -> next;
		//printf("destroying node\n");
		free(temp -> path);
		free(temp);
	}
	free(flist);
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
	fid_node *temp;
	fid_node *current;
	fid_node *prev;
	if(flist == NULL || flist -> head == NULL) return -1;
	/* if it is the first element of the list. remove it and change the flist head */
	if(flist -> head -> fid == fid){
		temp = flist -> head;
		flist -> head = flist -> head -> next;
		if(flist -> head == NULL) flist -> tail = NULL;
		if(temp -> path != NULL){
			free(temp -> path);
			temp -> path = NULL;
		}
		free(temp);
		return 0;
	}
	prev = flist -> head;
	current = flist -> head -> next;
	while(current != NULL){
		if(current -> fid == fid){
			prev -> next = current -> next;
			free(current -> path);
			free(current);
			return 0;
		}
		current = current -> next;
		prev = prev -> next;
	}
	return -1;
}

fid_node *find_fid_node_in_list(struct fid_list *flist, uint32_t fid){
	fid_node *current;
	if(flist == NULL || flist -> head == NULL) return NULL;
	current = flist -> head;
	while(current != NULL){
		if(current -> fid == fid) return current;
	}
	return NULL;
}

fid_node *create_fid_node(uint32_t fid, char* path){
	fid_node *fnode;
	fnode = (fid_node *) malloc (sizeof(fid_node));
	fnode -> fid = fid;
	fnode -> path = (char *) malloc(1000);
	strncpy(fnode->path, path, 999);
	fnode -> fd = -1;
	fnode -> dd = 0;
	fnode -> next = NULL;
	return fnode;
}

/* the fid_table functions */

/* creates and initializes the fid_table */
fid_list **fid_table_init(){
	fid_list  **fid_table;
	int i;
	fid_table = (fid_list **) malloc(HTABLE_SIZE * sizeof(fid_list *));
	for(i = 0; i < HTABLE_SIZE; i++){
		fid_table[i] = NULL;
	}
	return fid_table;
}

void fid_table_destroy(fid_list **fid_table){
	int i;
	for(i = 0; i < HTABLE_SIZE; i++){
		if(fid_table[i] != NULL){
			//printf("destroying fid table... \n");
			fid_list_destroy(fid_table[i]);
		}
	}
	free(fid_table);
	fid_table = NULL;
}

void fid_table_add_fid(fid_list **fid_table, uint32_t fid, char* path){
	int entry;
	if(fid_table == NULL){
		perror("Attempting to add to a null pointer fid_table\n");
		exit(1);
	}
	entry = fid % HTABLE_SIZE;
	/* handling the case when there is no fid_list in the entry */
	if(fid_table[entry] == NULL){
		fid_list *flist;
		flist = NULL;
		flist = create_fid_list();
		add_fid_node(flist, fid, path);
		fid_table[entry] = flist;
	}

	else{
		add_fid_node(fid_table[entry], fid, path);
	}
}

struct fid_node *fid_table_find_fid(fid_list **fid_table, uint32_t fid){
	int entry;
	entry = fid % HTABLE_SIZE;
	if(fid_table[entry] == NULL) return NULL;
	return find_fid_node_in_list(fid_table[entry], fid);
}

/* returns 0 on success. -1 on failure or if element is not found */
/* This should also close any open file/directory opened by this fid */
int fid_table_remove_fid(fid_list **fid_table, uint32_t fid){
	int entry;
	fid_node *fnode;
	struct stat s;
	entry = fid % HTABLE_SIZE;
	if(fid_table[entry] == NULL) return -1;
	fnode = find_fid_node_in_list(fid_table[entry], fid);
	if(fnode == NULL) return -1;
	if(lstat(fnode -> path, &s) == 0){
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
