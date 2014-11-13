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
  out.sin_port = htons(port);
  out.sin_addr.s_addr = inet_addr(server);

  dump_packet(request, size);

  if (sendto(sock, request, size, 0, &out, sizeof(out)) < 0) {
    // an error occurred
    return -1;
  }

  // wait for the DNS reply (timeout: 5 seconds)
  struct sockaddr_in in;
  /*in.sin_family = AF_INET;
  in.sin_port = htons(port);
  in.sin_addr.s_addr = inet_addr(server);*/
  socklen_t in_len;
  in_len = sizeof(in);

  // construct the socket set
  fd_set socks;
  FD_ZERO(&socks);
  FD_SET(sock, &socks);

  // construct the timeout
  struct timeval t;
  t.tv_sec = 5;
  t.tv_usec = 0;

  // wait to receive, or for a timeout
  char *input = (char *) malloc(65536);
  if (select(sock + 1, &socks, NULL, NULL, &t)) {
    if (recvfrom(sock, input, 65536, 0, &in, &in_len) < 0) {
      // an error occured
      perror("Recvfrom");
      return -1;
    }
  } else {
    // a timeout occurred
    fprintf(stdout, "NORESPONSE");
    return -1;
  }
  memcpy(&id, input, 2);
  if (ntohs(id) != 1337) {
    fprintf(stdout, "ERROR\tNot our packet");
    return -1; // not ours!
  }
  memcpy(&line_2, input+2, 2);
  line_2 = ntohs(line_2);
  //unsigned short qr = line_2_first & 0x80;
  if (!(line_2 & 0x8000)) {
    fprintf(stdout, "Error\tNot a response");
    return -1; // not a response
  }
  unsigned short opcode = line_2 & 0x7800;
  unsigned short aa;
  if (line_2 & 0x400) {
    aa = 1;
  } else {
    aa = 0;
  }
  unsigned short tc = line_2 & 0x200;
  if (line_2 & 0x200) {
    fprintf(stdout, "ERROR\tTRUNCATED");
    return -1; // its trunctated
  }
  unsigned short ra = line_2 & 0x80;
  if (!(line_2 & 0x80)) {
    fprintf(stdout, "ERROR\tNO RECUSION");
    return -1; // no recursion
  }
  unsigned short rcode = 0;
  rcode &= line_2 & 0xf;
  if (rcode == 1) {
    fprintf(stdout, "ERROR\tFormat error");
  } else if (rcode == 2) {
    fprintf(stdout, "ERROR\tServer failure");
  } else if (rcode == 3 && aa == 1) {
    fprintf(stdout, "NOTFOUND");
  } else if (rcode == 3 && aa == 0) {
    fprintf(stdout, "Error\tNot authoritative, name not found");
  } else if (rcode == 4) {
    fprintf(stdout, "ERROR\tNot implemented");
  } else if (rcode == 5) {
    fprintf(stdout, "ERROR\tRefused");
  }
  if (rcode != 0) {
    return -1;
  }
  memcpy(&qdcount, input + 4, 2);
  memcpy(&ancount, input + 6, 2);
  //printf("ancount: %hu\n", ntohs(ancount));
  /*if (ntohs(qdcount) != 0) {
    return -1; // not an answer
  }*/
  pos = 12; // skip the header
  // if there's a question, loop over it
  /*for (int k = 0; k < ntohs(qdcount); k++) {
    printf("qdcount: %hu\n", ntohs(qdcount));
    unsigned int len = 0;
    memcpy(input + pos, &len, 1);
    len = ntohl(len);
    printf("len: %hu", len);
    pos++;
    while (len != 0) {
      pos += len;
      memcpy(input + pos, &len, 1);
      len = ntohl(len);
      pos++;
    }
    pos += 4;
    printf("pos %d\n", pos); 
  }*/

  pos += size - 12;

  // loop over each answer
  char name_s[256];
  
  for (unsigned short i = 0; i < ntohs(ancount); i++) {
    // get the name
    unsigned short len;
    memcpy(&len, input + pos, 1);
    len = ntohs(len);
    pos++;
    int name_pos = 0;
    while (len != 0) {
      if (len & 0xC000) {
        // pointer
        unsigned short pointer = 0;
        memcpy(&pointer, input + pos - 1, 2);
        pos++;
        pointer = htons(pointer) & 0x3fff;
        len = 0;
        memcpy(&len, input + pointer, 1);
        memcpy(&name_s + name_pos, input + pointer + 1, len);
        name_pos += len;
        memcpy(&len, input + pos, 1);
        len = ntohs(len);
      } else {
        memcpy(&name_s + name_pos, input + pos, len);
        name_pos += len;
        pos += len;
        memcpy(&len, input + pos, 1);
        pos++;
        len = ntohs(len);
      }
      name_s[name_pos] = '.';
      name_pos++;
    }

    // get the type
    unsigned short type;
    memcpy(&type, input + pos, 2);
    type = ntohs(type);
    pos += 2;

    // class
    unsigned short class;
    memcpy(&class, input + pos, 2);
    class = ntohs(class);
    pos += 2;
    if (class != 0x0001) {
      fprintf(stdout, "ERROR\tnot an internet address");
      return -1; // not an internet address
    }

    // TTL
    unsigned int ttl;
    memcpy(&ttl, input + pos, 4);
    ttl = ntohl(ttl);
    pos += 4;

    // rdlength
    unsigned short rdlength;
    memcpy(&rdlength, input + pos, 2);
    rdlength = ntohs(rdlength);
    pos += 2;

    dump_packet(input, 400);

    if (type == 0x0001) {
      // ip address
      unsigned int ip_full;
      memcpy(&ip_full, input + pos, 4);
      pos += 4;
      unsigned char ip[4];
      ip[0] = ip_full & 0xFF;
      ip[1] = (ip_full >> 8) & 0xFF;
      ip[2] = (ip_full >> 16) & 0xFF;
      ip[3] = (ip_full >> 24) & 0xFF;
      printf("IP\t%d.%d.%d.%d\t", ip[0], ip[1], ip[2], ip[3]);
      if (aa == 0) {
        printf("nonauth\n");
      } else {
        printf("auth\n");
      }
    } else if (type == 0x0005) {
      // printing a name
      unsigned short read;
      char name_c[256] = "";
      name_pos = 0;

      while (read < rdlength) {
        memcpy(&len, input+pos, 1); // read the 1st byte into len
        printf("len: %hu\n", len);
        if (len & 0xc0) { // pointer (len starts with bits 11)
          unsigned short pointer; 
          memcpy(&pointer, input + pos, 2); // get the byte put in len and the next as a pointer
          pointer = ntohs(pointer); // order the pointer correctly
          pointer = pointer & 0x3fff; // remove the first two 1's from the pointer
          printf("pointer: %hu\n", pointer);
          memcpy(&len, input + pointer, 1); // get the length of the string at the pointer
          printf("len: %hu\n", len); 
          memcpy(name_c + name_pos, input + pointer + 1, len); // read len bytes into the name string
          read++; // read 1 pointer
          name_pos += len; // move len bytes forward in name_c
          pos += 2;
        } else { // not a pointer
          pos++;
          memcpy(name_c + name_pos, input + pos, len);
          name_pos += len;
          read += len;
          pos += len;
        }
        name_c[name_pos] = '.';
        name_pos++;
      }

      printf("CNAME %s\t", name_c);
      if (aa == 0) {
        printf("nonauth\n");
      } else {
        printf("auth\n");
      }
    }
  }
  if (ntohs(ancount) == 0) {
    fprintf(stdout, "NOTFOUND");
    return -1;
  }
  return 0;
  
}
/*
 * read_name starts at the given offset and reads until it either
 * passes off the job to another iteration of itself after finding
 * a pointer or reaches a 0 octet
 */
char *read_name(char *input, unsigned short offset) {
  char val[256];
  int val_pos = 0;
  unsigned short len;
  memcpy(&len, input + offset, 1);
  len = ntohs(len);
  while (len != 0) {
    if (len & 0xc0) { // pointer
      unsigned short pointer;
      memcpy(&pointer, input+offset, 2);
      pointer = ntohs(pointer);
      pointer &= 0x3fff;
      char *rest = read_name(input, pointer);
      strcpy(val+val_pos, rest, 256-val_pos);
      return val;
    } else { // not a pointer
      offset++;
      memcpy(val+val_pos, input+offset, len);
      offset += len;
      val_pos += len;
      val[val_pos] = '.';
      val_pos++;
    }
  }
  val[val_pos-1] = '\0';
  return val;
}
