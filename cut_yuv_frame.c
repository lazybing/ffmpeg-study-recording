#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *InputFileName  = NULL;
static char *OutputFileName = NULL;
static FILE *fdInputFile    = NULL;
static FILE *fdOutputFile   = NULL;
static int frameWidth       = 0;
static int frameHeight      = 0;
static int startFrame       = -1;
static int endFrame         = -1;

void parse_argv(int argc, char **argv)
{
    int i;

    for(i = 1; i < argc; i++){
        printf("argv %d:%s %s\n", i, argv[i], argv[i+1]);
        if(!strcmp(argv[i], "-i")){
            i++;
            InputFileName = argv[i];
        }
        if(!strcmp(argv[i], "-o")){
            i++;
            OutputFileName = argv[i];
        }
        if(!strcmp(argv[i], "-w")){
            i++;
            frameWidth = atoi(argv[i]);
        }

        if(!strcmp(argv[i], "-h")){
            i++;
            frameHeight = atoi(argv[i]);
        }

        if(!strcmp(argv[i], "-s")){
            i++;
            startFrame = atoi(argv[i]);
        }

        if(!strcmp(argv[i], "-e")){
            i++;
            endFrame = atoi(argv[i]);
        }
    }
}

int main(int argc, char **argv)
{
    int length;
    int framenum;
    long long i;
    char OutputFrame[20];
    char *ptr = NULL;
    char NumFrame[100];

    parse_argv(argc, argv);

    ptr = malloc(frameWidth);

    fdInputFile = fopen(InputFileName, "rb");
    if(!fdInputFile){
        fprintf(stderr, "Input file open fail\n");
        return -1;
    }

    for(framenum = 0; framenum < startFrame; framenum++){
        for(i = 0; i < frameHeight*3/2; i++){
            fread(ptr, 1, frameWidth, fdInputFile);
        }
    }

    
    for(framenum = startFrame; framenum <= endFrame; framenum++){
        strcpy(OutputFrame, "OutputFrame");
        sprintf(NumFrame, "%d", framenum);
        strcat(OutputFrame, NumFrame);
        strcat(OutputFrame, ".yuv");
        fdOutputFile = fopen(OutputFrame, "wb");
        if(!fdOutputFile){
            fprintf(stderr, "Output file open fail\n");
            return -1;
        }

        for(i = 0; i < frameHeight*3/2; i++){
            length = fread(ptr, 1, frameWidth, fdInputFile);
            if(length < 0){
                printf("fread file fail\n"); 
                return -1;
            }

            fwrite(ptr, 1, 1920, fdOutputFile);
        }
    }
}
