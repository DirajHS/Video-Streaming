//----------------------------------------------------------------------
// File: playlistMgr.cpp
// Purpose: Prototypes of MP3 streamer.

//----------------------------------------------------------------------
#ifndef _MP4STREAMER_H_
#define _MP4STREAMER_H_
//----------------------------------------------------------------------
// File: mp3streamer.h
// Purpose: Prototypes of MP3 streamer.
//----------------------------------------------------------------------
#include <list>
#include<iostream>
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

list<const char*>* PlayList::playlist = new list<const char*>;
list<const char*>::iterator PlayList::prevvideo = PlayList::playlist->begin ();

PlayList::PlayList()
{
	playlist->clear();
}

void PlayList::addVideoToPlayList(const char* video)
{
	playlist->push_back(video);
}

int PlayList::gotoTopPlayList(void)
{
	if (playlist->empty())
	{
		return -1;
	}
	else
	{
		prevvideo = playlist->begin();
		return 0;
	}
}

const char* PlayList::getNextVideo(void)
{
	list<const char*>::iterator nextvideo = prevvideo++;
	if (prevvideo == playlist->end())
	{
		prevvideo = playlist->begin();
	}
	return (*(nextvideo));
}

void PlayList::printPlayList(void)
{
	int j=0;
	for (list<const char*>::iterator i=playlist->begin(); i!=playlist->end();		++i)
	{
		std::cout<< ++j << ". " << *i <<std::endl;
	}
}

/*
int main (void)
{
	PlayList pl;

	if (pl.gotoTopPlayList() < 0)
	{
			cout << "ERROR: PlayList empty." << endl;
	}
	
	for (int i=0; i<9; i++)
	{
		cout <<  i+1 << "." << pl.getNextVideo() << endl;
	}
}
*/

//----------------------------------------------------------------------
// End of <playlistMgr.cpp>
//----------------------------------------------------------------------

