/**
 * @file watchdog.cpp
 * @author Ezgi Aysel Batı
 * @brief creates and watches processes, restarts terminated ones. 
 * @date 2021-01-02
 * 
 *    The main idea of this project is to concurrently run and handle multiple children of the same process. 
 * It also requires keeping track of children properly since head process differs from others. Sleep methods
 * inbetween were necessary to handle different outputs due to running order, which greately reduces
 * speed for large number of processes. 
 *     Overall this project was interesting since it both makes use of multiple concepts learned in the course
 * (pipes, forking, process ids...) and also models a solution to a real life problem. If I had the chance to
 * improve, I would find a different solution to sleep inbetween and also clear up the main method to consist
 * of calls to multiple functions so that it would be easier to read and understand.   
 */
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <csignal>
#include <sstream>
#include <fstream>
#include <string.h>

/**
 * @brief 
 * delta is used for nanosleep same way as in executor
 */
struct timespec delta = {0 /*secs*/, 300000000 /*nanosecs*/}; 

using namespace  std;

/**
 * @brief filestream for writing watchdog output
 * 
 */
ofstream file;

/**
 * @brief only handles SIGTERM for termination at the end.
 * 
 * @param signum signal number from table
 */
void signalHandler(int signum){
  if(signum == 15){
    file<<"Watchdog is terminating gracefully" <<endl;
    file.close();
  }
  exit(signum);
}
/**
 * @brief 
 * 
 * @param argc argument count
 * @param argv arguments -> 0:program 1:number of processes 2: process output path 3:watchdog output path
 * @return int 0 for end termination, 15 for SIGTERM termination
 */
int main(int argc, char *argv[]) {

	//command line arguments
  /**
   * @brief int $num_of_process$ number of children processes to be created
   */
	int num_of_process = stoi(argv[1]) ;
  /**
   * @brief string $watchdog_output_path$ path for watchdog output file
   * 
   */
	string watchdog_output_path = argv[3];

  /**
   * @brief pid_t [] $process$ -> process[process_no]=process_id, watchdog is process0
   * 
   */
  pid_t process[num_of_process+1];

  signal(SIGTERM, signalHandler);

  file.open(watchdog_output_path);,
  /**
   * @brief ofstream $processfile$ process output file only opened for initialization - clear previous contents
   * 
   */
  ofstream processfile;

  //clears process output file before writing anything
  //this is done here because process.cpp  must only append write to the file.
  processfile.open(argv[2]);
  processfile.close();

	//initiate write-end of pipe parallel to read end in executor
  int namedPipe;
  char * myfifo = (char*) "/tmp/myfifo";
  mkfifo(myfifo, 0644); //permissions are taken to be same as in executor
  namedPipe = open(myfifo,O_WRONLY);

  //write pid of watchdog to pipe
  /**
   * @brief pid_t $wpid$ process id of watchdog
   * 
   */
  pid_t wpid = getpid();
  string spid = "P0 " + to_string(wpid);
  process[0] = wpid;
  /**
   * @brief char[] $writer$ char array that keeps message to be written to pipe
   * 
   */
  char writer[30];
  strcpy(writer, spid.c_str());
  nanosleep(&delta,&delta);
  write(namedPipe, writer, 30);

  //create and write pid of children
  for(int i=1; i<=num_of_process; i++){
    /**
     * @brief int $forkID$ process ID of child for parent, 0 for child
     * 
     */
    int forkID = fork();
    /**
     * @brief pid_t $pid$ process id of running process
     * 
     */
		pid_t pid = getpid();

    if(forkID == 0){ //child process
    /**
     * @brief char[] $progno$ process number
     * 
     */
      char progno[5];
      string inp = to_string(i);
      strcpy(progno,inp.c_str());

      /**
       * @brief char[] $progname$ program name = "./process"
       * 
       */
      char progname[10];
      strcpy(progname,"./process");

      /**
       * @brief char*[] $args$ arguments for exec()
       * 
       */
    	char* args[] = {progname,progno,argv[2], NULL};
    	execv(args[0],args);
    }
     process[i] = forkID;
     string str = "P" + to_string(i) +" " + to_string(forkID);
    	file <<"P" <<i <<" is started and it has a pid of " <<forkID <<endl;
      strcpy(writer, str.c_str());
      nanosleep(&delta,&delta);
    	write(namedPipe, writer, 30);
  }

  //sleep-wait for a child to terminate.
  while(true){
    /**
     * @brief pid_t $child$ process id of terminated child
     * 
     */
    pid_t child = wait(NULL); //waiting until a process is terminated

    /**
     * @brief int $index$ process number of terminated child
     * 
     */
    int index = 0; //finding process no from process id
    for(int i =1; i<=num_of_process; i++){
      if(process[i]==child){
        index =i;
        break;
      }
    }
    
    if(index==1){ //if head process terminates, all processes get restarted
      file<<"P1 is killed, all processes must be killed" <<endl;
      file<<"Restarting all processes" <<endl;
      for(int i=2; i<=num_of_process; i++){//killing all other processes
        kill(process[i],SIGTERM);
        wait(NULL);//skip signals from killed processes for manual restart
      }

    for(int i=1; i<=num_of_process; i++){ //restart killed processes
      int forkID = fork();
		  pid_t pid = getpid();

      if(forkID == 0){ //child process
    	  char progno[5];
        string inp = to_string(i);
        strcpy(progno,inp.c_str());

        char progname[10];
        strcpy(progname,"./process");

    	  char* args[] = {progname,progno,argv[2], NULL};
    	  execv(args[0],args);
      }
      process[i] = forkID;
      string str = "P" + to_string(i) +" " + to_string(forkID);
    	file <<"P" <<i <<" is started and it has a pid of " <<forkID <<endl;

      strcpy(writer, str.c_str());
      nanosleep(&delta,&delta);
    	write(namedPipe, writer, 30);
    }
  }else{//single process terminated, only that is restarted
      file<<"P" <<index <<" is killed" <<endl;
      file <<"Restarting P" <<index <<endl;

      int forkID = fork();
		  pid_t pid = getpid();
    
    	if(forkID == 0){ //child process
    		char progno[5];
        string inp = to_string(index);
        strcpy(progno,inp.c_str());

        char progname[10];
        strcpy(progname,"./process");

    	  char* args[] = {progname,progno,argv[2], NULL};
    		execv(args[0],args);
      }
      process[index]= forkID;
      string str = "P" + to_string(index) +" " + to_string(forkID);
    	file <<"P" <<index <<" is started and it has a pid of " <<forkID <<endl;
      strcpy(writer, str.c_str());
      nanosleep(&delta,&delta);
    	write(namedPipe, writer, 30);
     }
  }
    close(namedPipe);

    return 0;
}