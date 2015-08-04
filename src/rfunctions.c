/*
 * rfunctions.c
 *
 *  Created on: Jul 17, 2015
 *      Author: kelghamrawy
 */
#include "9p.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

int permissions(struct stat *st){
	int permissions;
	permissions = 0;
	if(st->st_mode & S_IRUSR) permissions|=0b100000000;
	if(st->st_mode & S_IWUSR) permissions|=0b010000000;
	if(st->st_mode & S_IXUSR) permissions|=0b001000000;
	if(st->st_mode & S_IRGRP) permissions|=0b000100000;
	if(st->st_mode & S_IWGRP) permissions|=0b000010000;
	if(st->st_mode & S_IXGRP) permissions|=0b000001000;
	if(st->st_mode & S_IROTH) permissions|=0b000000100;
	if(st->st_mode & S_IWOTH) permissions|=0b000000010;
	if(st->st_mode & S_IXOTH) permissions|=0b000000001;
	return permissions;
}
void UNIX_stat_to_qid(struct stat *st, qid_t *qid){
	qid->path = st->st_ino;
	qid->version = (st->st_mtime ^ (st->st_size << 8)) & 0; /* may be this will need to reflect any modifications to the file. zero for now */
	qid->type = 0;
	if (S_ISDIR(st->st_mode)) qid->type |= QTDIR;
	if (S_ISLNK(st->st_mode)) qid->type |= QTSYMLINK;
}

void UNIX_stat_to_stat(char* filename, struct stat *st, stat_t *s){
	assert(st != NULL);
	s->name = (char *) malloc(50);
	s->name = strncpy(s->name, filename, 49);
	s->atime = st->st_atime;
	s->mtime = st->st_mtime;
	s->length = st->st_size;
	s->type = 0;
	s->dev = 0;
	s->qid = (qid_t *) malloc(sizeof(qid_t));
	UNIX_stat_to_qid(st, s->qid);
	s -> uid = (char *) malloc(50);
	s -> gid = (char *) malloc(50);
	strncpy(s->uid, (getpwuid(st->st_uid))->pw_name, 49);
	strncpy(s->gid, (getgrgid(st->st_gid))->gr_name, 49);
	/* dont forget to assign muid */
	s -> muid = "";
	/* setting up the mode for the stat */
	s->mode = ((((uint32_t) s->qid->type) << 24) | permissions(st));// & 0x80ffffff;
	/* allowing all permissions for now since eventually it is going to run internally in ESX which can be considered relatively trusted
	 * but this had to change later */
}

void make_qid_from_UNIX_file(const char *pathname, qid_t *qid){
	struct stat *st;
	st = (struct stat *) malloc(sizeof(struct stat));
	lstat(pathname, st);
	UNIX_stat_to_qid(st, qid);
	/* NEW */
	free(st);
}

void make_stat_from_UNIX_file(char *pathname, stat_t *s){
	struct stat *st;
	char *filename;
	st  = (struct stat *) malloc(sizeof(struct stat));
	lstat(pathname, st);
	filename = strrchr(pathname, '/');
	if(filename) filename = filename + 1;
	else filename = pathname;
	UNIX_stat_to_stat(filename, st, s);
	/* NEW */
	free(st);
}

int is_file_exists(char *newpathname){
	return access(newpathname, F_OK);
}

void create_directory(char *pathname, char* filename){
	char *s;
	s = (char *)malloc(1000 * sizeof(char));
	bzero(s, 1000);
	strcat(s, pathname);
	strcat(s, "/");
	strcat(s,filename);
#ifdef DEBUG
	printf("trying to create directory %s\n", s);
#endif
	errno = 0;
	/* TODO: ADD appropriate permissions */
	if(mkdir(s, 0755)!=0){
		printf("error number %d\n", errno);
		exit(1);
	}
	free(s);
}

void create_file(char *pathname, char* filename, int perm){
	char *s;
	int f;
	s = (char *)malloc(1000 * sizeof(char));
	bzero(s, 1000);
	strcat(s, pathname);
	strcat(s, "/");
	strcat(s,filename);
	f = open(s, O_CREAT|O_RDWR, perm);
	close(f);
	free(s);
}

void UNIX_rename_directory(char *path, char *new_name){
	char *new_path;
	new_path = (char *) malloc(1000 * sizeof(char));
	bzero(new_path, 1000);
	strcat(new_path, path);
	strcat(new_path, "../");
	strcat(new_path, new_name);
	rename(path, new_path);
	free(new_path);
}

void UNIX_rename_file(char *path, char *new_name){
	char *last;
	char *new_path;
	int len;
	last = strrchr(path, '/');
	len = (int)(last - path + 1);
	new_path = (char *) malloc(1000 * sizeof(char));
	bzero(new_path, 1000);
	strncat(new_path, path, len);
	strcat(new_path, new_name);
	assert(rename(path, new_path) == 0);
	free(new_path);
}

void UNIX_change_permissions(char *path, uint32_t mode){
	/* TODO: handle this later */

	mode = mode & 0x000001ff;
	chmod(path, mode);

}

int UNIX_read(int fd, uint8_t *data, unsigned long long offset, int count){
	lseek(fd, offset, SEEK_SET);
	return read(fd, data, count);
}


int UNIX_write(int fd, unsigned long long offset, uint8_t *data, int count){
	int n;
	lseek(fd, offset, SEEK_SET);
	n = write(fd, data, count);
	return n;
}

int UNIX_remove(char *path){
	return remove(path);
}
