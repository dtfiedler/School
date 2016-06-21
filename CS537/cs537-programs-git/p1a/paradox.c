/**
 * auther:  Xuyi Ruan 
 * date:    09/07/2014
 * Decription: This program is to simulate the birthday paradox, the program 
 * take two parameters as input and return the percentage chance of have two 
 * same birthdays in different random genreate group of people. 
 *
 */


/*
 * Main method
 * Arguments: NA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
	// error check for the input arguments = 5?
	if (argc != 5) 
	{
		fprintf(stderr, "Usage: paradox -i inputfile -o outputfile\n");
		exit(1);
	}
	// set the 2nd and 4th argument as inputfileName and outputfile Names.
	
	int ch; // the choice for the option in commnad line

	char *inFile = NULL;
	char *outFile = NULL;

	while ((ch = getopt(argc, argv, "i:o:")) != -1)
        {
                switch (ch) {
                        case 'i':
                                inFile = optarg;
                                break;
                        case 'o':
                                outFile = optarg;
                                break;
                        default:
                                fprintf(stderr, "Usage: paradox -i inputfile -o outputfile\n");
				exit(1);
                }
        }

	
	int random;
        FILE *fp, *ofp;
	// set up File pointer for both read/write file
	fp = fopen(inFile, "r");
	ofp = fopen(outFile, "w");

	if (fp == NULL) 
	{	
		fprintf(stderr, "Cannot open file: %s\n", inFile);
		exit(1);
	}
	
	// error check for output file
	if (ofp == NULL)
	{
		fprintf(stderr, "Cannot open file: %s\n", outFile);
		exit(1);
	}
	

	// inArr contains the list of numbers of person to participate in the 
	// paradox test.
	
	int inArr[1000];
	int index = 0;
	while (fscanf(fp, "%d", &random) != EOF) 
	{
		inArr[index] = random;
		index++;
	}

	// the code below starts to generate the test for birthday paradox
	//
	// 09/10 night
	////////////////////////////////////////////
	float outArr[index]; // ?? not sure if i can access index
	int curr = 0;
	//set up random number generate method by using the current time as seed.
	srand((unsigned)time(NULL));
	
	while (curr < index) 
	{
		int randomNum = inArr[curr];
		int *randomDate;
		randomDate = malloc(sizeof(int)*randomNum);
		if (randomDate == NULL)
		{
			fprintf(stderr, "Error to allocate memory space!");
			return 1;
		}
		// generate random birthday
		int i, j;
		int trials = 1;
		int count = 0;
		while (trials <= 1000) {
			for (i = 0; i < randomNum; i++)
                	{
                        	randomDate[i] = rand() % 365;
                	}
			for (i = 0; i < randomNum-1; i++)
			{
				for (j = i+1; j < randomNum; j++)
				{
					if (randomDate[i] == randomDate[j])
					{
						count++;
						i = j = randomNum;
						break;
					}
				}
			}
			// one trail finished
			trials++;
		}
		float result = (float)count/1000;
		// the below code store the results temperary into an array.
		// modified 9/12 night
		outArr[curr] = result;
		curr++;
		// free the requested memory space
		free(randomDate);	
	}
	
	// write all the elements in the outArr array out to the file
	
	int i;
	for (i = 0; i < index; i++) 
	{
		fprintf(ofp, "%.2f\n", outArr[i]);
	}
		
	fclose(fp);
	fclose(ofp);
	return 0;
}
