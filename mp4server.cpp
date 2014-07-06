//----------------------------------------------------------------------------
// File: mp4server.cpp
// Purpose: Implementation of mp4server
//----------------------------------------------------------------------------

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
#include <ext/hash_map>
namespace std { using namespace __gnu_cxx; }

#define TRUE 1
#define FALSE 0
#define MAXLINE 1024
#define MAXMNTLEN 64
#define MP4CLIENTPORT 9000
#define MP4STREAMERPORT 9001

#define SA struct sockaddr
#include <pthread.h>
#include <list>
using namespace std;


#define LISTENQ 10

struct eqstr
{
	bool operator() (const char* s1, const char* s2) const
	{
		return (strcmp (s1, s2) == 0);
	}
};

// Typedefs for iterators
typedef hash_map<int, const char*>::iterator lisMpIter;
typedef hash_map<int, const char*>::iterator strmMpIter;
typedef hash_map<const char*, hash_map<int, int>*, hash<const char*>, eqstr>::iterator masterIter;
typedef hash_map<int, int>::iterator clientsIter;

//Function prototypes
bool addClient (int, const char*);
void addStreamer (int, const char*);
void delClient (int);
void delStreamer(int);
void printAllTables ();
void printMasterTable ();
void printStreamerTable ();
void printListenerTable ();
int max2 (int s1, int s2);

//Thread function prototypes
void media_streamer (void*);
void accept_connections (void*);
void sigpipeHandler (int a);
//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------
int listen_client;	// listen for mp4 clients on this socket.
int listen_streamer;// listen for mp4 streamers on this socket.

pthread_mutex_t dbmutex = PTHREAD_MUTEX_INITIALIZER;

//hash maps to maintain sources and listerns
hash_map<int, const char*> streamers_2_mountpt;
hash_map<int, const char*> listeners_2_mountpt;
hash_map<const char*, hash_map<int,int>*, hash<const char*>, eqstr> master_table;


//----------------------------------------------------------------------------
// Main:
// 	Init the support tables.
// 	Create streamer side and client side server sockets.
// 	Create threads for accepting connections and streaming.
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
	signal (SIGPIPE, sigpipeHandler);

	// Clear all the hash maps
	streamers_2_mountpt.clear ();
	listeners_2_mountpt.clear ();
	master_table.clear ();
	
	// Create a TCP server socket to listen to streamer
	listen_streamer = socket (AF_INET, SOCK_STREAM, 0);
	if (listen_streamer < 0)
	{
		printf ("ERROR: streamer-socket creation error ...\n");
		exit (1);
	}

	struct sockaddr_in servaddr_streamer;
	bzero (&servaddr_streamer, sizeof (servaddr_streamer));
	servaddr_streamer.sin_family = AF_INET;
	servaddr_streamer.sin_addr.s_addr = htonl (INADDR_ANY);
	servaddr_streamer.sin_port = htons (MP4STREAMERPORT);

	int bind_streamer = bind (listen_streamer, (SA *) &servaddr_streamer, sizeof (servaddr_streamer)); 
	if (bind_streamer < 0 )
	{
		printf ("ERROR: streamer-socket bind error ...\n");
		exit (2);
	}

	// Create a TCP server socket to listen to client
	listen_client = socket (AF_INET, SOCK_STREAM, 0);
	if (listen_client < 0)
	{
		printf ("ERROR: client-socket creation error ...\n");
		exit (1);
	}

	struct sockaddr_in servaddr_client;
	bzero (&servaddr_client, sizeof (servaddr_client));
	servaddr_client.sin_family = AF_INET;
	servaddr_client.sin_addr.s_addr = htonl (INADDR_ANY);
	servaddr_client.sin_port = htons (MP4CLIENTPORT);
	int bind_client = bind (listen_client, (SA *) &servaddr_client, sizeof (servaddr_client)); 
	if (bind_client < 0 )
	{
		printf ("ERROR: client-socket bind error ...\n");
		exit (2);
	}
				
	listen (listen_streamer, LISTENQ); // Make the socket passive
	listen (listen_client, LISTENQ);   // Make the socket passive
	
	pthread_t connAcceptorTid, mediaStreamerTid;
	
	pthread_create (&connAcceptorTid, NULL, (void * (*) (void *))accept_connections, NULL);
	pthread_create (&mediaStreamerTid, NULL, (void * (*) (void *))media_streamer, NULL);

	// Wait for both the threads to finish.
	pthread_join (mediaStreamerTid, NULL);
	pthread_join (connAcceptorTid, NULL);
}

