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

#define DEBUG 1

void error(char *msg)
{
    perror(msg);
    exit(0);
}

void init_9p_obj(p9_obj_t* obj){
	obj -> size = -1;
	obj -> tag = -1;
	obj -> data = NULL;
	obj -> qid = NULL;
	obj -> aqid = NULL;
	obj -> stat = NULL;
	obj -> wqid = NULL;
	obj -> wname_list = NULL;
}

void destroy_p9_obj(p9_obj_t *obj){
        int i;
	obj -> size = -1;
	obj -> tag = -1;
	obj -> msize = -1;
	if(obj -> data) {
		free(obj -> data);
		obj -> data = NULL;
	}
	if(obj -> stat){
		free(obj -> stat);
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

	for(i = 0; i < obj -> nwname; i++){
		if(obj -> wname_list[i].wname) free(obj -> wname_list[i].wname);
	}
	obj -> nwname = 0;
	if(obj -> wname_list) {
		free(obj -> wname_list);
		obj -> wname_list = NULL;
	}
}

void thread_function(void *newsockfd_ptr){
	uint8_t* buffer;
	int n;
#ifdef DEBUG
	int i;
#endif
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
        buffer = (uint8_t *)malloc(9000 * sizeof(char));
   	bzero(buffer, 256);
	T_p9_obj = (p9_obj_t *) malloc (sizeof(p9_obj_t));
	R_p9_obj = (p9_obj_t *) malloc(sizeof(p9_obj_t));
	test_p9_obj = (p9_obj_t *) malloc(sizeof(p9_obj_t));
	init_9p_obj(T_p9_obj);
	init_9p_obj(R_p9_obj);
	init_9p_obj(test_p9_obj);
	newsockfd = *(int *)newsockfd_ptr;
	while((n = read(newsockfd, buffer, 9000))!=0){
#ifdef DEBUG
		for(i = 0; i < n; i++){
			//printf(stderr, "%d ", buffer[i]);
		}

		//fprintf(stderr, "\n");
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
			//fprintf(stderr, "%d ", Rbuffer[i]);
		}
		//fprintf(stderr, "\n");
#endif
		if(write(newsockfd, Rbuffer, R_p9_obj -> size) == -1){
			fprintf(stderr, "Error while writing to socket...\n");
			exit(1);
		}
		/***************************/
		destroy_p9_obj(T_p9_obj);
		destroy_p9_obj(R_p9_obj);
		destroy_p9_obj(test_p9_obj);
		free(Rbuffer);
	}
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
	pool = threadpool_create(64, 1000, 0);
	assert(pool!=NULL);
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	while(1){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);
		if(newsockfd < 0)
			exit(1);
		threadpool_add(pool, &thread_function, (void *)&newsockfd, 0);
	}
	return 0;
}

