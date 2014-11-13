/*
 * CS3600, Spring 2014
 * Project 3 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600dns.h"

/**
 * This function will print a hex dump of the provided packet to the screen
 * to help facilitate debugging.  In your milestone and final submission, you 
 * MUST call dump_packet() with your packet right before calling sendto().  
 * You're welcome to use it at other times to help debug, but please comment those
 * out in your submissions.
 *
 * DO NOT MODIFY THIS FUNCTION
 *
 * data - The pointer to your packet buffer
 * size - The length of your packet
 */
static void dump_packet(unsigned char *data, int size) {
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isprint(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

int main(int argc, char *argv[]) {
  /**
   * I've included some basic code for opening a socket in C, sending
   * a UDP packet, and then receiving a response (or timeout).  You'll 
   * need to fill in many of the details, but this should be enough to
   * get you started.
   */

  // process the arguments
  if (argc != 3){
    return -1;
  }
  char *server_port_pair = argv[1]; // @00.00.00.00:0000
  char *name = argv[2]; // name to query for
  
  if (strncmp(server_port_pair, "@", 1) != 0){
    // error in server port pair
    return -1;
  }

  server_port_pair++;
  char *server = strtok(server_port_pair, ":");
  char *port_c = strtok(NULL, ":");
  short port;
  if (port_c == NULL){
    port = 53;
  } else {
    port = atoi(port_c);
  }
  //printf("Server: %s, Port: %hu", server, port);

  // construct the DNS request
  int size = 18 + strlen(name);
  char *request = (char *) malloc(size);
  unsigned short id = htons(0x0539);
  unsigned short line_2 = htons(0x0100);
  unsigned short qdcount = htons(0x0001);
  unsigned short ancount = htons(0x0000);
  unsigned short nscount = htons(0x0000);
  unsigned short arcount = htons(0x0000);
  unsigned short qtype = htons(0x0001);
  unsigned short qclass = htons(0x0001);
  memcpy(request, &id, 2);
  memcpy(request+2, &line_2, 2);
  memcpy(request+4, &qdcount, 2);
  memcpy(request+6, &ancount, 2);
  memcpy(request+8, &nscount, 2);
  memcpy(request+10, &arcount, 2);

  char *qname_part = strtok(name, ".");
  int len;
  int pos = 12;
  while (qname_part != NULL) {
    len = strlen(qname_part);
    memcpy(request+pos, &len, 1);
    pos++;
    memcpy(request+pos, qname_part, len);
    pos += len;
    qname_part = strtok(NULL, ".");
  }
  unsigned short end = htons(0x0000);
  memcpy(request+pos, &end, 1);
  memcpy(request+pos+1, &qtype, 2);
  memcpy(request+pos+3, &qclass, 2);


  
  // send the DNS request (and call dump_packet with your request)
  
  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // next, construct the destination address
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(53);
  out.sin_addr.s_addr = inet_addr("129.10.112.152");

  if (sendto(sock, request, size, 0, &out, sizeof(out)) < 0) {
    // an error occurred
  }
  dump_packet(request, size);
  return 0;
  /*

  // wait for the DNS reply (timeout: 5 seconds)
  struct sockaddr_in in;
  socklen_t in_len;

  // construct the socket set
  fd_set socks;
  FD_ZERO(&socks);
  FD_SET(sock, &socks);

  // construct the timeout
  struct timeval t;
  t.tv_sec = ;
  t.tv_usec = 0;

  // wait to receive, or for a timeout
  if (select(sock + 1, &socks, NULL, NULL, &t)) {
    if (recvfrom(sock, <<your input buffer>>, <<input len>>, 0, &in, &in_len) < 0) {
      // an error occured
    }
  } else {
    // a timeout occurred
  }

  // print out the result
  
  return 0;
  */
  
}
