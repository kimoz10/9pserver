#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <strings.h>
#include <stdlib.h>
#include "threadpool.h"
#include <assert.h>
#include <string.h>
#include "9p.h"
#include "rmessage.h"
#include "fid.h"
#include <stdint.h>
//#include <objLib.h>


void error(char *msg)
{
    perror(msg);
    exit(0);
}

void init_9p_obj(p9_obj_t* obj){
	bzero(obj, sizeof(p9_obj_t));
}

void destroy_p9_obj(p9_obj_t *obj){
    int i;
	obj -> size = -1;
	obj -> tag = -1;
	obj -> msize = -1;
	if(obj -> version != NULL){
		free(obj -> version);
		obj -> version = NULL;
	}

	if(obj -> type == P9_TATTACH){
		free (obj -> aname);
		obj -> aname = NULL;
	}

	if(obj -> type == P9_TATTACH){
		free (obj -> uname);
		obj -> uname = NULL;
	}

	if(obj -> data) {
		free(obj -> data);
		obj -> data = NULL;
	}
	if(obj -> stat){
		destroy_stat(obj -> stat);
		obj -> stat = NULL;
	}
	if(obj -> qid) {
		free(obj -> qid);
		obj -> qid = NULL;
	}
	if(obj -> aqid) {
		free(obj -> aqid);
		obj -> aqid = NULL;
	}
	for(i = 0; i < obj->nwqid; i++){
		if(obj -> wqid[i]) free(obj -> wqid[i]);
	}
	obj -> nwqid = 0;
	if(obj -> wqid) {
		free(obj -> wqid);
		obj -> wqid = NULL;
	}
        if(obj -> wname_list != NULL){
		for(i = 0; i < obj -> nwname; i++){
			if(obj -> wname_list[i].wname) free(obj -> wname_list[i].wname);
		}
        	free(obj -> wname_list);
        	obj -> wname_list = NULL;
	}
	obj -> nwname = 0;

	if(obj -> wname_list) {
		free(obj -> wname_list);
		obj -> wname_list = NULL;
	}
	
	if(obj -> ename){
		free(obj -> ename);
		obj -> ename = NULL;
	}
	obj -> ename_len = 0;
}

void thread_function(void *newsockfd_ptr){
	int fid_count;
	uint8_t* buffer;
	int n;
#ifdef DEBUG
	int i;
#endif
	int attach_flag;
	int newsockfd;
	uint8_t *Rbuffer;
	p9_obj_t *T_p9_obj;
	p9_obj_t *R_p9_obj;
        p9_obj_t *test_p9_obj;
	/* allocate the fid table */
	/* should be persistent through out the connection */
	fid_list **fid_table;
	fid_table = fid_table_init();
	/* end of fid_table allocation */
    assert(fid_table[0] == NULL);
    /* TODO: msize should be the minimum of 9000 and the one offered by the client */
    buffer = (uint8_t *)malloc(9000 * sizeof(char));
   	bzero(buffer, 256);
	T_p9_obj = (p9_obj_t *) malloc (sizeof(p9_obj_t));
	R_p9_obj = (p9_obj_t *) malloc(sizeof(p9_obj_t));
	test_p9_obj = (p9_obj_t *) malloc(sizeof(p9_obj_t));
	init_9p_obj(T_p9_obj);
	init_9p_obj(R_p9_obj);
	init_9p_obj(test_p9_obj);
	newsockfd = *(int *)newsockfd_ptr;
	fid_count = 0;
	attach_flag = 0;
	while(1){
	    int size;
		int read_bytes;
		size = 0;
		n = read(newsockfd, buffer, 4);
		assert(n==4);
                size = buffer_bytes_to_int(buffer, 0, 4);
		assert(size!=0);
		read_bytes = 0;
		while(read_bytes < (size - 4)){
			n = read(newsockfd, buffer + 4 + read_bytes, size - 4 - read_bytes);
			read_bytes += n;
		}
		assert(read_bytes == (size - 4));
#ifdef DEBUG
		for(i = 0; i < n; i++){
			fprintf(stderr, "%d ", buffer[i]);
		}

		fprintf(stderr, "\n");
#endif

		/* decode the buffer and create the T object */
		decode(buffer, T_p9_obj);
		/* Print the T object in a friendly manner */
#ifdef DEBUG
		print_p9_obj(T_p9_obj);
#endif
		
		/* prepare the RMessage */
		prepare_reply(T_p9_obj, R_p9_obj, fid_table);
		/***************************/
		/* ENCODE, PRINT, AND SEND */
		/* encode the RMessage     */
		Rbuffer = encode(R_p9_obj);

		/* Check that the message buffer represents the R object */
#ifdef DEBUG
		decode(Rbuffer, test_p9_obj);
		assert(compare_9p_obj(R_p9_obj, test_p9_obj)==1);
		/* after checking. print and send */
		/* print the R P9 object in a friendly way */
		print_p9_obj(R_p9_obj);
#endif
		/* send the message buffer */
#ifdef DEBUG
		for(i = 0; i < R_p9_obj -> size; i++){
			fprintf(stderr, "%d ", Rbuffer[i]);
		}
		fprintf(stderr, "\n");
#endif
		if((n = write(newsockfd, Rbuffer, R_p9_obj -> size)) == -1){
			fprintf(stderr, "Error while writing to socket...\n");
			exit(1);
		}
		assert(n == R_p9_obj -> size);
		/***************************/
		if(T_p9_obj -> type == P9_TATTACH) attach_flag = 1;
		fid_count = get_fid_count(fid_table);
#ifdef DEBUG
		printf("fid_count_is %d\n", fid_count);
#endif
		destroy_p9_obj(T_p9_obj);
		destroy_p9_obj(R_p9_obj);
		destroy_p9_obj(test_p9_obj);
		free(Rbuffer);
#ifdef DEBUG
		printf("attach flag is %d\n", attach_flag);
#endif
		if((attach_flag == 1) && (fid_count ==0)){
#ifdef DEBUG
			printf("Connection is closing \n");
#endif
			break;
		}
	}
	fid_table_destroy(fid_table);
	free(buffer);
	free(T_p9_obj);
	free(R_p9_obj);
	free(test_p9_obj);
	close(newsockfd);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
	threadpool_t *pool;
	struct sockaddr_in serv_addr, cli_addr;
	if(argc < 2){
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}
	//ObjLib_Init();
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("Error opening socket\n");
		exit(1);
	}
	bzero(&serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		printf("error in binding\n");
		exit(1);
	}
	pool = threadpool_create(2, 10, 0);
	assert(pool!=NULL);
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	while(1){
		int err;
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);
		if(newsockfd < 0)
			exit(1);
		if((err = threadpool_add(pool, &thread_function, (void *)&newsockfd, 0)) != 0){
			printf("Error %d while adding job to the job queue\n", err);
			exit(1);
		}
	}
	//ObjLib_Exit();
	close(sockfd);
	return 0;
}

