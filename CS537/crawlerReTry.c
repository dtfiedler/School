#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include <time.h>


//GLOBAL VARIABLES
#define HASHSIZE 15

//fetch function, edge function
char *(*fetchfn)(char *url);
void (*edgefn)(char *from, char *to);

typedef struct linkedList{
	struct linkedList *next;
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
} Queue;



Queue *parseQueue;
char **downloadQueue;
int downloadTotal = 0;
int downloadSize;
int downloadPtr = 0;
int active = 0;
int duseptr = 0;

linkedList *head;

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
int searchForLink(char *link){

	pthread_mutex_lock(&listLock);
	linkedList *new = malloc(sizeof(linkedList));
	linkedList *temp = head;
    linkedList *prev = NULL;
	char *tempLink;

	while(temp != NULL){
        tempLink = temp->link;
        if (tempLink != NULL){
            if (strcmp(tempLink, link) == 0) {
                //found link in list, unlock and return 1
                pthread_mutex_unlock(&listLock);
                return 1;
            }  else {
                prev = temp;
                temp = temp->next;
            }
       }
        
        new->link = link;
        new->next = NULL;
        head->next = new;
    }
    
	//didn't find link in list, add at beginning
    if (prev == NULL){
        head = new;
    } else {
        prev->next = new;
    }

	pthread_mutex_unlock(&listLock);

	//didn't find link in list
	return 0;
    
}

void addToDownloads(char *content){
    downloadQueue[downloadPtr] = content;
    downloadPtr = (downloadPtr + 1) % downloadSize;
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
    temp->from = link;
    
    if (parseQueue->head == NULL){
        parseQueue->head = temp;
        parseQueue->tail = temp;
    } else {
        (parseQueue->tail)->next = temp;
        parseQueue->tail = temp;
    }
    
    parseQueue->parseSize++;
    
    pthread_mutex_lock(&activeLock);
    active++;
    pthread_mutex_lock(&activeLock);
    
}



//added functions
void *download(){
    
    printf("downloading...");
	//aquire link lock

	//loop until all pages have been downladed
	while (1){
		pthread_mutex_lock(&downloadLock);
		//aquire links lock and add to page queue
		while(downloadTotal == 0){
			pthread_cond_wait(&downloadEmpty, &downloadLock);
		}
        
        printf("****inside here ********");



        char*  link = downloadQueue[duseptr];
        duseptr = (duseptr + 1) % downloadSize;
        downloadTotal--;

        printf("****over here****");


		//signal that link queue is empty and release lock
		pthread_cond_signal(&downloadFill);
		pthread_mutex_unlock(&downloadLock);
        
        

		//add to parse and
		char *newPage = fetchfn(link);
        
        printf("*****whats up *********");

		pthread_mutex_lock(&parseLock);
		addToParse(link, newPage);

		pthread_mutex_lock(&activeLock);
		active--;
		pthread_cond_signal(&workDone);
		pthread_mutex_unlock(&activeLock);

		//let parses know to start parsing
		pthread_cond_signal(&parseEmpty);
		pthread_mutex_unlock(&parseLock);

	}

}

char *checkLink(char *link){
    
    printf("checking link");
    
        char *extra = NULL;
        if(link == NULL)
            return NULL;
        char *linkCopy = strdup(link);
        char *temp = strtok_r(linkCopy, ":", &extra);
        if(temp == NULL)
            return NULL;
        if(strcmp(temp, "link") == 0){
            temp = strtok_r(NULL, " \n", &extra);
            return temp;
        }
        return NULL;
}

Node *getParsed(){
    
    Node *temp = parseQueue->head;
    parseQueue->head = temp->next;
    parseQueue->parseSize--;
    
    return temp;
    
    
}