//----------------------------------------------------------------------------
// Function	: accept_connections (void* arg) 
// Description	: Thread
// 	a. Accepts connects from both stremers and clients
// 	b. Updates the support tables
//----------------------------------------------------------------------------

void accept_connections (void* arg) 
{

	cout << "Waiting for connections ..." << endl;
	while(1)
	{	     
		fd_set rset;
		FD_ZERO (&rset);
		FD_SET (listen_streamer, &rset);
		FD_SET (listen_client, &rset);

		struct timeval tv;
		tv.tv_sec=0;	
		tv.tv_usec=0;	

		select (max2 (listen_streamer, listen_client) + 1, &rset, NULL, NULL, &tv);
		if (FD_ISSET (listen_streamer, &rset))
		{
			struct sockaddr_in cliaddr_streamer;
			socklen_t len = sizeof (cliaddr_streamer);
			int conn_streamer = accept (listen_streamer, (SA *)&cliaddr_streamer, &len);
			if (conn_streamer < 0 )
			{
				cout << strerror(errno) << endl;
				cout << "ERROR: Error accepting connection from streamer" << endl;
				//do cleanup.
			}
			else
			{
				// Currently, the mount point is the only thing the
				// streamer will send. Hence what we receive is the
				// mount point.
				char mountpt_from_streamer [MAXMNTLEN]; 
				int recvlen = recv (conn_streamer,mountpt_from_streamer,MAXMNTLEN, 0);
				if (recvlen < 0)
				{
					cout << strerror(errno) << endl;
					exit (1);
				} 
				else
				{
					// add a '\0' to the end. 
					mountpt_from_streamer[recvlen]='\0';

					cout << "Read " << recvlen << " bytes from socket" << endl;
					printf ("%s: Mount point received is :%s\n",__PRETTY_FUNCTION__,mountpt_from_streamer);

					int pml = pthread_mutex_lock (&dbmutex);
					addStreamer (conn_streamer,mountpt_from_streamer);
					pml = pthread_mutex_unlock (&dbmutex);
				}
			}
		} else if (FD_ISSET (listen_client, &rset))
		{
			char client_header[MAXLINE];
			char mountpt[MAXMNTLEN];
			int i = 0;
			struct sockaddr_in cliaddr_client;	
			socklen_t len = sizeof (struct sockaddr_in);
			int conn_client = accept (listen_client, (SA *)&cliaddr_client ,&len);
			if(conn_client < 0 )
			{
				printf("ERROR: Error accepting connection from client\n");
			} else
			{
				int bytes_read = recv (conn_client ,
						client_header, MAXLINE, 0);
				if (bytes_read < 0)
				{
					printf ("Mount point not received from client ...\n");
					exit (1); 
				} else
				{
					//printf ("%s: Mount point received is :", __PRETTY_FUNCTION__, mountpt_from_streamer);
				}
				printf ("Client header : \n%s", client_header);
				if (strncmp (client_header ,"GET" , 4))
				{
					char *ptr1 = strchr (client_header ,'/');
					printf ("ptr1 = %c\n",*ptr1);
					int i = 0;
					while ((*ptr1 != ' ')) 
					{
						mountpt[i++] = *ptr1++;
					}
					mountpt[i] = '\0';  
					printf ("mountPoint = (%s)\n", mountpt);
				}
				else
				{
					printf ("Header not recived properly \n");
				}

				printf ("Client num = %d\n", conn_client);
				if (conn_client == -1)
				{
					printf ("ERROR: Error accepting client connction \n");
					//exit (3);
				}	
				else
				{
					char videoBuffer[MAXLINE];

					printf ("Accepted connection from client ...\n");

					int pml = pthread_mutex_lock (&dbmutex);
					bool addClientSuccess = addClient (conn_client, mountpt);
					pml = pthread_mutex_unlock (&dbmutex);

					if (addClientSuccess)
					{
						strcpy (videoBuffer, "HTTP/1.0 200 OK\r\n");
						strcat (videoBuffer, "Content-Type: video/mp4\r\n");
					}
					else
					{
						strcat (videoBuffer, "HTTP/1.0 404 Not Found\r\n");
					}
					strcat (videoBuffer, "\r\n");

					// Send the HTTP Acknowledgement
					if (send (conn_client, videoBuffer, strlen (videoBuffer), 0) < 0)
					{
						perror ("send of HTTP Ack failure");
					}
				}
			} // else of (if (conn_client < 0 ))
		} // 

		FD_SET(listen_streamer, &rset);
		FD_SET(listen_client, &rset);
	} // while (1)
}

