/* Program: server.c
 * A simple TCP server using sockets.
 * Server is executed before Client.
 * Port number is to be passed as an argument. 
 * 
 * To test: Open a terminal window.
 * At the prompt ($ is my prompt symbol) you may
 * type the following as a test:
 *
 * $ ./server 54554
 * Run client by providing host and port
 *
 * 
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<ctype.h>
#include<pthread.h>
#include <semaphore.h>
#define bool int

void error(const char *msg)
{
    perror(msg);
    exit(1);
}



// Binary to ASCII conversion
int binToDec(int *n){
	int dec_value = 0;
	int num;
	int base = 1;
	for (int i = 7; i>0; i--)
	{
		num = n[i] * base;
		dec_value = dec_value + num;
		base = base * 2;
	}
	return dec_value;
}

//Physical Layer_decoder
int *physicalLayerDecoder(char *filename, int *frame, int *nValue){
	//reading binary file
	FILE *fPointer;
	fPointer = fopen(filename,"rb");
	int bitArray[5200];
	fread(bitArray, sizeof(bitArray),1,fPointer);
	fclose(fPointer);
	
	int n = 0;
	int synArr[8] = {0,0,0,1,0,1,1,0};
	for (int a = 0; a<5104; a= a+88)
	{	
		if ((bitArray[a] == synArr[0]) & (bitArray[a+1] == synArr[1]) & (bitArray[a+2] == synArr[2]) & 	(bitArray[a+3] == synArr[3]) & (bitArray[a+4] == synArr[4]) & (bitArray[a+5] == synArr[5]) & (bitArray[a+6] == synArr[6]) & (bitArray[a+7] == synArr[7]))
				{n++;}
		
	}	
	
	int len = n*88;
	*nValue = n;
	
	//converting bits to character
 	int binNum[8];
	int k =0;
	for (int e = 0; e<len; e = e+8){
		for (int b = 0; b<8; b++){
			binNum[b] = bitArray[e+b];
		}					
		frame[k] = binToDec(binNum);   
		k++;
	}
	return frame;

}

//Data Link Layer Decoder
int *dataLinkLayerDecoder(int *frame, int *dataArray,int frameNo, int *kValue){
	//deframing
	int n,j,k=0;
	int max = frameNo*11;
	for (int i = 0; i<max; i++){
		if ((frame[i] == 22) & (frame[i+1] == 22)){
			n = frame[i+2];
			for (j = i+3; j<i+3+n; j++){
				dataArray[k] = frame[j];
				k++;
			}
		}
	}
	*kValue = k;
	return dataArray;
}

//Application Layer Decoder
void applicationLayerDecoder(char *filename, int *data, int k){
	//writing the data in output file
	
	FILE *fPointer;
	fPointer = fopen(filename,"w");
	int i = 0;
	while (i<k){	
		fprintf(fPointer, "%c", data[i]);
		i++;
	}
	fclose(fPointer);
}


int *buffer[10];
int count = 0;
int *data;
int final[659];
int finalCount = 0;

void *queue(void *x)
{
	for(int i = 0; i<10; i++)
	{	
		buffer[i] = &data[count];
		//printf("%c",*buffer[i]);
 		count++;
	}
	//printf("\n");
	
}

void *replace(void *x)
{
	char ch = *(char *)x;
	for (int i = 0;i<10;i++)
	{		
		if (isspace(*buffer[i]))
		{	
			*buffer[i] = ch;
		}
		//printf("%c",*buffer[i]);
	}	
	//printf("\n");
}

void *upper(void *x)
{
	for (int i = 0;i<10;i++)
	{		
		if(islower(*buffer[i]))
		{
			*buffer[i] = *buffer[i]-32;
		}
		//printf("%c",*buffer[i]);
	
	}
	
	//printf("\n");
}

void *store(void *x)
{
	for (int i = 0;i<10;i++)
	{		
		final[finalCount] = *buffer[i];
		//printf("%c",final[finalCount]);		
		finalCount++;
	}
	//printf("\n");
	
}

//ASCII to Binary convertion
int *bin(int x, int *n)
{
        int c,k;
        for (c = 7; c>=0; c--)
        {
                k = x>>c;
                if (k&1)
                        n[7-c] = 1;
                else
                        n[7-c] = 0;
        }
        return n;
}

//Odd bit parity 
bool oddParity (int n)
{
        bool parity = 1;
        while (n)
        {
                parity = !parity;
                n = n & (n-1);
        }
        return parity;
}

//Application Layer Encoder
char *applicationLayerEncoder(char *filename, char *ch)
{
	//reading data from input files
	FILE *fPointer;
	fPointer = fopen(filename,"r");
	int i = 0;
	char singleChar;
	while ((singleChar = fgetc(fPointer)) != EOF)
	{
		ch[i] = singleChar;
		//printf("%c",ch[i]);
		i = i+1;
		
	}
	ch[i] = 0;
	fclose(fPointer);
	return ch;
}

//Data Link Layer Encoder
int *dataLinkLayerEncoder(char *a, int n, int len, int *frame){
	int syn = 22; //syn character
	int index = 0;
	//framing
	for (int i = 0; i<(n*11); i=i+11){
		frame[i] = syn; //first syn character insertion in the frame
		frame[i+1] = syn; //second syn character insertion in the frame
		//control character insertion in the frame
		if ((len-(((i+11)/11)*8))>=0)
			frame[i+2] = 8;
		else if ((len-(((i+11)/11)*8))<0)
			frame[i+2] = len%8;
		//data insertion in the frame
		for (int j = i+3; j<i+11; j++){
			if (index < len){
				frame[j] = a[index];
				index++;
			}
		}	
	}
	return frame;
}

//Physical Layer Encoder
void physicalLayerEncoder(int n, int *frame, char*filename){

	//converting frames to binary and adding odd bit parity
	int binData[n*88];
	int l = 0;	
	for (int i=0; i<(n*11); i=i+11)
	{	for (int j=i; j<i+11; j++){
			int binArray[8];
			int *binNum;
			int x;
			x = frame[j];
			binNum = bin (x,binArray); //converting frame to binary
			binNum[0] = oddParity(x); //adding oddParity to the frame
			
			for(int a = 0; a<8; a++)
			{
				binData[l] = binNum[a];
				l++;	
			}
			
		}
	}
	//writing the binary frames in the output file
	//writing the packed binary frames in the output file
	FILE *fPointer;
		
	fPointer = fopen(filename,"wb");
	fwrite(binData,sizeof(binData),1,fPointer);
	fclose(fPointer);
}


int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     fprintf(stdout, "Run client by providing host and port\n");
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
        error("ERROR on accept");

	
	//creating two child processes
     	
	int serverDecoder = fork();
	if (serverDecoder == 0) 
	{
		printf("In server decoder\n");
		//reading the character to be replaced with
			char sc;
				
			n = write(newsockfd, "Enter character: \n", strlen("Enter character: \n"));
			if (n<0) error("Error in writing to socket");
			read(newsockfd, &sc, sizeof(char));
			printf("Client - Character is %c\n",sc);

			
			//Physical layer
			char *filename1 = "BinaryFileClient.binf";
			int frameArrayD[700];
			int *frameD;
			int frameNumberD;
			frameD = physicalLayerDecoder(filename1,frameArrayD, &frameNumberD);
			
			
			
			//Data Link Layer
			int dataArray[659];
			
			int k;
			data = dataLinkLayerDecoder(frameD,dataArray,frameNumberD,&k);
			
			
			pthread_t th[4];
			while (count<659)
			{
				for (int i = 0; i<4; i++)
				{
					if (i==0)
					{
						if (pthread_create(th+i,NULL,&queue,NULL)!=0)
						{	perror("Failed to create thread");
						}
						if (pthread_join(th[i],NULL)!=0)
						{	perror("Failed to join thread");
						}
					}
					else if (i==1)
					{
						if (pthread_create(th+i,NULL,&replace,&sc)!=0)
						{	perror("Failed to create thread");
						}
						if (pthread_join(th[i],NULL)!=0)
						{	perror("Failed to join thread");
						}
					}
					else if (i==2)
					{
						if (pthread_create(th+i,NULL,&upper,NULL)!=0)
						{	perror("Failed to create thread");
						}
						if (pthread_join(th[i],NULL)!=0)
						{	perror("Failed to join thread");
						}
					}
					else if (i==3)
					{
						if (pthread_create(th+i,NULL,&store,NULL)!=0)
						{	perror("Failed to create thread");
						}
						if (pthread_join(th[i],NULL)!=0)
						{	perror("Failed to join thread");
						}
					}
				}
			}
			for (int i = 0; i<458;i++)
			{
				printf("%c",final[i]);
			}
			//Application Layer
			char *filename2 = "resultserver.txt";
			applicationLayerDecoder(filename2, final, 458);
			
	}
	else
	{
		printf("In server parent\n");
		int serverEncoder = fork();
		if (serverEncoder == 0)
		{
			printf("In server encoder\n");
			//Application Layer

			char *filename_input = "resultserver.txt";
				
			char charArray[458];
			char *a;
			
			a = applicationLayerEncoder(filename_input,charArray);
			
			
			//Data Link Layer: Framing
			
			int len = strlen(a)-1;//length of data
			int numberOfFrames = len/8 +1; //number of frames necessary	
			int frameArray[numberOfFrames*11];
			int *frame;
			
			
			if (len%8 == 0){
				n = len/8;}
			else {
				n = len/8 + 1;}
			
			frame = dataLinkLayerEncoder(a, numberOfFrames,len, frameArray);

			//Physical Layer: 
			
			char *filename_output = "BinaryFileServer.binf";
			physicalLayerEncoder(n,frame, filename_output);

					
		}
		
	}	



     close(newsockfd);
     close(sockfd);
     return 0; 
}

