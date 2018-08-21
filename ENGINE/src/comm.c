/*
 * communication.c
 *
 *  Created on: Aug 15, 2018
 *      Author: Administrator
 */

#include "comm.h"
#include "junqi.h"

#if 1
void memout(u8 *pdata,u8 len)
{
	int i;
	for(i=0;i<len;i++)
	{
		printf("%02X ",*(pdata+i));
		if((i+1)%8==0)
		{
			printf("\n");
		}
	}
	printf("\n");
}
#endif
////////////////////////////////

void PacketHeader(CommHeader *header, u8 iDir, u8 eFun)
{
	memset(header, 0, sizeof(CommHeader));
	memcpy(header->aMagic, aMagic, 4);
	//目前该位只在COMM_START标识先手位
	header->iDir = iDir;
	header->eFun = eFun;
}

void SendData(Junqi* pJunqi, CommHeader *header, void *data, int len)
{
	u8 buf[100];
	int length = 0;

	length += sizeof(CommHeader);
	memcpy(buf, header, length);

	memcpy(buf+length, data, len);
	length += len;

	sendto(pJunqi->socket_fd, buf, length, 0,
			(struct sockaddr *)&pJunqi->addr, sizeof(struct sockaddr));
	log_a("send");
	memout(buf,length);
}

void SendHeader(Junqi* pJunqi, u8 iDir, u8 eFun)
{
	CommHeader header;
	PacketHeader(&header, iDir, eFun);
	SendData(pJunqi, &header, NULL, 0);
}

void SetRecLineup(Junqi* pJunqi, u8 *data, int iDir)
{
	int i;
	for(i=0; i<30; i++)
	{
		pJunqi->Lineup[iDir][i].type = data[i];
		//assert( pJunqi->Lineup[iDir][i].type!=DARK );
	}

}

void SendMove(Junqi* pJunqi,  BoardChess *pSrc, BoardChess *pDst)
{
	MoveResultData send_data;
	CommHeader header;

	memset(&send_data, 0, sizeof(MoveResultData));
	send_data.src[0] = pSrc->point.x;
	send_data.src[1] = pSrc->point.y;
	send_data.dst[0] = pDst->point.x;
	send_data.dst[1] = pDst->point.y;

	PacketHeader(&header, pJunqi->eTurn, COMM_MOVE);
	SendData(pJunqi, &header, &send_data, sizeof(MoveResultData));
}

void SendEvent(Junqi* pJunqi, int iDir, u8 event)
{
	CommHeader header;
	u8 data = event;
	PacketHeader(&header, iDir, COMM_EVNET);
	SendData(pJunqi, &header, &data, 1);
}

void *comm_thread(void *arg)
{
	Junqi* pJunqi = (Junqi*)arg;
	int socket_fd;
	struct sockaddr_in addr,local;
	size_t recvbytes = 0;
	u8 buf[REC_LEN]={0};

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0)
	{
		printf("Create Socket Failed!\n");
		pthread_detach(pthread_self());
	}

	local.sin_family = AF_INET;
	local.sin_addr.s_addr=INADDR_ANY;
	local.sin_port = htons(5678);
	if(bind(socket_fd, (struct sockaddr *)&local, sizeof(struct sockaddr) )<0)
	{
		printf("Bind Error!\n");
		pthread_detach(pthread_self());
	}

	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(1234);

	pJunqi->socket_fd = socket_fd;
	pJunqi->addr = addr;

	SendHeader(pJunqi, 0, COMM_READY);

	while(1)
	{
		recvbytes=recvfrom(socket_fd, buf, 200, 0,NULL ,NULL);
		mq_send(pJunqi->qid, (char*)buf, recvbytes, 0);

	}

	pthread_detach(pthread_self());
	return NULL;
}

pthread_t CreatCommThread(Junqi* pJunqi)
{
    pthread_t tidp;
    pthread_create(&tidp,NULL,(void*)comm_thread,pJunqi);
    return tidp;
}


