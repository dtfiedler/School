#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>


//GLOBAL VARIABLES
#define HASHSIZE 15

//fetch function, edge function
char *(*fetchfn)(char *url);
void (*edgefn)(char *from, char *to);

typedef struct pages {
	char *from;
	char *content;
} pages;

typedef struct linkedList_t{
	struct linkedList *next;
	char *link;
}linkedList;

linkedList *head;

typedef struct Node_t{
	struct Node_t *next;
	char *from;
	char *content;
} Node;

typedef parseQueue_t{
	Node *head;
	Node *tail;
	int size;
} queue;


//parse Queue;
Queue *parseQueue;
char **downloadQueue;
int downloadTotal = 0;
int downloadSize;
int downloadPtr = 0;

int active;

//locks for threads
pthread_mutex_t downloadLock;
pthread_mutex_t parseLock;
pthread_mutex_t listLock;
pthread_mutex_t activeLock;

//condition variables to use
pthread_cond_t downloadFill = PTHREAD_COND_INITIALIZER;
pthread_cond_t downloadEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t parseFill = PTHREAD_COND_INITIALIZER;
pthread_cond_t parseEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t workDone =PTHREAD_COND_INITIALIZER;


//search if link already exists in our linked list
//return 1 if link does not exist, 0 if it does
int *searchForLink(char *link){

	pthread_mutex_lock(&listLock);
	linkedList *new = malloc(sizeof(linkedList));
	linkedList *temp = head;
	char *tempLink;

	while(temp != NULL){
		
		if (strcmp(tempLink, link)) == 0 {
			//found link in list, unlock and return 1
			pthread_mutext_unlock(*listLock);
			return 1;
		}  else {
			prev = temp;
			temp = temp->next;
		}
	}

	//didn't find link in list, add at beginning
	new->link = link;
	new->next = head;
	head->next = new;

	pthread_mutext_unlock(&listLock);

	//didn't find link in list
	return 0;
}


//added functions
void *download(){
	//aquire link lock

	//loop until all pages have been downladed
	while (1){
		pthread_mutex_lock(&downloadLock);
		//aquire links lock and add to page queue
		while(topQueue < 0){
			pthread_cond_wait(&downloadQueue, &downloadLock);
		}


		char *link = downloadQueue[downloadPtr];
		downloadPtr--;
		downloadTotal--;

		//signal that link queue is empty and release lock
		pthread_cond_signal(&downloadFill);
		pthread_mutex_unlock(&downloadLock);


		//add to parse and
		char *newPage = fetchfn(link;
		pthread_mutex_lock(&parseLock);
		addToParse(link, page);

		pthread_mutext_loxk(&activeLock);
		active--;
		pthread_cond_signal(&workDone);
		pthread_mutex_unlock(&activeLock);

		//let parses know to start parsing
		pthread_cond_signal(&parseEmpty);
		pthread_mutex_unlock(*parseLock);

	}

}

void *parse(void *ptr){

	//similar formatting to download, just parsing now



	while(1){

		pthread_mutex_lock(&parseLock);

		while(pasreQueue->size < 1){
			pthread_cond_wait(&parseEmpty, &parseLock);
		}

		pthread_mutext_unlock(&parseLock);

		Node *page = parseQueue->head;
		parseQueue->head = page->next;
		parseQueue->size--;

		//parse 
		char *saveptr = NULL;
		char *copy = strdup(page);
		char *temp = strtok_r(page, " \n", &saveptr);
		char *keyWord = "link:";

		while(temp != NULL){
		
			if (strcmp(keyWord, temp) == 0){

				pthread_mutext_lock(&downloadLock);
				while(downloadTotal == downloadSize){
					pthread_cond_wait(&downloadFill, &downloadLock);
				}

				if (searchForLink(link)){ //returns 1 if link hasn't been download
					addToDownloads(link);
					pthread_cond_signal(&downloadEmpty);
				}

				pthread_mutext_unlock(&downloadLock);
				edgefn(page->from, link);
			}

			temp = strtok_r(NULL, " \n", &saveptr);
		}

		pthread_mutext_lock(&activeLock);
		active--;
		pthread_cond_signal(&workDone);
		pthread_mutex_unlock(&activeLock);
}

void addToDownloads(char *content){
	downloadQueue[downloadPtr] = content;
	downloadPtr = (downloadPtr + 1) $ downloadSize;
	downloadTotal++;

	pthread_mutex_lock(&activeLock);
	active++;
	pthread_mutex_unlock(&activeLock);
}

void addToParse(char *link, char *content){
	Node *temp = malloc(sizeof(Node));

	if (temp == NULL){
		exit(1);
	}

	temp->next = NULL;
	temp->content = content;
	temp->parent = link;

	if (parseQueue->head == NULL){
		parseQueue->head = temp;
		parseQueue->tail = temp;
	} else {
		parseQueue->tail->next = temp;
		pasrseQueue->tail = temp;
	}

	parseQueue->size++;

	pthread_mutext_lock(&activeLock);
	active++;
	pthread_mutext_lock(&activeLock);

}

//****START HERE*****/
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

	//locks
	pthread_mutex_init(&linkQueueLock, NULL);
	pthread_mutex_init(&pageQueueLock, NULL);
	pthread_mutex_init(&hashTableLock, NULL);
	pthread_mutex_init(&activeWorksLock, NULL);

	//size of queues
	maxSize = queue_size;

	//intialize queues
	downloadQueue = malloc(sizeof(char*)*downloadSize));
	parseQueue = malloc(sizeof(Queue));
	if (downloadQueue == NULL || parseQueue == NULL){
		return -1;
	}

	parseQueue->head = NULL;
	parseQueue->tail = NULL;
	parseQueue->size = 0;

	//insert first link



	//create new thread
	pthread_t pid[parse_workers], cid[download_workers];
	int i;

	for (i = 0; i < parse_workers; i++){
		pthread_create(&pid[i], NULL, parse, NULL);
	}

	for(i = 0; i<download_workers; i++){
		pthread_create(&cid[i], NULL, download, NULL);
	}

	//continue for all active works, wait UNTIL FINISHED
	pthread_mutex_lock(&activeWorksLock);
	while(active > 0){
		pthread_cond_wait(&activeWorksEmpty, &activeWorksLock);
	}

	pthread_mutex_unlock(&activeWorksLock);

  return 0;
}



