//----------------------------------------------------------------------
// File: mp4streamer.cpp
// Purpose: Implementation of MP4 streamer.
//----------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <hash_map>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define MAXLINE 1024
#define MAXMNTLEN 64
#define MP4CLIENTPORT 9000
#define MP4STREAMERPORT 9001

#define SA struct sockaddr
#ifndef _MP4STREAMER_H_
#define _MP4STREAMER_H_
//----------------------------------------------------------------------
// File: mp4streamer.h
// Purpose: Prototypes of MP4 streamer.
//----------------------------------------------------------------------
#include <list>
#define MAXPATHLEN 256
using namespace std;
void timeoutHandler (int);

class PlayList
{
public:
	PlayList();
	const char* getNextVideo();
	void addVideoToPlayList(const char* video);
	int gotoTopPlayList(void);
	void printPlayList(void);
private:
	static std::list<const char*>* playlist;
	static std::list<const char*>::iterator prevvideo;
};

#endif _MP4STREAMER_H_
#include <fstream>

void timeoutHandler (int);

FILE* fd;
int sockfd;
bool over = false;

int main(int argc, char **argv)
{
	int  n ,file_size = 0;
	char* channelname;
	struct sockaddr_in servaddr;

	if (argc != 5)
	{
		printf("usage: %s <IPaddress> <portno> <channel> <playlistfile>\n", argv[0]);
		exit(1);
	}

	char* playlistfilename = (char*) malloc (strlen(argv[4]));
	strcpy(playlistfilename, argv[4]);
	//printf("%s\n", argv[4]);
	// Check if we can open the playlist file.
	FILE* plf = fopen(playlistfilename, "r");
	if (plf == NULL)
	{
		cout << "ERROR: Error opening " << playlistfilename << endl;
		cout << strerror(errno) << endl;
		exit(1);
	}

	// Create the internal playlist
	PlayList pl;
	while (!feof(plf))
	{
		char* videoname = (char*) malloc (1024);
		char *pch = NULL;
		fgets (videoname, 1024, plf);
		pch = strrchr(videoname, '\n');
		if(pch != NULL) *pch = '\0';
		pl.addVideoToPlayList(videoname);
	}

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("err_sys : socket error");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(MP4STREAMERPORT);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == 0)
	{
		printf("err_quit : inet_pton  error for %s", argv[1]);
		exit(3);
	}

	if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
	{
		printf("err_sys : connect error\n");
		printf("%s\n", strerror(errno));
		exit(1);
	}

	channelname = (char*) malloc (strlen(argv[3])+1);
	strcpy (channelname, argv[3]);
	send (sockfd, channelname, strlen (channelname), 0);

	printf("Mount point sent is :::::::::::: %s\n", channelname);
	free (channelname);

	cout << "Waiting a wee bit for the mount point to be properly recognized ... " << endl;
	cout << "sleep(1)..." << endl;
	sleep(1);

	if (pl.gotoTopPlayList() < 0)
	{
			cout << "ERROR: PlayList empty." << endl;
			close(sockfd);
			exit(1);
	}

	while(1)
	{
	    	const char *nextVideo = NULL;
		signal (SIGALRM, timeoutHandler);
		cout << "Next Song in Playlist: " << pl.getNextVideo()<< endl;

        	nextVideo = pl.getNextVideo();
		fd = fopen(pl.getNextVideo(), "r");
		if (fd == NULL)
		{
			printf ("Error opening file [%s]\n", nextVideo);
			cout << strerror(errno) << endl;
			exit(2);
		}

		itimerval ival;
		ival.it_value.tv_sec = 0;
		ival.it_value.tv_usec = 2000;//32768; //62500;
		ival.it_interval.tv_sec = 0;
		ival.it_interval.tv_usec = 2000;//32768; //62500;
		setitimer (ITIMER_REAL, &ival, NULL);

		while(!over);
			over=false;
			fclose(fd);
	}
	close(sockfd);
	exit(0);
}

void timeoutHandler (int a)
{
	char buf[4096];
	int n = fread (buf, 1, 1024, fd);
	static int sc=0;

	cout << "#";
	fflush(stdout);

	if (n > 0)
	{
		if (send (sockfd, buf, n, 0) < 0)
			printf ("error writing\n");
	}
	else
		over = true;
}
//----------------------------------------------------------------------
//	End of <mp4streamer.cpp>
//----------------------------------------------------------------------