//----------------------------------------------------------------------------
// Function	: media_streamer 
// Description	: Thread
// 	a. Receives the streamed media from the streamers
// 	b. Streams the media to the registered clients
//----------------------------------------------------------------------------

void media_streamer (void* arg)
{
	char buff[MAXLINE];
	int oldmax=0;

	while (1)
	{
		int x = pthread_mutex_lock (&dbmutex);

		fd_set rset;
		FD_ZERO (&rset);			

		int maxsockfd = 0;
		for (strmMpIter i = streamers_2_mountpt.begin (); i != streamers_2_mountpt.end(); ++i)
		{
			// Add sockfd to the set of sockfds to be listened to.
			FD_SET (i->first, &rset);
			// Find the maxsockfd also.
			if (i->first > maxsockfd)
				maxsockfd = i->first;
		}
		if (maxsockfd > oldmax)
		{
			cout << "New streamer added with sockfd " << maxsockfd << endl;
			oldmax = maxsockfd;
		}

		struct timeval tv;
		tv.tv_sec = 0;	
		tv.tv_usec = 0;	
		select (maxsockfd+1, &rset, NULL, NULL, &tv);

		list<int> removeStreamerList;
		removeStreamerList.clear ();

		for (strmMpIter i = streamers_2_mountpt.begin(); i != streamers_2_mountpt.end(); ++i)
		{
			if (FD_ISSET (i->first, &rset))
			{
				list<int> removeListenerList;
				removeListenerList.clear ();
				int recvLen = recv (i->first, buff, MAXLINE, 0);

				if (recvLen <= 0)
				{
					printf ("Read error from streamer...\n");
					removeStreamerList.push_back (i->first);
				} else
				{
					// First get the "channel" onto which the socket 
					// is feeding. (basically mountpoint).
					const char* mp = i->second;

					// Now get the table of listeners for that mountPoint.
					hash_map<int, int>* phmii = master_table[ mp ];

					// For all the listeners, feed the song. If an error
					// is found for a listener, add that socket 
					// to removeList so that we can remove later.
					for (clientsIter j = phmii->begin (); j != phmii->end (); ++j)
					{
						int sendRetVal = send (j->first, buff, recvLen, 0);
						printf("%d bytes of data streamed from the server\n", sendRetVal);
						if (sendRetVal < 0)
						{
							removeListenerList.push_back (j->first);
						}
					}

					// remove listeners who have had problems earlier.
					for (list<int>::iterator k = removeListenerList.begin ();k != removeListenerList.end (); ++k)
					{
							delClient (*k);
					}
				} // else (if recvLen < 0)
			} // if (FD_ISSET (i->first, &rset)) 
		}// end of for loop to locate the streamer accepted

		// Remove the streamers who were misbehaving.
		for (list<int>::iterator rsli = removeStreamerList.begin ();rsli != removeStreamerList.end (); ++rsli)
		{
			cout << "Deleting streamer " << *rsli << endl;
			delStreamer (*rsli);
		}
		// We are done with this list. Clear all.
		removeStreamerList.clear();
		// Also if the streamers list is empty, clear rset also.
		if (streamers_2_mountpt.empty())
		{
			FD_ZERO(&rset);
		}
		int y = pthread_mutex_unlock (&dbmutex);
	}//end of while loop
}
				
