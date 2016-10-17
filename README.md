# MP4 Streamer

[![Crates.io](https://img.shields.io/crates/l/rustc-serialize.svg?maxAge=2592000)]()

## H264 Video Streaming using UNIX Networking 
<p align="justify">
This project MP4-streaming is mainly concerned about brodcasting MP4 videos over the net, mobile phones. In this project I replicate the above process by using a client and a server machines. The client sends it's request for viewing MP4. The server in turn streams the MP4 videos to the client, if the connection is established. The important  thing here is that no MP4s are copied as  whole while sending from server to client. The MP4 videos are stored in MPEG Transport format which is standard for transmission and storage of audio, video and Program and System Information Protocol. Transport stream specifies a container format encapsulating packetized elementary streams, with error correction and stream synchronization features for maintaining transmission integrity when the signal is degraded.
</p>

<p align="justify">
Transport streams differ from the similarly named program streams in several important ways: program streams are designed for reasonably reliable media, such as discs (like DVDs), while transport streams are designed for less reliable transmission, namely terrestrial or satellite broadcast. Further, a transport stream may carry multiple programs.
</p>

<p align="justify">
The Network project  is done basically to work on the UNIX Platform by using the client server technology. The source code is written in C++ Language.
</p>

<p align="justify">
The project will work on the standalone systems, LAN connected systems and the remote machine connected to the internet. However the user will not be able to listen the videos of his/her choice. Only the videos streamed by the streamer will be viewed (like TV programs are streamed).
</p>

## Basic Requirements:
* Your computer is running on Linux with Multimedia Utilities (vlc or ffplay installed).
* Your computer is at least Pentium class, and has 64MB of memory 
* Your computer has a working network configuration 
* You can can type the words 'make' and './configure 

## Execution steps:

>First execute both the server and streamer on the host machine. After this goto any client system (VLC or MX Player etc.) on the network and execute client operation. The following describes steps involved in both host and client part execution.

### HOST PART 
*Server*
* Open a new terminal by pressing `ctrl+alt+f1`, enter the user name and password.
* Now goto the directory in which all the project files are stored by using `cd` command.
* Compile the server file   [mp4server.cpp](https://github.com/DirajHS/MP4_Streamer/blob/master/mp4server.cpp) by giving 
    ```
    $ g++ mp4server.cpp -o se -lpthread 
    ```
	 on the command prompt.
* Execute the compiled file by giving 
    ```
    $ ./se
    ```
    at the command prompt and shift to other terminal.	

*Streamer*
* Once the server is running shift to other terminal by pressing ctrl+alt+f2, goto to the dir in which all the project files are stored by using `cd` command.
* Compile the [mp4streamer.cpp](https://github.com/DirajHS/MP4_Streamer/blob/master/mp4streamer.cpp) file by giving 
    ```
    $ g++ mp4streamer.cpp playlist.cpp -o st
    ```
* Execute the compiled file by  giving 
    ```
        $ ./st <localhost address> 9001 /h pl.txt
    ```
    mp4streamer will start running.

    	
	   
### CLIENT PART
* Open a new terminal and type the IP address of server with port number and mount to play the video. 
* You can use any vidoe player like VLC or MX player.
    ```
    $ vlc http://192.168.0.100:9000/h
    ```

#### SCREENSHOTS
![Terminal output](https://github.com/DirajHS/MP4_Streamer/blob/master/Screenshots/header_details(ignore_red_error).png "Terminal output")

![Playing](https://github.com/DirajHS/MP4_Streamer/blob/master/Screenshots/ffplay_playing.png "Playing")

## License

MIT License

Copyright (c) 2016 DirajHS

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
