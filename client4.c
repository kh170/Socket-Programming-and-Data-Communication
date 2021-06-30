/*
 * Simple client to work with server.c program.
 * Host name and port used by server are to be
 * passed as arguments.
 *
 * To test: Open a terminal window.
 * At prompt ($ is my prompt symbol) you may
 * type the following as a test:
 *
 * $./client 127.0.0.1 54554
 * Please enter the message: Programming with sockets is fun!  
 * I got your message
 * $ 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h>
#define bool int

void error(const char *msg)
{
    perror(msg);
    exit(0);
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

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    unsigned char buffer[5120];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)     &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
	
	int status;
	
	//client encoder
	
	int clientEncoder = fork();
	if (clientEncoder == 0) 
	{
		printf("In client encoder\n");
		//Application Layer

		char *filename_input = "intext.txt";
			
		char charArray[600];
		char *a;
		
		a = applicationLayerEncoder(filename_input,charArray);
		
		
		//Data Link Layer: Framing
		
		int len = strlen(a)-1;//length of data
		int numberOfFramesE = len/8 +1; //number of frames necessary	
		int frameArrayE[numberOfFramesE*11];
		int *frameE;
		
		
		if (len%8 == 0){
			n = len/8;}
		else {
			n = len/8 + 1;}
		
		frameE = dataLinkLayerEncoder(a, numberOfFramesE,len, frameArrayE);

		//Physical Layer: 
		
		char *filename_output = "BinaryFileClient.binf";
		physicalLayerEncoder(n,frameE, filename_output);



		char sc;
		bzero(buffer,sizeof(buffer));
		n = read(sockfd,buffer,sizeof(buffer));
		if (n<0) error("Error reading from socket");
		printf("Server - %s",buffer);
		scanf("%s",&sc);
		write(sockfd,&sc,sizeof(char));
	}
	else
	{
		printf("In client parent\n");
		wait(NULL);
		int clientDecoder = fork();
		if (clientDecoder == 0)
		{
			printf("In client decoder\n");
				
			//Client Decoder
			//Physical layer
			char *filename1 = "BinaryFileServer.binf";
			int frameArrayD[700];
			int *frameD;
			int frameNumberD;
			frameD = physicalLayerDecoder(filename1,frameArrayD, &frameNumberD);
			
			
			//Data Link Layer
			int dataArray[458];
			int *data;
			int k;
			data = dataLinkLayerDecoder(frameD,dataArray,frameNumberD,&k);
			//Application Layer
			char *filename2 = "resultclient.txt";
			applicationLayerDecoder(filename2, data, k);
		}
		
	}
	

    close(sockfd);
    return 0;
}

