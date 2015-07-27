/*
 * rfunctions.h
 *
 *  Created on: Jul 17, 2015
 *      Author: kelghamrawy
 */

#ifndef SRC_RFUNCTIONS_H_
#define SRC_RFUNCTIONS_H_

#include "9p.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* creates a qid data structure from a stat datastructure */
void UNIX_stat_to_qid(struct stat *st, qid_t *qid);

/* creates a machine-independent stat datastructure from the UNIX stat datastructure */
void UNIX_stat_to_stat(char* filename, struct stat *st, stat_t *s);

/* given a path name, get the qid of the file */
void make_qid_from_UNIX_file(const char *pathname, qid_t *qid);

/* given a pathname, get the machine independent stat data structure */
void make_stat_from_UNIX_file(const char *pathname, stat_t *s);

/* returns 1 on success. -1 on failure */
int is_file_exists(char *pathname);

void create_directory(char *pathname, char* filename);

void create_file(char *pathname, char* filename, int perm);

/* given a stat of a file, return an integer form of the permission */
int permissions(struct stat *st);

void UNIX_rename_directory(char *path, char *new_name);

void UNIX_rename_file(char *path, char *new_name);

void UNIX_change_permissions(char *path, uint32_t mode);

int UNIX_write(int fd, unsigned long long offset, uint8_t *data, int count);

int UNIX_read(int fd, uint8_t *data, unsigned long long offset, int count);

int UNIX_remove(char *path);

#endif /* SRC_RFUNCTIONS_H_ */
