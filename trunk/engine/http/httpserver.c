#include "quakedef.h"

#ifdef WEBSERVER

#include "iweb.h"

#include "netinc.h"

//FIXME: Before any admins use this for any serious usage, make the server send bits of file slowly.

static qboolean httpserverinitied = false;
qboolean httpserverfailed = false;
static int	httpserversocket;
static int	httpserverport;

#if 0//def WEBSVONLY
static int natpmpsocket = INVALID_SOCKET;
static int natpmptime;
#ifdef _WIN32
#include <mmsystem.h>
#endif
static void sendnatpmp(int port)
{
	struct sockaddr_in router;
	struct
	{
		qbyte ver;
		qbyte op;
		short reserved1;
		short privport; short pubport;
		int mapping_expectancy;
	} pmpreqmsg;

	int curtime = timeGetTime();
	if (natpmpsocket == INVALID_SOCKET)
	{
		unsigned long _true = true;
		natpmpsocket = socket(AF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0);
		if (natpmpsocket == INVALID_SOCKET)
			return;
		ioctlsocket (natpmpsocket, FIONBIO, &_true);
	}
	else if (curtime - natpmptime < 0)
		return;
	natpmptime = curtime+60*1000;

	memset(&router, 0, sizeof(router));
	router.sin_family = AF_INET;
	router.sin_port = htons(5351);
	router.sin_addr.S_un.S_un_b.s_b1 = 192;
	router.sin_addr.S_un.S_un_b.s_b2 = 168;
	router.sin_addr.S_un.S_un_b.s_b3 = 0;
	router.sin_addr.S_un.S_un_b.s_b4 = 1;

	pmpreqmsg.ver = 0;
	pmpreqmsg.op = 0;
	pmpreqmsg.reserved1 = htons(0);
	pmpreqmsg.privport = htons(port);
	pmpreqmsg.pubport = htons(port);
	pmpreqmsg.mapping_expectancy = htons(60*5);

	sendto(natpmpsocket, (void*)&pmpreqmsg, 2, 0, (struct sockaddr*)&router, sizeof(router));

	pmpreqmsg.op = 2;
	sendto(natpmpsocket, (void*)&pmpreqmsg, sizeof(pmpreqmsg), 0, (struct sockaddr*)&router, sizeof(router));
}
void checknatpmp(int port)
{
	struct
	{
		qbyte ver; qbyte op; short resultcode;
		int age;
		union
		{
			struct
			{
				short privport; short pubport;
				int mapping_expectancy;
			};
			qbyte ipv4[4];
		};
	} pmpreqrep;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	int len;
	static int oldip=-1;
	static short oldport;
	memset(&pmpreqrep, 0, sizeof(pmpreqrep));
	sendnatpmp(port);
	if (natpmpsocket == INVALID_SOCKET)
		len = -1;
	else
		len = recvfrom(natpmpsocket, (void*)&pmpreqrep, sizeof(pmpreqrep), 0, (struct sockaddr*)&from, &fromlen);
	if (len == 12 && pmpreqrep.op == 128)
	{
		if (oldip != *(int*)pmpreqrep.ipv4)
		{
			oldip = *(int*)pmpreqrep.ipv4;
			oldport = 0;
			IWebPrintf("Public ip is %i.%i.%i.%i\n", pmpreqrep.ipv4[0], pmpreqrep.ipv4[1], pmpreqrep.ipv4[2], pmpreqrep.ipv4[3]);
		}
	}
	else if (len == 16 && pmpreqrep.op == 129)
	{
		if (oldport != pmpreqrep.pubport)
		{
			oldport = pmpreqrep.pubport;
			IWebPrintf("Public udp port %i (local %i)\n", ntohs(pmpreqrep.pubport), ntohs(pmpreqrep.privport));
		}
	}
	else if (len == 16 && pmpreqrep.op == 130)
	{
		if (oldport != pmpreqrep.pubport)
		{
			oldport = pmpreqrep.pubport;
			IWebPrintf("Public tcp port %i (local %i)\n", ntohs(pmpreqrep.pubport), ntohs(pmpreqrep.privport));
		}
	}
}
#else
void checknatpmp(int port)
{
}
#endif