//----------------------------------------------------------------------------
// Function	: addClient
// Description	: adds clients sockfd & mountpoint provided in hash maps -
// 		a) listeners_2_mountpt
// 		b) master_table 
// Inputs	:connected socket descriptor & requested MountPoint
// Return Value	:a flag indicating whether hash maps are upgraded or not
// Notes	:
//----------------------------------------------------------------------------

bool addClient (int sockfd, const char* mp)
{
	printf ("%s: sockfd = (%d), mp = (%s)\n", __PRETTY_FUNCTION__, sockfd,mp);

	// int x = pthread_mutex_lock (&dbmutex);
	masterIter iter = master_table.find (mp);
	if (iter != master_table.end ())
	{
		printf ("%s: Added a client to the list.", __PRETTY_FUNCTION__);
		lisMpIter i = listeners_2_mountpt.find (sockfd); 
		if(i == listeners_2_mountpt.end())
		{
			char *s = (char*) malloc (strlen (mp) + 1);
			strcpy (s, mp);
			listeners_2_mountpt[sockfd] = s;
			hash_map<int, int>* plhm = iter->second;
			(*(plhm))[sockfd] = sockfd;
		} else
		{
			printf ("Client :(%d) exists\n",sockfd);
			return false; // Though this case is extreemely (100%) unlikely !
		}
	}else
	{
		printf("Client cannot be added as source streaming %s songs is not streaming\n",mp);		   
		return false;
	}
	// int y = pthread_mutex_unlock (&dbmutex);
	printAllTables ();
	return true;
}


//----------------------------------------------------------------------------
// Function	: addStreamer
// Description	: adds clients sockfd & mountpoint provided in hash maps -
// 		a) streamers_2_mountpt
// 		b) master_table 
// Inputs	:connected socket descriptor & requested MountPoint
// Return Value	:a flag indicating whether hash maps are upgraded or not
// Notes	:
//----------------------------------------------------------------------------
void addStreamer (int sockfd, const char* mp)
{
	printf ("%s: sockfd = (%d), mountPoint = (%s)\n", __PRETTY_FUNCTION__,
			sockfd, mp);
	// int x = pthread_mutex_lock (&dbmutex);
	printAllTables();

	// Check if mount point present in Master Table
	masterIter iter = master_table.find (mp);
	if (iter == master_table.end ())
	{
		// This mountpoint not in Master Table. Go ahead and add it.
		cout << "No entry in Master Table for " << mp << endl;
		cout << "Adding " << mp  << " to Master table and streamers_2_mountpt table" << endl;

		strmMpIter i = streamers_2_mountpt.find (sockfd);	
		if(i == streamers_2_mountpt.end())
		{
			// streamers_2_mountpt does not contain sockfd
			char* s = (char*) malloc (strlen (mp) + 1);
			strcpy (s, mp);
			streamers_2_mountpt[sockfd] = s;
			master_table[s] = new hash_map<int, int>;
		} 
		else
		{
			// streamers_2_mountpt already contains sockfd
			printf ("%s: Streamer with sockfd(%d) is already in use\n", __PRETTY_FUNCTION__, sockfd);
		}
	} else
	{
		// This mountpoint already present in Master Table. Ignore this
		// streamer. Also close the socket connection to the streamer.
	    cout << "Mount point " << mp << " already in Master Table. Ignoring YOU." << endl;
	    close (sockfd);
	}

	printAllTables ();
	// int y = pthread_mutex_unlock (&dbmutex);
}

