#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

//fetch function, edge function
char * (*_fetch_function)(char *);
void (*_edge_function)(char *, char *);

//added functions
int searchForLink(char* link);
void addToDownloads(char *content);
void addToParse(char *prev_link, char* content);
void *parse();
void *download();

typedef struct linkedList_t{
	struct linkedList_t *next;
	char *link;
}linkedList;

typedef struct Node_t{
	struct Node_t *next;
	char *from;
	char *content;
}Node;

typedef struct parseQueue_t{
	Node *head;
	Node *tail;
	int parseSize;
}Queue;

//GLOBAL VARIABLES
Queue *parseQueue;
char **downloadQueue;
int downloadTotal = 0;
int downloadSize = 0;
int downloadPtr = 0;
int used = 0;
int active = 0;

//LIST HEAD POINTER
linkedList *head;

//locks for threads
pthread_mutex_t downloadLock;
pthread_mutex_t parseLock;
pthread_mutex_t listLock;
pthread_mutex_t activeLock;

//condition variables 
pthread_cond_t downloadFill = PTHREAD_COND_INITIALIZER;
pthread_cond_t downloadEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t parseFill = PTHREAD_COND_INITIALIZER;
pthread_cond_t parseEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t workDone =PTHREAD_COND_INITIALIZER;


//****START HERE*****/
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

	//lockS
	pthread_mutex_init(&downloadLock, NULL);
	pthread_mutex_init(&parseLock, NULL);
	pthread_mutex_init(&listLock, NULL);
	pthread_mutex_init(&activeLock, NULL);

	// //size of queues
	downloadSize = queue_size;
	_fetch_function = _fetch_fn;
	_edge_function = _edge_fn;


	//intialize queues
	downloadQueue = malloc(sizeof(char*)*downloadSize);
	parseQueue = malloc(sizeof(Queue));
	if (downloadQueue == NULL || parseQueue == NULL){
		fprintf(stderr, "%s\n", "failed to intialize queues" );
		return -1;
	}

	parseQueue->head = NULL;
	parseQueue->tail = NULL;
	parseQueue->parseSize = 0;

	//printf("beginning here....\n");
	//insert first link
	addToDownloads(start_url);
	searchForLink(start_url);
	//create new thread
	pthread_t pid[parse_workers], cid[download_workers];
	int i;

	for (i = 0; i < parse_workers; i++){
		pthread_create(&pid[i], NULL, parse, NULL);
	}

	for(i = 0; i<download_workers; i++){
		pthread_create(&cid[i], NULL, download, NULL);
	}

	//printf("checking active status\n");
	//continue for all active works, ait UNTIL FINISHED
	pthread_mutex_lock(&activeLock);
	while(active > 0){
		pthread_cond_wait(&workDone, &activeLock);
	}
	pthread_mutex_unlock(&activeLock);

	//printf("******FINISHED******");
  return 0;
}


//SEARCH FOR LINK, 1 IF FOUND, 0 IF NOT FOUND
int searchForLink(char *link){

	///printf("searching for link...");
	pthread_mutex_lock(&listLock);
	linkedList *new = malloc(sizeof(linkedList*));
	linkedList *temp = head;
	linkedList *prev = NULL;
	char *tempLink;
	while(temp != NULL){
		tempLink = temp->link;
		if(tempLink != NULL){

			//found a match
			if(strcmp(tempLink, link) == 0){ 
				free(new);
				//printf("found in list\n");
				pthread_mutex_unlock(&listLock);
				return 1;
			}
		}
		
		prev = temp;
		temp = temp->next;

	}

	//printf("not found...adding\n");
	new->next = NULL;
	new->link = link;
	if(prev == NULL) {
		head = new;
	} else { 
		prev->next = new;
	}

	pthread_mutex_unlock(&listLock);
	return 0;
}

//ADD TO DOWNLOAD QUEUE
void addToDownloads(char *content){
	//printf("adding to downloads");
	downloadQueue[downloadPtr] = content;
	downloadPtr = (downloadPtr + 1) % downloadSize;
	downloadTotal++;

	pthread_mutex_lock(&activeLock);
	active++;
	pthread_mutex_unlock(&activeLock);

	//printf("added\n");
}

