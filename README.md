# IPC Chat and Performance Test Tool 

## Part A: Chat Tool

In this part we involve the implementation of a chat tool that facilitates communication between two side over a network.

#### The usage of the Chat Tool feature is as follows:

1. First of all compile the files by " make stnc ".
2. Open first terminal and run the server side by " ./stnc -s <PORT> ".
3. Open second terminal and run the cliend side by " ./stnc -c <IP> <Port> ".
4. Send messages from the terminal by normal typing.
 
  
## Part B: Performance Test

In addition to the chat functionality, the tool is extended to serve as a network performance test utility. The performance test involves transmitting a chunk of data (100MB in size) using various communication styles and measuring the time it takes. The supported communication styles are as follows:

   1. TCP/UDP over IPv4 or IPv6 (4 variants)
   2. Mapped file and named pipe (2 variants)
   3. Unix Domain Socket (UDS) using stream and datagram modes (2 variants)

#### The usage of the performance test feature is as follows:
  
  1. First of all compile the files by " make stnc ".
  2. Open first terminal and run the server side by " ./stnc -s <PORT> -p -q ".
     The -p flag indicates the performance test, and the -q flag enables the quiet mode where only the testing results are printed.
     This mode is essential for automated testing.
  3. Open second terminal and run the cliend side by " ./stnc -c <IP> <Port> -p <type> <param> ".
  
  ##### (After you run the server you can run the cliend several times to test every communication style).
  
  
  
  Note: Its important to use the flags in the order we showed you here, else the quite mode and the performance mode will probably fail.