typedef enum {HTTP_WAITINGFORREQUEST,HTTP_SENDING} http_mode_t;



qboolean HTTP_ServerInit(int port)
{
	struct sockaddr_in address;
	unsigned long _true = true;
	int i;

	if ((httpserversocket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		IWebPrintf ("HTTP_ServerInit: socket: %s\n", strerror(neterrno()));
		httpserverfailed = true;
		return false;
	}

	if (ioctlsocket (httpserversocket, FIONBIO, &_true) == -1)
	{
		IWebPrintf ("HTTP_ServerInit: ioctl FIONBIO: %s\n", strerror(neterrno()));
		httpserverfailed = true;
		return false;
	}

	address.sin_family = AF_INET;
//check for interface binding option
	if ((i = COM_CheckParm("-ip")) != 0 && i < com_argc)
	{
		address.sin_addr.s_addr = inet_addr(com_argv[i+1]);
		Con_TPrintf("Binding to IP Interface Address of %s\n",
				inet_ntoa(address.sin_addr));
	}
	else
		address.sin_addr.s_addr = INADDR_ANY;

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	if( bind (httpserversocket, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(httpserversocket);
		IWebPrintf("HTTP_ServerInit: failed to bind to socket\n");
		httpserverfailed = true;
		return false;
	}

	listen(httpserversocket, 3);

	httpserverinitied = true;
	httpserverfailed = false;
	httpserverport = port;

	IWebPrintf("HTTP server is running\n");
	return true;
}

void HTTP_ServerShutdown(void)
{
	closesocket(httpserversocket);
	IWebPrintf("HTTP server closed\n");

	httpserverinitied = false;
}

typedef struct HTTP_active_connections_s {
	SOCKET datasock;
	char peername[256];
	vfsfile_t *file;
	struct HTTP_active_connections_s *next;

	http_mode_t mode;
	qboolean modeswitched;
	qboolean closeaftertransaction;
	char *closereason;
	qboolean acceptgzip;

	char *inbuffer;
	unsigned int inbuffersize;
	unsigned int inbufferused;

	char *outbuffer;
	unsigned int outbuffersize;
	unsigned int outbufferused;
} HTTP_active_connections_t;
static HTTP_active_connections_t *HTTP_ServerConnections;
static int httpconnectioncount;

static void ExpandInBuffer(HTTP_active_connections_t *cl, unsigned int quant, qboolean fixedsize)
{
	unsigned int newsize;
	if (fixedsize)
		newsize = quant;
	else
		newsize = cl->inbuffersize+quant;
	if (newsize <= cl->inbuffersize)
		return;

	cl->inbuffer = IWebRealloc(cl->inbuffer, newsize);
	cl->inbuffersize = newsize;
}
static void ExpandOutBuffer(HTTP_active_connections_t *cl, unsigned int quant, qboolean fixedsize)
{
	unsigned int newsize;
	if (fixedsize)
		newsize = quant;
	else
		newsize = cl->outbuffersize+quant;
	if (newsize <= cl->outbuffersize)
		return;

	cl->outbuffer = IWebRealloc(cl->outbuffer, newsize);
	cl->outbuffersize = newsize;
}

void HTTP_RunExisting (void)
{
	char *content;
	char *msg, *nl;
	char buf2[2560];	//short lived temp buffer.
	char resource[2560];
	char host[256];
	char mode[80];
	qboolean hostspecified;
	unsigned int contentlen;

	int HTTPmarkup;	//version
	int localerrno;

	HTTP_active_connections_t **link, *cl;

	link = &HTTP_ServerConnections;
	for (link = &HTTP_ServerConnections; *link;)
	{
		int ammount, wanted;

		cl = *link;

		if (cl->closereason)
		{
			IWebPrintf("%s: Closing connection: %s\n", cl->peername, cl->closereason);

			*link = cl->next;
			closesocket(cl->datasock);
			cl->datasock = INVALID_SOCKET;
			if (cl->inbuffer)
				IWebFree(cl->inbuffer);
			if (cl->outbuffer)
				IWebFree(cl->outbuffer);
			if (cl->file)
				VFS_CLOSE(cl->file);
			IWebFree(cl);
			httpconnectioncount--;
			continue;
		}

		link = &(*link)->next;

		switch(cl->mode)
		{
		case HTTP_WAITINGFORREQUEST:
			if (cl->outbufferused)
				Sys_Error("Persistant connection was waiting for input with unsent output");
			ammount = cl->inbuffersize-1 - cl->inbufferused;
			if (ammount < 128)
			{
				if (cl->inbuffersize>128*1024)
				{
					cl->closereason = "headers larger than 128kb";	//that's just taking the piss.
					continue;
				}

				ExpandInBuffer(cl, 1500, false);
				ammount = cl->inbuffersize-1 - cl->inbufferused;
			}
			if (cl->modeswitched)
			{
				ammount = 0;
			}
			else
			{
				//we can't try and recv 0 bytes as we use an expanding buffer
				ammount = recv(cl->datasock, cl->inbuffer+cl->inbufferused, ammount, 0);
				if (ammount < 0)
				{
					int e = neterrno();
					if (e != NET_EWOULDBLOCK)	//they closed on us. Assume end.
					{
						cl->closereason = "recv error";
					}
					continue;
				}
				if (ammount == 0)
				{
					cl->closereason = "peer closed connection";
					continue;
				}
			}
			cl->modeswitched = false;

			cl->inbufferused += ammount;
			cl->inbuffer[cl->inbufferused] = '\0';

			content = NULL;
			msg = cl->inbuffer;
			nl = strchr(msg, '\n');
			if (!nl)
			{
cont:
				continue;	//we need more... MORE!!! MORE I TELL YOU!!!!
			}

			//FIXME: COM_ParseOut recognises // comments, which is not correct.
			msg = COM_ParseOut(msg, mode, sizeof(mode));

			msg = COM_ParseOut(msg, resource, sizeof(resource));

			if (!*resource)
			{
				cl->closereason = "not http";	//even if they forgot to specify a resource, we didn't find an HTTP so we have no option but to close.
				continue;
			}

			host[0] = '?';
			host[1] = 0;

			hostspecified = false;
			if (!strnicmp(resource, "http://", 7))
			{	//groan... 1.1 compliance requires parsing this correctly, without the client ever specifiying it.
				char *slash;	//we don't do multiple hosts.
				hostspecified=true;
				slash = strchr(resource+7, '/');
				if (!slash)
					strcpy(resource, "/");
				else
				{
					int hlen = slash-(resource+7);
					if (hlen > sizeof(host)-1)
						hlen = sizeof(host)-1;
					memcpy(host, resource+7, hlen);
					host[hlen] = 0;
					memmove(resource, slash, strlen(slash+1));	//just get rid of the http:// stuff.
				}
			}

			if (!strcmp(resource, "/"))
				strcpy(resource, "/index.html");

			cl->acceptgzip = false;

			msg = COM_ParseOut(msg, buf2, sizeof(buf2));
			contentlen = 0;
			if (!strnicmp(buf2, "HTTP/", 5))
			{
				if (!strncmp(buf2, "HTTP/1.1", 8))
				{
					HTTPmarkup = 3;
					cl->closeaftertransaction = false;
				}
				else if (!strncmp(buf2, "HTTP/1", 6))
				{
					HTTPmarkup = 2;
					cl->closeaftertransaction = true;
				}
				else
				{
					HTTPmarkup = 1;	//0.9... lamer.
					cl->closeaftertransaction = true;
				}

				//expect X lines containing options.
				//then a blank line. Don't continue till we have that.

				msg = nl+1;
				while (1)
				{
					if (*msg == '\r')
						msg++;
					if (*msg == '\n')
					{
						msg++;
						break;	//that was our blank line.
					}

					while(*msg == ' ')
						msg++;

					if (!strnicmp(msg, "Host: ", 6))	//parse needed header fields
					{
						int l = 0;
						msg += 6;
						while (*msg == ' ' || *msg == '\t')
							msg++;
						while (*msg != '\r' && *msg != '\n' && l < sizeof(host)-1)
							host[l++] = *msg++;
						host[l] = 0;
						hostspecified = true;
					}
					else if (!strnicmp(msg, "Content-Length: ", 16))	//parse needed header fields
						contentlen = strtoul(msg+16, NULL, 0);
					else if (!strnicmp(msg, "Accept-Encoding:", 16))	//parse needed header fields
					{
						char *e;
						msg += 16;
						while(*msg)
						{
							if (*msg == '\n')
								break;
							while (*msg == ' ' || *msg == '\t')
								msg++;
							if (*msg == ',')
							{
								msg++;
								continue;
							}
							e = msg;
							while(*e)
							{
								if (*e == ',' || *e == '\r' || *e == '\n')
									break;
								e++;
							}
							while(e > msg && (e[-1] == ' ' || e[-1] == '\t'))
								e--;
							if (e-msg == 4 && !strncmp(msg, "gzip", 4))
								cl->acceptgzip = true;
							while(*msg && *msg != '\n' && *msg != ',')
								msg++;
						}
					}
					else if (!strnicmp(msg, "Transfer-Encoding: ", 18))	//parse needed header fields
					{
						cl->closeaftertransaction = true;
						goto notimplemented;
					}
					else if (!strnicmp(msg, "Connection: close", 17))
						cl->closeaftertransaction = true;
					else if (!strnicmp(msg, "Connection: Keep-Alive", 22))
						cl->closeaftertransaction = false;

					while(*msg != '\n')
					{
						if (!*msg)
						{
							goto cont;
						}
						msg++;
					}
					msg++;
				}
			}
			else
			{
				HTTPmarkup = 0;	//strimmed... totally...
				cl->closeaftertransaction = true;
				//don't bother running to nl.
			}

			if (cl->inbufferused-(msg-cl->inbuffer) < contentlen)
				continue;

			cl->modeswitched = true;

			if (contentlen)
			{
				content = IWebMalloc(contentlen+1);
				memcpy(content, msg, contentlen+1);
			}

			memmove(cl->inbuffer, cl->inbuffer+(msg-cl->inbuffer+contentlen), cl->inbufferused-(msg-cl->inbuffer+contentlen));
			cl->inbufferused -= msg-cl->inbuffer+contentlen;


			if (HTTPmarkup == 3 && !hostspecified)	//1.1 requires the host to be specified... we ca,just ignore it as we're not routing or imitating two servers. (for complience we need to encourage the client to send - does nothing for compatability or anything, just compliance to spec. not always the same thing)
			{
				msg = "HTTP/1.1 400 Bad Request\r\n"	/*"Content-Type: application/octet-stream\r\n"*/		"Content-Length: 69\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n"	"400 Bad Request\r\nYour client failed to provide the host header line";

				IWebPrintf("%s: no host specified\n", cl->peername);

				ammount = strlen(msg);
				ExpandOutBuffer(cl, ammount, true);
				memcpy(cl->outbuffer, msg, ammount);
				cl->outbufferused = ammount;
				cl->mode = HTTP_SENDING;
			}
			else if (!stricmp(mode, "GET") || !stricmp(mode, "HEAD") || !stricmp(mode, "POST"))
			{
				qboolean gzipped = false;
				if (*resource != '/')
				{
					resource[0] = '/';
					resource[1] = 0;	//I'm lazy, they need to comply
				}
				IWebPrintf("%s: Download request for \"http://%s/%s\"\n", cl->peername, host, resource+1);

				if (!strnicmp(mode, "P", 1))	//when stuff is posted, data is provided. Give an error message if we couldn't do anything with that data.
					cl->file = IWebGenerateFile(resource+1, content, contentlen);
				else
				{
					char filename[MAX_OSPATH], *q;
					cl->file = NULL;
					Q_strncpyz(filename, resource+1, sizeof(filename));
					q = strchr(filename, '?');
					if (q) *q = 0;

					if (SV_AllowDownload(filename))
					{
						char nbuf[MAX_OSPATH];

						if (cl->acceptgzip && strlen(filename) < sizeof(nbuf)-4)
						{
							Q_strncpyz(nbuf, filename, sizeof(nbuf));
							Q_strncatz(nbuf, ".gz", sizeof(nbuf));
							cl->file = FS_OpenVFS(nbuf, "rb", FS_GAME);
						}
						if (cl->file)
							gzipped = true;
						else
							cl->file = FS_OpenVFS(filename, "rb", FS_GAME);
					}

					if (!cl->file)
					{
						cl->file = IWebGenerateFile(resource+1, content, contentlen);
					}
				}
				if (!cl->file)
				{
					IWebPrintf("%s: 404 - not found\n", cl->peername);

					if (HTTPmarkup >= 3)
						msg = "HTTP/1.1 404 Not Found\r\n"	"Content-Type: text/plain\r\n"		"Content-Length: 15\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n"	"404 Bad address";
					else if (HTTPmarkup == 2)
						msg = "HTTP/1.0 404 Not Found\r\n"	"Content-Type: text/plain\r\n"		"Content-Length: 15\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n"	"404 Bad address";
					else if (HTTPmarkup)
						msg = "HTTP/0.9 404 Not Found\r\n"																						"\r\n"	"404 Bad address";
					else
						msg = "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY>404 Not Found<BR>The specified file could not be found on the server</HEAD></HTML>";

					ammount = strlen(msg);
					ExpandOutBuffer(cl, ammount, true);
					memcpy(cl->outbuffer, msg, ammount);
					cl->outbufferused = ammount;
					cl->mode = HTTP_SENDING;
				}
				else
				{
					//fixme: add connection: keep-alive or whatever so that ie3 is happy...
					if (HTTPmarkup>=3)
						sprintf(resource, "HTTP/1.1 200 OK\r\n"		"%s%s"		"Connection: %s\r\n"	"Content-Length: %i\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n", strstr(resource, ".htm")?"Content-Type: text/html\r\n":"", gzipped?"Content-Encoding: gzip\r\nCache-Control: public, max-age=86400\r\n":"", cl->closeaftertransaction?"close":"keep-alive", (int)VFS_GETLEN(cl->file));
					else if (HTTPmarkup==2)
						sprintf(resource, "HTTP/1.0 200 OK\r\n"		"%s%s"		"Connection: %s\r\n"	"Content-Length: %i\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n", strstr(resource, ".htm")?"Content-Type: text/html\r\n":"", gzipped?"Content-Encoding: gzip\r\nCache-Control: public, max-age=86400\r\n":"", cl->closeaftertransaction?"close":"keep-alive", (int)VFS_GETLEN(cl->file));
					else if (HTTPmarkup)
						sprintf(resource, "HTTP/0.9 200 OK\r\n\r\n");
					else
						strcpy(resource, "");
					msg = resource;

					if (*mode == 'H' || *mode == 'h')
					{

						VFS_CLOSE(cl->file);
						cl->file = NULL;
					}

					ammount = strlen(msg);
					ExpandOutBuffer(cl, ammount, true);
					memcpy(cl->outbuffer, msg, ammount);
					cl->outbufferused = ammount;
					cl->mode = HTTP_SENDING;
				}
			}
			//PUT/POST must support chunked transfer encoding for 1.1 compliance.
/*			else if (!stricmp(mode, "PUT"))	//put is replacement of a resource. (file uploads)
			{
			}
*/
			else
			{
notimplemented:
				if (HTTPmarkup >= 3)
					msg = "HTTP/1.1 501 Not Implemented\r\n\r\n";
				else if (HTTPmarkup == 2)
					msg = "HTTP/1.0 501 Not Implemented\r\n\r\n";
				else if (HTTPmarkup)
					msg = "HTTP/0.9 501 Not Implemented\r\n\r\n";
				else
				{
					msg = NULL;
					cl->closereason = "unsupported http version";
				}
				IWebPrintf("%s: 501 - not implemented\n", cl->peername);

				if (msg)
				{
					ammount = strlen(msg);
					ExpandOutBuffer(cl, ammount, true);
					memcpy(cl->outbuffer, msg, ammount);
					cl->outbufferused = ammount;
					cl->mode = HTTP_SENDING;
				}
			}

			if (content)
				IWebFree(content);
			break;

		case HTTP_SENDING:
			if (cl->outbufferused < 8192)
			{
				if (cl->file)
				{
					ExpandOutBuffer(cl, 32768, true);
					wanted = cl->outbuffersize - cl->outbufferused;
					ammount = VFS_READ(cl->file, cl->outbuffer+cl->outbufferused, wanted);

					if (!ammount)
					{
						VFS_CLOSE(cl->file);
						cl->file = NULL;

						IWebPrintf("%s: Download complete\n", cl->peername);
					}
					else
						cl->outbufferused+=ammount;
				}
			}

			ammount = send(cl->datasock, cl->outbuffer, cl->outbufferused, 0);

			if (ammount == -1)
			{
				localerrno = neterrno();
				if (localerrno != NET_EWOULDBLOCK)
				{
					cl->closereason = "some error when sending";
				}
			}
			else if (ammount||!cl->outbufferused)
			{
				memcpy(cl->outbuffer, cl->outbuffer+ammount, cl->outbufferused-ammount);
				cl->outbufferused -= ammount;
				if (!cl->outbufferused && !cl->file)
				{
					cl->modeswitched = true;
					cl->mode = HTTP_WAITINGFORREQUEST;
					if (cl->closeaftertransaction)
						cl->closereason = "file sent";
				}
			}
			else
				cl->closereason = "peer prematurely closed connection";
			break;

	/*	case HTTP_RECEIVING:
			sent = recv(cl->datasock, resource, ammount, 0);
			if (sent == -1)
			{
				if (qerrno != EWOULDBLOCK)	//they closed on us. Assume end.
				{
					VFS_CLOSE(cl->file);
					cl->file = NULL;
					cl->close = true;
					continue;
				}
			}
			if (sent != 0)
				IWebFWrite(resource, 1, sent, cl->file);
			break;*/
		}
	}
}

#ifdef WEBSVONLY
void VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
#ifdef _WIN32
#undef _vsnprintf
	_vsnprintf (dest, size-1, fmt, args);
#else
	vsnprintf (dest, size-1, fmt, args);
#endif
	va_end (args);
	//make sure its terminated.
	dest[size-1] = 0;
}
#endif

qboolean HTTP_ServerPoll(qboolean httpserverwanted, int portnum)	//loop while true
{
	struct sockaddr_qstorage	from;
	int		fromlen;
	int clientsock;
	int _true = true;

	HTTP_active_connections_t *cl;

	if (httpserverport != portnum && httpserverinitied)
		HTTP_ServerShutdown();
	if (!httpserverinitied)
	{
		if (httpserverwanted)
			return HTTP_ServerInit(portnum);
		return false;
	}
	else if (!httpserverwanted)
	{
		HTTP_ServerShutdown();
		return false;
	}

	checknatpmp(httpserverport);

	if (httpconnectioncount>32)
		return false;

	fromlen = sizeof(from);
	clientsock = accept(httpserversocket, (struct sockaddr *)&from, &fromlen);

	if (clientsock == -1)
	{
		int e = neterrno();
		if (e == NET_EWOULDBLOCK)
		{
			HTTP_RunExisting();
			return false;
		}

		if (e == NET_ECONNABORTED || e == NET_ECONNRESET)
		{
			Con_TPrintf ("Connection lost or aborted\n");
			return false;
		}


		IWebPrintf ("NET_GetPacket: %s\n", strerror(e));
		return false;
	}

	if (ioctlsocket (clientsock, FIONBIO, (u_long *)&_true) == -1)
	{
		IWebPrintf ("HTTP_ServerInit: ioctl FIONBIO: %s\n", strerror(neterrno()));
		closesocket(clientsock);
		return false;
	}

	cl = IWebMalloc(sizeof(HTTP_active_connections_t));

#ifndef WEBSVONLY
	{
		netadr_t na;
		SockadrToNetadr(&from, &na);
		NET_AdrToString(cl->peername, sizeof(cl->peername), &na);
	}
#else
	Q_snprintfz(cl->peername, sizeof(cl->peername), "%s:%i", inet_ntoa(((struct sockaddr_in*)&from)->sin_addr), ntohs(((struct sockaddr_in*)&from)->sin_port));
#endif
	IWebPrintf("%s: New http connection\n", cl->peername);

	cl->datasock = clientsock;

	cl->next = HTTP_ServerConnections;
	HTTP_ServerConnections = cl;
	httpconnectioncount++;

	return true;
}

#endif
