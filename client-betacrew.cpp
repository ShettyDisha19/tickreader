#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#  pragma pack(1)

struct Packet {
  char  symbol[4];
    char  buySellIndicator;
    int32_t quantity;
    int32_t price;
    int32_t packetSequence;
};
#  pragma pack()

int sel_make_sock_connect( int port, char *ip_address) {
  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == -1) {
    std::cerr << "Socket creation error." << std::endl;
    return -1;
  }
  
  
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);  
  serverAddress.sin_addr.s_addr = inet_addr(ip_address); 

  // Connect to server
  if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
    std::cerr << "Connection to server failed." << std::endl;
    return -1;
  }else {
    return clientSocket ; 
  }
}

int main() {


  std::string	 symbol;
  char		 buySell ;
  int32_t	 quantity, price, sequence;
  int		 port		 = 3000 ;
  char		*ip_address	 = (char *)"127.0.0.1";
  int		 expected_seq_no = 1;
  
  int socket =  sel_make_sock_connect(port , ip_address);

    typedef struct request_s{
      uint8_t calltype;
      uint8_t resend;
    }request_t;
    request_t my_request_payload ;
    my_request_payload.calltype =1;
    my_request_payload.resend = 2;

    int missing_array[100];
    int missing_sequence_count = 0;

    
    Packet packet;
    std::cout << " size of packet is " << sizeof(packet) << std::endl;
    send(socket, &my_request_payload, sizeof(my_request_payload), 0);

    std::ofstream logFile("stream_all_packets_log.txt");
    
    logFile << "BEFORE RECEIVE ClientSOcket: "<<socket<<std::endl;

    // Inside the loop where you receive response packets
    while (recv(socket, &packet, 17, 0) > 0) {
      //      logFile << "HERE RECEIVED A PACKET OF SIZE : "<<sizeof(packet)<<std::endl;
      logFile<<""<<std::endl;
      symbol   = packet.symbol;
      buySell  = packet.buySellIndicator;
      quantity = ntohl(packet.quantity);
      price    = ntohl(packet.price); 
      sequence = ntohl(packet.packetSequence); 
      

      logFile << "== Symbol: " << symbol << ", Buy/Sell: " << buySell
	      << ", Quantity: " << quantity << ", Price: " << price
	      << ", Packet Sequence received: " << sequence << " expectedseqno "<<expected_seq_no<<std::endl;


      if (expected_seq_no < sequence )  {
	int miss_count = sequence - expected_seq_no;
	for (int i = 0; i < miss_count ; i++){
	  missing_array[missing_sequence_count] = expected_seq_no + i ;
	  missing_sequence_count ++ ;
	}
	

      }
      
      expected_seq_no = sequence + 1 ;
    }

    //    sleep(10);
    socket =  sel_make_sock_connect(port , ip_address);

    logFile<<""<<std::endl;
    logFile<<"Connected to the server again at socket "<<socket<<std::endl;
    logFile<<""<<std::endl;
    logFile<<"As We have missing packets , missing count is "<<missing_sequence_count<<std::endl;
    int i;
    std::ofstream logFileResend("resend_packet_log.txt");
    for (i = 0 ; i < missing_sequence_count; i ++ ) {
      logFile << "Requesting a resend for packet with sequence number "<<missing_array[i]<<std::endl;
	my_request_payload.calltype = 2;
	my_request_payload.resend   = missing_array[i];



	send(socket, &my_request_payload, sizeof(my_request_payload), 0);
	
	int retvalrecv = 	recv(socket, &packet, 17, 0);
	  if (retvalrecv != 0){
	    symbol = packet.symbol;
	    buySell = packet.buySellIndicator;
	    quantity = ntohl(packet.quantity);
	    price = ntohl(packet.price); 
	    sequence = ntohl(packet.packetSequence); 
      
	    logFileResend << "Resent Symbol: " << symbol << ", Buy/Sell: " << buySell
	    << ", Quantity: " << quantity << ", Price: " << price
	    << ", Packet Sequence: " << sequence <<std::endl;	    
	  }
    }
    logFile.close();

    // Construct request payload for "Resend Packet"
    // uint8_t resendCallType = 2;
    // uint8_t resendSeq = 123;  // Replace with the actual packet sequence to be resent
    // send(socket, &resendCallType, sizeof(resendCallType), 0);
    // send(socket, &resendSeq, sizeof(resendSeq), 0);

    // Receive and log the resent packet for "Resend Packet"

    //    recv(socket, &packet, sizeof(packet), 0);
    // Parse and log the resent packet fields here

    logFileResend.close();

    // Close connection
    close(socket);

    return 0;
}
