
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int compare( const void *str1, const void *str2){

	//printf("comparing: %s & %s\n", *(char* const*) str1, *(char* const*) str1);

	const char * firstString = *(const char **)str1;
	const char * secondString = *(const char **)str2;

	int compare = (double)strcmp(firstString, secondString);

	if (compare == 0){
		return 0;
	} else if (compare > 0){
		return 1;
	} else {
		return -1;
	}
}

int main(int argc, char *argv[]){

	//check arguments
	if (argc < 2 || argc > 3){
		fprintf(stderr, "%s", "Error: Bad command line parameters\n");
		exit(1);
	}
	char * input_file;
	char *ptr;
  	long wordIndex;	
  	if (argc == 3) { //identify which word to copmare in each line
  	wordIndex = strtol(argv[1], &ptr, 10);
		if (wordIndex < 0){
			wordIndex = wordIndex * -1;
		}

		if (wordIndex == 0){
			fprintf(stderr, "%s", "Error: Bad command line parameters\n");
			exit(1);
		}

		input_file = argv[2];
	} else {
		input_file = argv[1];
		wordIndex = 0;
	}	
	

	//open file
	FILE *fp = fopen(input_file, "r");
	if (fp == NULL){
		fprintf(stderr, "%s %s\n", "Error: Cannot open file", input_file);
		exit(1);
	}

	int maxPerLine = 128;
	char newLine[maxPerLine];
	int numberOfRows = 0;

	while (fgets(newLine, maxPerLine, fp) != NULL){
		if (strlen(newLine) > 126){
			fprintf(stderr, "%s\n", "Line too long");
			exit(1);
		}
		numberOfRows++;
	}

	//printf("%d %s\n", numberOfRows, "total lines");
	rewind(fp);

	int size = numberOfRows * maxPerLine;
	//printf("%d bytes\n", size);

	char **lines = (char**)malloc(sizeof(char) * size);
	if (lines == NULL){
		fprintf(stderr, "malloc failed");
		exit(1);
	}

	char newLine2[maxPerLine];
	numberOfRows = 0;

	while (fgets(newLine2, maxPerLine - 1, fp) != NULL){
		lines[numberOfRows] = (char*) malloc (sizeof(char) * (1 + strlen(newLine2)));
		if (lines[numberOfRows] == NULL){
			fprintf(stderr, "Error: malloc failed\n");
			exit(1);
		}
		strncpy(lines[numberOfRows], newLine2, sizeof(char) * (1 + strlen(newLine2)));
		numberOfRows++;
	}



	//printf("%s", "\n*********Lines Unsorted********\n");
	//int x;	
	//for (x = 0; x < numberOfRows; x++){
	//		printf("%d: %s",  x, lines[x]);
	//}

	char **sortedWords = (char**)malloc(sizeof(char) * size);
	if (sortedWords == NULL){
		fprintf(stderr, "malloc failed");
		exit(1);
	}

	char **unsortedWords = (char**)malloc(sizeof(char) * size);
	if (unsortedWords == NULL){
		fprintf(stderr, "malloc failed");
		exit(1);
	}

	char **sortedLines = (char**)malloc(sizeof(char) * size);
	if (sortedLines == NULL){
		fprintf(stderr, "malloc failed");
		exit(1);
	}
	
	char *buffer = " ";
	int i;
	for (i = 0; i < numberOfRows; i++){
		sortedWords[i] = (char*) malloc (sizeof(char) * (1 + strlen(lines[i])));
		if (sortedWords[i] == NULL){
			fprintf(stderr, "Error: malloc failed\n");
			exit(1);
		}
		strncpy(sortedWords[i], lines[i], sizeof(char) * (1 + strlen(lines[i])));
		int y = 1;
		char *temp = strtok(sortedWords[i], buffer);
		char *wordString = strdup(temp);
		while(y < wordIndex ){
			char *temp = strtok(NULL, buffer);
			if (temp == NULL){
			} else {
				wordString = strdup(temp);
			}
			y++;
		}

		unsortedWords[i] = (char*) malloc (sizeof(char) * (1 + strlen(wordString)));
		if (unsortedWords[i] == NULL){
			fprintf(stderr, "Error: malloc failed\n");
			exit(1);
		}
		strncpy(unsortedWords[i], wordString, sizeof(char) * 1 + strlen(wordString));
		strncpy(sortedWords[i], wordString, sizeof(char) * 1 + strlen(wordString));
	}

 	//use qsort and compare function to sort alphabetically
	qsort(sortedWords, numberOfRows, sizeof(char*), compare);
	int z;
	for (z = 0; z < numberOfRows; z++){
		int y = 0;
		//printf("%d of sorted words = %s\n", z, sortedWords[z]);
		while (y < numberOfRows){

			if (strcmp(unsortedWords[y], sortedWords[z]) == 0){
				sortedLines[z] = (char*) malloc (sizeof(char) * (1 + strlen(lines[y])));
				strncpy(sortedLines[z], lines[y], sizeof(char) * (1 + strlen(lines[y])));
				y = 0;
				break;
			} else {
				y++;
			}
		}
	}

	//printf("%s\n%s\n", "________________", "Lines sorted:");
	int h;
	for (h = 0; h < numberOfRows; h++){
		printf("%s",  sortedLines[h]);
	 }
	int j;
	//free up used memory (all ros)
	for(j = 0; j < numberOfRows; j++){
		free(lines[j]);
		free(sortedWords[j]);
		free(unsortedWords[j]);
		free(sortedLines[j]);
	}

	// //free up used memory (columns)
	free(lines);
	free(sortedLines);
	free(sortedWords);
	free(unsortedWords);

	//close the file

	fclose(fp);
	exit(0);

}

