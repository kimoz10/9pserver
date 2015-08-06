/*
 * rmessage.c
 *
 *  Created on: Jul 17, 2015
 *      Author: kelghamrawy
 */

#include "rmessage.h"
#include "9p.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "fid.h"
#include "rfunctions.h"
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void prepare_reply(p9_obj_t *T_p9_obj, p9_obj_t *R_p9_obj, fid_list **fid_table){
	fid_node *fnode;
	switch(T_p9_obj -> type){
		case P9_TVERSION:
			R_p9_obj -> size = T_p9_obj -> size; /* this is basically the same size as the T message */
			R_p9_obj -> type = P9_RVERSION;
			R_p9_obj -> tag = T_p9_obj -> tag;
			R_p9_obj -> msize = T_p9_obj -> msize;
			R_p9_obj -> version_len = 6;
			R_p9_obj -> version = (char *) malloc(20 * sizeof(char));;
			strcpy(R_p9_obj -> version, "9P2000");
			break;
		case P9_TATTACH:
			R_p9_obj -> size = 20; /* this is the size of the RMessage */
			R_p9_obj -> qid = (qid_t *) malloc (sizeof(qid_t));
			make_qid_from_UNIX_file("/", R_p9_obj -> qid);
			/* adding the entry to the fid table */
			fid_table_add_fid(fid_table, T_p9_obj -> fid, "/");

			R_p9_obj -> tag = T_p9_obj -> tag;
			R_p9_obj -> type = P9_RATTACH;
			break;
		case P9_TSTAT:
			R_p9_obj -> tag = T_p9_obj -> tag;
			R_p9_obj -> type = P9_RSTAT;
			R_p9_obj -> stat = (stat_t *) malloc(sizeof(stat_t));
			fnode = fid_table_find_fid(fid_table, T_p9_obj -> fid);
			if(fnode == NULL){ //stating a file that does not exist in the fid table
				perror("TSTAT a file that is not in the fid table\n");
				exit(1);
			}
			make_stat_from_UNIX_file(fnode -> path, R_p9_obj -> stat);
			if(R_p9_obj -> stat -> qid -> type == 128){
				R_p9_obj -> stat -> length = 0;
			}
			R_p9_obj -> stat_len = get_stat_length(R_p9_obj -> stat) + 2; //the stat length should be the length of the stat + the size
			R_p9_obj -> size = 7 + 2 + R_p9_obj -> stat_len; //stat[n] and the size field
			break;
		case P9_TWALK:
			/* if newfid is in use, an RERROR should be returned. The only exception is when newfid is the same as fid	*/
			/* The case where newfid == fid should be handled separately
			 * 												*/
			if(fid_table_find_fid(fid_table, T_p9_obj -> newfid) != NULL && T_p9_obj -> newfid != T_p9_obj -> fid){
				char *error_msg;
				int error_len;
				R_p9_obj -> type = P9_RERROR;
				error_msg = "newfid is in use and it is not equal to fid\n";
				error_len = strlen(error_msg);
				R_p9_obj -> size = 7 + 2 + error_len;
				R_p9_obj -> ename_len = error_len;
				R_p9_obj -> ename = error_msg;
				R_p9_obj -> tag = T_p9_obj -> tag;
			}
			else{ //newfid is not being used or newfid == fid (should be the same case until we change the fid_table)

				fid_node *fnode;
				fnode = fid_table_find_fid(fid_table, T_p9_obj -> fid);
				if(fnode == NULL || fnode -> fd != -1){
					if(!fnode)perror("TWALK message received with an fid that is open\n");
					else perror("TWALK message received with an fid that does not exist in the fid table");
					exit(1);
				}
				if(T_p9_obj -> nwname == 0){
					fid_table_add_fid(fid_table, T_p9_obj -> newfid, fnode -> path);
					R_p9_obj -> size = 7 + 2;
					R_p9_obj -> type = P9_RWALK;
					R_p9_obj -> tag = T_p9_obj -> tag;
					R_p9_obj -> nwqid = 0;
				}
				else{ //nwname != 0
					/* Check if the first element is walkable, if not an RERROR will return */
					/* first get the number of nwqids */
					char *path;
					int nwqid, i;
					path = (char *)malloc(1000 * sizeof(char));
					strcpy(path, fnode -> path);
					assert(path);
					nwqid = 0;
					for(i = 0; i < T_p9_obj -> nwname; i++){
						strcat(path, "/");
						strcat(path, (T_p9_obj -> wname_list + i) -> wname);
						//fprintf(stderr, "directory path is %s\n", path);
						if(is_file_exists(path)) nwqid++;
					}

					if(nwqid == 0){
						/* First element does not exist. return RERROR */
						char *error_msg;
						int error_len;
						R_p9_obj -> type = P9_RERROR;
						error_msg = "No such file or directory";
						error_len = strlen(error_msg);
						R_p9_obj -> size = 7 + 2 + error_len;
						R_p9_obj -> ename_len = error_len;
						R_p9_obj -> ename = (char *) malloc(error_len * sizeof(char) + 1);
						bzero(R_p9_obj -> ename, error_len + 1);
						strcpy(R_p9_obj -> ename, error_msg);
						R_p9_obj -> tag = T_p9_obj -> tag;
						free(path);
					}
						/* The first element is walkabale. RWALK will return	*/
					else{
						int i;
						bzero(path, 1000);
						strcpy(path, fnode -> path);
						R_p9_obj -> type = P9_RWALK;
						R_p9_obj -> tag = T_p9_obj -> tag;
						R_p9_obj -> size = 7 + 2 + nwqid * 13;
						R_p9_obj -> nwqid = nwqid;
						R_p9_obj -> wqid = (qid_t **) malloc(nwqid * sizeof(qid_t *));
						for(i = 0; i < nwqid; i++){
							qid_t *qid;
							strcat(path, "/");
							strcat(path, (T_p9_obj -> wname_list + i) -> wname);
							qid = (qid_t *) malloc(sizeof(qid_t));
							make_qid_from_UNIX_file(path, qid);
							R_p9_obj -> wqid[i] = qid;

						}
						/* newfid will be affected only if nwqid == nwnames */

						if(nwqid == T_p9_obj -> nwname){
							/* path is now the full path */
							fid_node *fnode;
							fnode = fid_table_find_fid(fid_table, T_p9_obj -> newfid);
							if(fnode != NULL){
								//fid = newfid case
								fnode -> fid = T_p9_obj -> newfid;
								strncpy(fnode -> path, path, 999);
							}
							else
								fid_table_add_fid(fid_table, T_p9_obj -> newfid, path);
						}
						free(path);
					}
				}
			}
			break;
		case P9_TCLUNK:
			/* Should remove the fid from the fid directory */
			/* the remove fid should close any file or directory opened by this fid */
			if(fid_table_remove_fid(fid_table, T_p9_obj -> fid) == -1){
				int error_len;
				char *error_msg;
				perror("TCLUNK received for an fid that does not exist\n ");
				error_msg = strerror(EBADF);
                                error_len = strlen(error_msg);
                                R_p9_obj -> size = 7 + 2 + error_len;
                                R_p9_obj -> ename_len = error_len;
				R_p9_obj -> ename = (char *) malloc(error_len + 1);
				bzero(R_p9_obj -> ename, error_len + 1);
                                strcpy(R_p9_obj -> ename , error_msg);
                                R_p9_obj -> tag = T_p9_obj -> tag;
			}
			else{
				assert(fid_table_find_fid(fid_table, T_p9_obj -> fid) == NULL);
				R_p9_obj -> tag = T_p9_obj -> tag;
				R_p9_obj -> type = P9_RCLUNK;
				R_p9_obj -> size = 7;
			}
			break;
		case P9_TOPEN:
			fnode = fid_table_find_fid(fid_table, T_p9_obj -> fid);
			assert(fnode != NULL);
			assert(fnode -> fid == T_p9_obj -> fid);
			assert(fnode -> fd == -1);
			/* if fid refers to a directory, just send back the ROPEN message */
			/* if fid refers to a file, open the file, change the file descriptor, and send the ROPEN message */
			R_p9_obj -> size = 4 + 1 + 2 + 13 + 4;
			R_p9_obj -> type = P9_ROPEN;
			R_p9_obj -> tag = T_p9_obj -> tag;
			R_p9_obj -> qid = (qid_t *) malloc(sizeof(qid_t));
			make_qid_from_UNIX_file(fnode->path, R_p9_obj -> qid);
			R_p9_obj -> iounit = 0;
			if(R_p9_obj -> qid -> type == 0 ){ //this is a regular file
				int fd;
				fd = -1;
				assert((T_p9_obj -> mode & 0x10) != 0x10 );
				if((T_p9_obj->mode != 0) && (T_p9_obj -> mode != 1) && (T_p9_obj -> mode != 2)){
					printf("UNFAMILIAR MODE %d\n", T_p9_obj->mode);
					exit(1);
				}
				switch(T_p9_obj -> mode){
				case 0:
#ifdef DEBUG
					printf("opening file %s for read only\n", fnode -> path);
#endif
					
					fd = open(fnode -> path, O_RDONLY);
					break;
				case 1:
#ifdef DEBUG
					printf("opening file %s for write only\n", fnode -> path);
#endif
					fd = open(fnode->path, O_WRONLY | O_TRUNC);
					break;
				case 2:
#ifdef DEBUG
					printf("opening file %s for read write\n", fnode -> path);
#endif
					fd = open(fnode -> path, O_RDWR);
					break;
				default:
					printf("mode is different\n");
					printf(" %d\n", T_p9_obj -> mode);
					exit(1);
				}

				fnode -> fd = fd;
				if(fnode -> fd == -1){
					R_p9_obj -> type =  P9_RERROR;
                                        R_p9_obj -> ename = strerror(errno);
                                        R_p9_obj -> ename_len = strlen(R_p9_obj -> ename);
                                        R_p9_obj -> size = 4 + 1 + 2 + 2 + R_p9_obj -> ename_len;
				}
				else assert(fnode -> fd != -1);
			}
			else{ //file is a directory
				fnode -> dd = opendir(fnode -> path);
				if(fnode -> dd == NULL){
					R_p9_obj -> type =  P9_RERROR;
					R_p9_obj -> ename = strerror(errno);
				        R_p9_obj -> ename_len = strlen(R_p9_obj -> ename);
					R_p9_obj -> size = 4 + 1 + 2 + 2 + R_p9_obj -> ename_len;
				}
				/* handle permissions and rw access */
			}
			break;
		case P9_TREAD:
			{//defining a scope in the case statement
			int fid;
			uint8_t *data;
			//fprintf(stderr, "I am here!");
			fid = T_p9_obj -> fid;
			fnode = fid_table_find_fid(fid_table, fid);
			assert(fnode != NULL);
			//fprintf(stderr, "I am here tooooo\n");
			data = (uint8_t *) malloc(T_p9_obj -> count * sizeof(uint8_t));
			//fprintf(stderr, "before zeroing\n");
			bzero(data, T_p9_obj -> count);
			//fprintf(stderr, "after zeroing\n");
			/* handling the directory case */
			assert(fnode);
			//fprintf(stderr, "fd is %d\n", fnode -> fd);
			if(fnode -> fd == -1){ //this must be a directory then
				struct dirent *entry;
				int idx;
				char *newpathname;
				newpathname = (char *) malloc(1000 * sizeof(char));
				idx = 0;
				assert(fnode -> dd);
#ifdef DEBUG
				//fprintf(stderr, "Attempting to read directory %s\n", fnode -> dd);
#endif

				while((entry = readdir(fnode -> dd))){
#ifdef DEBUG
					//fprintf(stderr, "entry is %s\n", entry->d_name);
#endif
					char *entry_name;
					stat_t *s;
					bzero(newpathname, 1000);
					strcpy(newpathname, fnode -> path);
					if((strcmp(entry->d_name, ".") == 0)  || (strcmp(entry->d_name, "..") == 0))
						continue;
					entry_name = entry->d_name;
					newpathname = strcat(newpathname, "/");
					newpathname = strcat(newpathname, entry_name);
					s = (stat_t *) malloc(sizeof(stat_t));
#ifdef DEBUG
					//fprintf(stderr, "Attempting to create stat now\n");
					//fprintf(stderr, "stating pathname: %s\n", newpathname);
#endif
					make_stat_from_UNIX_file(newpathname, s);
					//int_to_buffer_bytes(get_stat_length(s) + 2, data, idx, 2);
					//idx += 2;
					encode_stat(s, data, idx, get_stat_length(s));
					idx += (2 + get_stat_length(s));

					destroy_stat(s);
					/* just a quick hack */
					if(idx > ((90.0 / 100.0) * (float)(T_p9_obj->count))) break; //this is a safety factor to make sure we are not exceeding the Tcount
					assert(idx <= T_p9_obj -> count);
				}
				free(newpathname);
				R_p9_obj -> count = idx;
				R_p9_obj -> data  = data;
				R_p9_obj -> size = 4 + 2 + 4 + 1  + R_p9_obj -> count;
				R_p9_obj -> tag = T_p9_obj -> tag;
				R_p9_obj -> type = P9_RREAD;
			}
			/* handling the file case */
			else{ //assuming it is a directory(however there are things other than files and directories)
				int fd, count, read_bytes;
				//fprintf(stderr, "inside else\n");
				fd = fnode -> fd; /* the file descriptor of the *should be open* file */
				count = T_p9_obj -> count;
				read_bytes = UNIX_read(fd, data, T_p9_obj -> offset, count);
				R_p9_obj -> count = read_bytes;
				R_p9_obj -> data = data;
				R_p9_obj -> size = 4 + 2 + 4 + 1 + R_p9_obj -> count;
				R_p9_obj -> tag = T_p9_obj -> tag;
				R_p9_obj -> type = P9_RREAD;
			}

			break;
			}//ending scope
		case P9_TWRITE:
		{
			int fid, count, write_count;
#ifdef DEBUG
			int i;
#endif
			unsigned long long offset;
			fid_node *fnode;
			R_p9_obj -> size = 11;
			R_p9_obj -> type = P9_RWRITE;
			R_p9_obj -> tag = T_p9_obj -> tag;
			fid = T_p9_obj -> fid;
			offset = T_p9_obj -> offset;
			count = T_p9_obj -> count;
			fnode = fid_table_find_fid(fid_table, fid);
			assert(fnode != NULL);
			assert(fnode -> fd != -1); /* file must be open */
#ifdef DEBUG
			printf("DATA\n");
			for(i = 0; i < T_p9_obj -> count; i++){
				printf("%d ", T_p9_obj -> data[i]);
			}
#endif
			write_count = UNIX_write(fnode -> fd, offset, T_p9_obj -> data, count);
			R_p9_obj -> count = write_count;
			break;
		}//ending scope
		case P9_TCREATE:
			{
			int fid;
			struct stat *s;
			uint32_t perm;
			char *newpathname;
			fid = T_p9_obj -> fid;
			fnode = fid_table_find_fid(fid_table, fid);
			if(fnode == NULL){
				perror("Trying to create a new file in a directory that does not exist in the fid_table\n");
				exit(1);
			}
			s = (struct stat *)malloc(sizeof(struct stat));
			if(lstat(fnode -> path, s)==0){
				if(!S_ISDIR(s->st_mode)){
					perror("The fid belongs to a file not a directory. Can't execute TCREATE\n");
				}
			}
			else{
				perror("cant stat fnode->path\n");
				exit(1);
			}

			perm = T_p9_obj -> perm;
			if((perm & 0x80000000) == 0x80000000){ //this is a directory

				create_directory(fnode->path, T_p9_obj -> name);
  		    }
			else{//a file needs to be create
				create_file(fnode->path, T_p9_obj -> name, perm);
			}
			/* now the newly created file needs to be opened */
			R_p9_obj -> size = 4 + 1 + 2 + 13 + 4;
			R_p9_obj -> type = P9_RCREATE;
			R_p9_obj -> tag = T_p9_obj -> tag;
			R_p9_obj -> qid = (qid_t *) malloc(sizeof(qid_t));
			newpathname = (char *)malloc(1000);
			bzero(newpathname, 1000);
			strcat(newpathname, fnode->path);
			strcat(newpathname, "/");
			strcat(newpathname, T_p9_obj -> name);
#ifdef DEBUG
			printf("ATTEMPTING TO CREATE %s\n", newpathname);
#endif
			/* this fid should represent the newly created file now */
			fnode -> path = newpathname;
			fnode -> dd = 0;
			fnode -> fd = -1;

			make_qid_from_UNIX_file(fnode->path, R_p9_obj -> qid);
			R_p9_obj -> iounit = 0;
			if(R_p9_obj -> qid -> type == 0 ){ //this is a regular file
				int fd;
				fd = -1;
				switch(T_p9_obj -> mode){
					case 0:
						fd = open(fnode -> path, O_RDONLY);
						break;
					case 1:
						fd = open(fnode->path, O_WRONLY);
						break;
					case 2:
						fd = open(fnode -> path, O_RDWR);
						break;
				}
				fnode -> fd = fd;
				assert(fnode -> fd != -1);
			}
			else{ //file is a directory
				fnode -> dd = opendir(fnode -> path);
				/* handle permissions and rw access */
			}
			free(s); //free the temporary allocated stat data structure

			break;
			}//end scope
		case P9_TREMOVE:{
			int fid;
			R_p9_obj -> size = 7;
			R_p9_obj -> type = P9_RREMOVE;
			R_p9_obj -> tag = T_p9_obj -> tag;
			fid = T_p9_obj -> fid;
			fnode = fid_table_find_fid(fid_table, fid);
			assert(fnode != NULL);
			assert(fnode->path != NULL);
			if(UNIX_remove(fnode->path) != 0){
				perror("failed to remove\n");
				exit(1);
			}
			if(fid_table_remove_fid(fid_table, fid) == -1){
				perror("TRREMOVE");
				exit(1);
			}
			assert(fid_table_find_fid(fid_table, T_p9_obj -> fid) == NULL);
			break;
			}//end scope
		case P9_TWSTAT:
		{
			int fid;
			fid_node *fnode;
			stat_t *s_new;
			stat_t *s_old;
			fid = T_p9_obj -> fid;
			s_new = T_p9_obj -> stat;
			fnode = fid_table_find_fid(fid_table, fid);
			if(fnode == NULL){
				perror("writing stat to non existing file\n");
				exit(1);
			}
			s_old = (stat_t *)malloc(sizeof(stat_t));
			make_stat_from_UNIX_file(fnode->path, s_old);
			/* now you have s_new and s_old. Check differences and call the appropriate UNIX api */
			/* it doesn't make any sense to change the type, dev, qid, atime, mtime, muid */
			/* only name, uid, gid, permission part of the mode can be changed
			 *
			 */
			/* it does not make any sense to change the length */
			if((strcmp(s_new -> name, "")!= 0) && (strcmp(s_old -> name, s_new -> name) != 0)){
#ifdef DEBUG
				printf("RENAMING: %s to %s\n", s_old -> name, s_new -> name);
#endif
				if(T_p9_obj -> stat -> qid -> type == 128){
					/* TODO: check permissions */
					UNIX_rename_directory(fnode -> path, s_new -> name);
				}
				else{
					/* TODO: check permission */
					UNIX_rename_file(fnode->path, s_new->name);
				}
			}

			if(s_new -> mode != 0xffffffff && s_old -> mode != s_new -> mode){
				/* only change the permissions */
#ifdef DEBUG
				printf("MODE required to change from %d to %d!\n", s_old -> mode, s_new -> mode);
#endif
				UNIX_change_permissions(fnode->path, s_new -> mode);
			}
			R_p9_obj -> size = 7;
			R_p9_obj -> type = P9_RWSTAT;
			R_p9_obj -> tag = T_p9_obj -> tag;
			/* also gid can be changed but that should be taken care of later */
			destroy_stat(s_old);
			break;
		}//end of scope
		default:
			break;
	};
}
