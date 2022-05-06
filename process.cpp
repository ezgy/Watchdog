/**
 * @file process.cpp
 * @author Ezgi Aysel Batı
 * @brief process is created as child of watchdog and sleeps until signal is recieved
 * @date 2021-01-02
 * 
 */
#include <iostream>
#include <string>
#include <csignal>
#include <unistd.h>
#include <fstream>
/**
 * @brief 
 * delta is used for nanosleep same way as in executor
 */
struct timespec delta = {0 /*secs*/, 300000000 /*nanosecs*/};

using namespace std;
/**
 * @brief process number (N in PN) given by caller as argument
 * 
 */
int processno;
/**
 * @brief path of output file for process given by caller as argument
 * 
 */
string output_path;
/**
 * @brief filestream for writing process output
 * 
 */
ofstream file;

/**
 * @brief handles signals by printing signal number
 * 
 * @param signum signal number from table
 */
void signalHandler(int signum){
	if(signum==15){//SIGTERM seperated because it also terminates
    file.open(output_path,fstream::app);
		file<<"P" <<processno <<" received signal 15, terminating gracefully" <<endl;
    file.close();
		exit(signum);
	}else{
    file.open(output_path,fstream::app);
		file<< "P" <<processno <<" received signal " <<signum <<endl ;
    file.close();
	}
	
}

/**
 * @brief 
 * 
 * @param argc argument count
 * @param argv arguments -> 0: program 1: process number 2: output path
 * @return int 0 if self termination, 15 if SIGTERM termination
 */
int main(int argc, char *argv[]){ 
  //process number and output are sent as command line/call arguments
 	processno = stoi(argv[1]);
 	output_path = argv[2];

 	file.open(output_path,fstream::app);
	file<<"P" <<processno <<" is waiting for a signal" <<endl;
  file.close();
  nanosleep(&delta,&delta);

  //connect signals to be handled with the same signalHandler function
	signal(SIGHUP,signalHandler);
	signal(SIGINT,signalHandler);
	signal(SIGILL,signalHandler);
	signal(SIGTRAP,signalHandler);
	signal(SIGFPE,signalHandler);
	signal(SIGSEGV,signalHandler);
	signal(SIGTERM,signalHandler);
	signal(SIGXCPU,signalHandler);


  //process sleeps unless a signal is recieved.
	while(true){
		nanosleep(&delta,&delta);
	}

	return 0;
}