void extraParse(char* prev_link, char* page){
    char *tmp, *link, *copy;
    char *saveptr = NULL;
    copy = strdup(page);
    tmp = strtok_r(page, " \n", &saveptr);
    
    while(tmp != NULL){
        if((link = checkLink(tmp)) != NULL){
            pthread_mutex_lock(&downloadLock);
            while(downloadTotal == downloadSize)
                pthread_cond_wait(&downloadFill, &downloadLock);
            
            if(searchForLink(link)){
                addToDownloads(link);//means that the link has not been downloaded before
                pthread_cond_signal(&downloadEmpty);
            }
            
            pthread_mutex_unlock(&downloadLock);
            edgefn(prev_link, link);
            
        }
        tmp = strtok_r(NULL, " \n", &saveptr);
    }
    
    /////////
    pthread_mutex_lock(&activeLock);
    active--;
    pthread_cond_signal(&workDone);
    pthread_mutex_unlock(&activeLock);
    
}

void *parse(){
    
    printf("now parsing...\n");
    Node *newPage = malloc(sizeof(Node));

    newPage->next = parseQueue->head;
	//similar formatting to download, just parsing now
	while(1){

		pthread_mutex_lock(&parseLock);
//
		while(parseQueue->parseSize == 0){
			pthread_cond_wait(&parseEmpty, &parseLock);
		}
        parseQueue->head = newPage->next;
        parseQueue->parseSize--;
        
        pthread_mutex_unlock(&parseLock);

        printf("....");
       extraParse(newPage->from, newPage->content); //will implement the pthread_wait within

		//parse 
		// char *saveptr = NULL;
		// char *copy = strdup(page->content);
		// char *token = strtok_r(copy, " \n", &saveptr);
  //      char *newLink = checkLink(token);
       
		// while(token != NULL){

           
		// 	if (newLink != NULL){

		// 		pthread_mutex_lock(&downloadLock);
		// 		while(downloadTotal == downloadSize){
		// 			pthread_cond_wait(&downloadFill, &downloadLock);
		// 		}

		// 		if (searchForLink(newLink)){ //returns 1 if link hasn't been download
		// 			addToDownloads(newLink);
		// 			pthread_cond_signal(&downloadEmpty);
		// 		}

		// 		pthread_mutex_unlock(&downloadLock);
		// 		edgefn(page->from, newLink);
		// 	}

		// 	token = strtok_r(NULL, " \n", &saveptr);
		// }

		// pthread_mutex_lock(&activeLock);
		// active--;
		// pthread_cond_signal(&workDone);
		// pthread_mutex_unlock(&activeLock);
        
        ///free(page);
    }
    return NULL;
}


//****START HERE*****/
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

    printf("starting...\n");
	//locks
	pthread_mutex_init(&downloadLock, NULL);
	pthread_mutex_init(&parseLock, NULL);
	pthread_mutex_init(&listLock, NULL);
	pthread_mutex_init(&activeLock, NULL);

	//size of queues
	downloadSize = queue_size;

	//intialize queues
	downloadQueue = malloc(sizeof(char*)*downloadSize);
	parseQueue = malloc(sizeof(Queue));
    
	if (downloadQueue == NULL || parseQueue == NULL){
		return -1;
	}

	parseQueue->head = NULL;
	parseQueue->tail = NULL;
	parseQueue->parseSize = 0;
    
	//insert first link
    printf("inserting first link...\n");

    addToDownloads(start_url);
    searchForLink(start_url);

	//create new thread
	pthread_t pid[parse_workers], cid[download_workers];
	int i;
    
    for (i = 0; i < parse_workers; i++){
        pthread_create(&pid[i], NULL, parse, NULL);
    }
    
    for(i = 0; i < download_workers; i++){
        pthread_create(&cid[i], NULL, download, NULL);
    }
    


	//continue for all active works, wait UNTIL FINISHED
	pthread_mutex_lock(&activeLock);
	while(active > -1){
		pthread_cond_wait(&workDone, &activeLock);
	}
//
	pthread_mutex_unlock(&activeLock);
    


  return 0;
}



