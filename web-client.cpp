/* This page contains a client program that can request a file from the server programon the next page. 
   The server responds by sending the whole file.
*/

#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>

#define SERVER_PORT 8080 /* arbitrary, but client & server must agree */
#define BUFSIZE 4096	 /* block transfer size */

using namespace std;

// Função para segmentar a URL nas partes convevientes
vector<string> segmentar_URL(string url)
{	
	int inicio = 0;
	if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p')
		inicio = 7;
	
	//int inicio = 7; // Começar busca após "http://"
	int ultima = url.find(':', inicio);
	string hostname = url.substr(inicio, ultima - inicio);

	inicio = ultima + 1;
	ultima = url.find('/', inicio);
	string porta = url.substr(inicio, ultima - inicio);

	string objeto_solicitado = url.substr(ultima + 1, url.size() - ultima);

	vector<string> separacao(4);
	separacao[0] = hostname;
	separacao[1] = porta;
	separacao[2] = objeto_solicitado;

	inicio = objeto_solicitado.find('/', 0);
	string nome_interpretado = objeto_solicitado.substr(inicio + 1, objeto_solicitado.size() - inicio);
	separacao[3] = nome_interpretado;

	/*cout << hostname << endl;
	cout << porta << endl;
	cout << objeto_solicitado << endl;
	cout << nome_interpretado << endl;*/

	return separacao;
}

int main(int argc, char **argv)
{

	int c, s, bytes;
	char buf[BUFSIZE];			/* buffer for incoming file */
	struct hostent *h;			/* info about server */
	struct sockaddr_in channel; /* holds IP address */

	vector<string> separacao = segmentar_URL(argv[1]);
	string hostname = separacao[0];
	string porta = separacao[1];
	string objeto_solicitado = separacao[2];
	string nome_interpretado = separacao[3];

	/*if (argc != 3)
	{
		printf("Usage: client server-name file-name");
		exit(-1);
	}*/

	h = gethostbyname(&hostname[0]); /* look up host’s IP address */
	if (!h)
	{
		printf("gethostbyname failed to locate %s", &hostname[0]);
		exit(-1);
	}

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
	{
		printf("socket call failed");
		exit(-1);
	}

	memset(&channel, 0, sizeof(channel));
	channel.sin_family = AF_INET;
	memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);

	channel.sin_port = htons(stoi(porta));
	c = connect(s, (struct sockaddr *)&channel, sizeof(channel));

	if (c < 0)
	{
		printf("connect failed");
		exit(-1);
	}

	/* Connection is now established. Send file name including 0 byte at end. */
	string mensagem_HTTP = "GET /" + objeto_solicitado + " HTTP/1.1\r\nHost:" + hostname + "\r\n\r\n";
	write(s, &mensagem_HTTP[0], mensagem_HTTP.size() + 1);

	/* Go get the file and write it to standard output.*/
	while (1)
	{
		bytes = read(s, buf, BUFSIZE); /* read from socket */

		if (bytes <= 0)
			exit(0); /* check for end of file */

		if (buf[9] == '2')
		{
			int i;
			string buffer = "";
			int fim = 0;
			int primeira;
			for (i = 0; i < sizeof(buf) && fim == 0; i++)
			{
				if (buf[i] == '<')
				{
					primeira = i;
					fim = 1;
				}
			}
			fim = 0;

			for (i = primeira; i < sizeof(buf) && fim < 2; i++)
			{
				buffer = buffer + buf[i];
				if (buf[i] == '>')
					fim = fim + 1;
			}

			string nova = nome_interpretado;
			ofstream file(nova);
			file << buffer;

			//cout << buffer << endl;
		}

		/*if (buf[9] == '4' and buf[11] == '0')
		{
			cout << '400 BAD REQUEST' << endl;
		}

		if (buf[9] == '4' and buf[11] == '4')
		{
			cout << '404 NOT FOUND' << endl;
		}*/

		write(1, buf, bytes); /* write to standard output */
	}
}