//ADD TO PARSE QUEUE
void addToParse(char *lastLink, char* content){
	//Make the node that we will insert
	Node *tmp;
	if((tmp = malloc(sizeof(Node))) == NULL){
		fprintf(stderr, "malloc fail in addToParse");
		exit(2);
	}
	
	//fill and replace
	tmp->next = NULL;
	tmp->content = content;
	tmp->from = lastLink;
	
	//if queue is currently empty
	if(parseQueue->head == NULL){
		parseQueue->head = tmp;
		parseQueue->tail = tmp;
	}
	else{
		(parseQueue->tail)->next = tmp;
		parseQueue->tail = tmp;
	}

	parseQueue->parseSize++;
	
	pthread_mutex_lock(&activeLock);
	active++;
	pthread_mutex_unlock(&activeLock);
}


//PARSER
void *parse(){

	//printf("parsing\n");

	//similar formatting to download, just parsing now
	while(1){

		//printf("here\n");
		pthread_mutex_lock(&parseLock);

		//if parse queue is empty, wait until dowloader gives something
		while(parseQueue->parseSize == 0){
			pthread_cond_wait(&parseEmpty, &parseLock);
		}

		Node *newPage;
		newPage = parseQueue->head;
		parseQueue->head = newPage->next;
		parseQueue->parseSize--;

		pthread_mutex_unlock(&parseLock);
	
			//printf("in parse more");
		char *temp;
		char *copy;
		char *link = NULL;
		char *saveptr = NULL;
		copy = strdup(newPage->content);
		temp = strtok_r(newPage->content, " \n", &saveptr);

		while(temp != NULL){
			
			char *extra = NULL;
			char *copy2 = strdup(temp);
			char *compare = strtok_r(copy2, ":", &extra);
	
			if (compare == NULL){
				link = NULL;
			} else {
				if (strcmp(compare, "link") == 0){
					compare = strtok_r(NULL, " \n", &extra);
					link = compare;
				} else {
					link = NULL;
				}
			}

		   	// printf("link is: %s\n", link);
				if(link != NULL){
				pthread_mutex_lock(&downloadLock);
				while(downloadTotal == downloadSize)
					pthread_cond_wait(&downloadFill, &downloadLock);

				if(!searchForLink(link)){
					addToDownloads(link);//means that the link has not been downloaded before
					pthread_cond_signal(&downloadEmpty);
				}

				pthread_mutex_unlock(&downloadLock);
				_edge_function(newPage->from, link);
			}
			temp = strtok_r(NULL, " \n", &saveptr);
		}
		pthread_mutex_lock(&activeLock);
		active--;
		pthread_cond_signal(&workDone);
		pthread_mutex_unlock(&activeLock);
		//printf("end of parse loop");

		free(newPage);
	}
	return NULL;
}


//DOWNLOADER
void *download(){
	//printf("downloading\n");

	//loop until all pages have been downladed
	while (1){

		//printf("inside download loop\n");
		pthread_mutex_lock(&downloadLock);
		
		while(downloadTotal == 0){
			pthread_cond_wait(&downloadEmpty, &downloadLock);
		}

		//pull link off of download queue
		char *link = downloadQueue[used];
		used = (used + 1) % downloadSize;
		downloadTotal--;

		pthread_cond_signal(&downloadFill);
		pthread_mutex_unlock(&downloadLock);

		char *page = _fetch_function(link);
		//add content to parse queue
		pthread_mutex_lock(&parseLock);
		addToParse(link, page);

		//printf("parse has returned\n");

		pthread_mutex_lock(&activeLock);
		active--;
		pthread_cond_signal(&workDone);
		pthread_mutex_unlock(&activeLock);

		pthread_cond_signal(&parseEmpty);
		pthread_mutex_unlock(&parseLock);

		//printf("end of download loop\n");
	}

}