//----------------------------------------------------------------------
// Function delClient
// 		
//----------------------------------------------------------------------
void delClient(int sockfd)
{
	printf ("%s: sockfd = (%d)\n", __PRETTY_FUNCTION__, sockfd);
	// int x = pthread_mutex_lock (&dbmutex);

	// Remove clients' entry in the dynamic client table.
	clientsIter ci = (*(master_table[listeners_2_mountpt[sockfd]])).find(sockfd);
	(*(master_table[listeners_2_mountpt[sockfd]])).erase(ci);

	lisMpIter lmi = listeners_2_mountpt.find(sockfd);
	// Free the memory allocated to store the mount point
	free((char*)lmi->second);
	// Remove clients' entry from listeners_2_mountpt table	
	listeners_2_mountpt.erase(lmi);
	// Also close the clients' conn socket
	close (sockfd);

	printAllTables();

	// int y = pthread_mutex_unlock (&dbmutex);
}

//----------------------------------------------------------------------

void delStreamer(int sockfd)
{
	printf ("%s: sockfd = (%d)\n", __PRETTY_FUNCTION__, sockfd);

	// int x = pthread_mutex_lock (&dbmutex);
	hash_map<int, int>* plistbl = master_table[streamers_2_mountpt[sockfd]];
	
	// Close all the client conn sockets listening to this streamer
	// Also erase off the client's entry in the listeners table.
	for (clientsIter si = plistbl->begin(); si != plistbl->end(); ++si)
	{
		cout << "Closing socket" << si->first << endl;
		close(si->first);

		// erase client's entry in the listeners table.
		listeners_2_mountpt.erase(si->first);
	}

	// Delete the listeners table
	delete plistbl;

	// Erase the streamers entry in the master table.
	masterIter mi = master_table.find(streamers_2_mountpt[sockfd]);
	master_table.erase(mi);

	strmMpIter si = streamers_2_mountpt.find(sockfd);
	// Free the memory allocated to store the mount point
	free((char*)si->second);
	// Erase the streamer from the streamers list.
	streamers_2_mountpt.erase(si);

	printAllTables();

	// int y = pthread_mutex_unlock (&dbmutex);
}
//----------------------------------------------------------------------

void sigpipeHandler (int a)
{
	 printf ("%s called.", __PRETTY_FUNCTION__);
}

void printAllTables ()
{
	printf ("\n-----------------------------------------------------------\n");	
	printMasterTable ();
	printStreamerTable ();
	printListenerTable ();
	printf ("\n-----------------------------------------------------------\n");	
}

void printMasterTable ()
{
	printf ("----------------- Master Table ----------------\n");
	for (masterIter i = master_table.begin (); i != master_table.end ();++i)
	{
		printf ("mp = (%s)\n", i->first);
		hash_map<int, int>* plt = i->second;
		for (clientsIter j = plt->begin (); j != plt->end (); ++j)
		{
			printf ("\tsockfd = (%d)\n", j->first);
		}
	}
	printf ("\n");
}

void printStreamerTable ()
{
	printf ("----------------- Streamer Table ----------------\n");
	for (strmMpIter i = streamers_2_mountpt.begin (); i != streamers_2_mountpt.end (); ++i)
	{
		printf ("sockfd = (%d), mp = (%s)\n", i->first, i->second);
	}
	printf ("\n");
}

void printListenerTable ()
{
	printf ("----------------- Listeners Table ----------------\n");
	for (lisMpIter i = listeners_2_mountpt.begin (); i != listeners_2_mountpt.end (); ++i)
	{
		printf ("sockfd = (%d), mp = (%s)\n", i->first, i->second);
	}
	printf ("\n");
}

int max2 (int s1, int s2)
{
	return (s1 > s2 ? s1 : s2);
}
//----------------------------------------------------------------------
// End of <mp4server.cpp>
//----------------------------------------------------------------------
