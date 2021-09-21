/* This is the server code */

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <pthread.h>

#define SERVER_PORT 8080 /* arbitrary, but client & server must agree*/
#define BUF_SIZE 4096    /* block transfer size */
#define QUEUE_SIZE 10

using namespace std;

vector<string> requisicao(string entrada)
{
   int inicio = 0;
   int ultima = entrada.find(' ', inicio);
   string tipo_requisicao = entrada.substr(inicio, ultima - inicio);

   inicio = ultima + 1;
   ultima = entrada.find(' ', inicio);
   string nome_arquivo = entrada.substr(inicio + 1, ultima - inicio - 1);

   inicio = ultima + 1;
   ultima = entrada.find('\r', inicio);
   string header = entrada.substr(inicio, ultima - inicio);

   string hostname = entrada.substr(ultima + 1, entrada.size() - ultima);

   vector<string> separacao_requisicao(3);
   separacao_requisicao[0] = tipo_requisicao;
   separacao_requisicao[1] = nome_arquivo;
   separacao_requisicao[2] = header;

   return separacao_requisicao;
}

// Função para converter nome do host do servidor em endereço IP
string host2IP(string host)
{
   struct addrinfo hints;
   struct addrinfo *res;
   char ip_string[INET_ADDRSTRLEN] = {'\0'};
   int status = 0;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   if ((status = getaddrinfo(host.c_str(), "80", &hints, &res)) != 0)
   {
      cerr << "getaddrinfo: " << gai_strerror(status) << endl;
      return "";
   }

   struct addrinfo *p = res;
   struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;

   // Converter IP em string
   inet_ntop(p->ai_family, &(ipv4->sin_addr), ip_string, sizeof(ip_string));

   freeaddrinfo(res);
   return ip_string;
}

int main(int argc, char *argv[])
{
   string host;
   string port;
   string dir;

   // $ web-server [host] [port] [dir]
   // $ ./web-server localhost 3000 /tmp
   //host = argv[1];
   //port = argv[2];
   //dir = argv[3];

   if (argc == 4)
   {
      host = argv[1];
      port = argv[2];
      dir = argv[3];
   }
   else if (argc == 3)
   {
      host = argv[1];
      port = argv[2];
   }
   else if (argc == 2)
   {
      host = argv[1];
   }

   // Converter a porta PORT em número inteiro PORTA
   int porta = stoi(port);

   // Converter nome do host do servidor em endereço IP
   string ip = host2IP(host);

   int s, b, l, fd, sa, bytes, on = 1;
   char buf[BUF_SIZE];         /* buffer for outgoing file */
   struct sockaddr_in channel; /* holds IP address */

   /* Build address structure to bind to socket. */
   memset(&channel, 0, sizeof(channel));

   /* zero channel */
   struct hostent *h;
   h = gethostbyname(&host[0]);
   channel.sin_family = AF_INET;
   memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
   //channel.sin_addr.s_addr = htonl(INADDR_ANY);
   channel.sin_port = htons(porta); //SERVER_PORT -> porta

   /* Passive open. Wait for connection. */
   s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* create socket */
   if (s < 0)
   {
      printf("socket call failed");
      exit(-1);
   }

   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

   b = bind(s, (struct sockaddr *)&channel, sizeof(channel));
   if (b < 0)
   {
      printf("bind failed");
      exit(-1);
   }

   l = listen(s, QUEUE_SIZE); /* specify queue size */
   if (l < 0)
   {
      printf("listen failed");
      exit(-1);
   }

   /* Socket is now set up and bound. Wait for connection and process it. */
   while (1)
   {
      sa = accept(s, 0, 0); /* block for connection request */
      string resposta;

      if (sa < 0)
      {
         printf("accept failed");
         exit(-1);
      }

      //write: escrever no socket sa o nome do arquivo (dir?)
      //write(sa, &dir[0], dir.size() + 1);

      read(sa, buf, BUF_SIZE); /* read file name from socket */
      //cout << buf << endl;

      vector<string> partes_requisicao = requisicao(buf);
      //cout << partes_requisicao[0] << endl;
      //cout << endl;
      //cout << partes_requisicao[1] << endl;
      //cout << endl;
      //cout << partes_requisicao[2] << endl;

      string nome_arquivo;
      if (argc == 4)
      {
         nome_arquivo = "." + dir + "/" + partes_requisicao[1];
      }
      else
      {
         nome_arquivo = partes_requisicao[1];
      }

      /* Get and return the file. */
      //cout << nome_arquivo << endl;
      fd = open(nome_arquivo.c_str(), O_RDONLY); /* open the file to be sent back */

      if (fd < 0)
      {
         //printf("open failed");
         resposta = partes_requisicao[2] + " 404 Not Found" + "\r\n";
         write(1, resposta.c_str(), strlen(resposta.c_str()));
         write(sa, resposta.c_str(), strlen(resposta.c_str())); /* write bytes to socket */
      }

      //printf("teste teste teste\n");

      while (1)
      {

         bytes = read(fd, buf, BUF_SIZE); /* read from file */
         //write(1, buf, bytes);

         if (bytes <= 0)
            break; /* check for end of file */

         string conteudo_arq;
         for (int i = 0; i < bytes; i++)
         {
            conteudo_arq += buf[i];
         }

         string bytes_str = to_string(bytes);

         resposta = resposta + partes_requisicao[2] + " 200 OK" + "\r\n" + "Content-Length: " + bytes_str + "\r\n" + conteudo_arq + "\r\n";

         write(1, resposta.c_str(), strlen(resposta.c_str()));
         write(sa, resposta.c_str(), strlen(resposta.c_str())); /* write bytes to socket */

         //cout << sa << endl;
      }

      close(fd); /* close file */
      close(sa); /* close connection */
   }
}
