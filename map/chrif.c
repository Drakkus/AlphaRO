// $Id: chrif.c,v 1.7 2004/02/15 15:05:24 rovert Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"
#include "timer.h"
#include "map.h"
#include "battle.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static const int packet_len_table[0x20]={
	60, 3,-1, 3,14,-1, 7, 6,		// 2af8-2aff
	 6,-1,10, 7,-1,41,40, 0,		// 2b00-2b07
	 6,30,-1,10,9,7,					// 2b08-2b0d
};

int char_fd;
static char char_ip_str[16];
static int char_ip;
static int char_port = 6121;
static char userid[24],passwd[24];
static int chrif_state;

// �ݒ�t�@�C���ǂݍ��݊֌W
/*==========================================
 *
 *------------------------------------------
 */
void chrif_setuserid(char *id)
{
	memcpy(userid,id,24);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setpasswd(char *pwd)
{
	memcpy(passwd,pwd,24);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setip(char *ip)
{
	memcpy(char_ip_str,ip,16);
	char_ip=inet_addr(char_ip_str);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setport(int port)
{
	char_port=port;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_isconnect(void)
{
	return chrif_state==2;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_save(struct map_session_data *sd)
{
	if(char_fd<0)
		return -1;

	pc_makesavestatus(sd);

	WFIFOW(char_fd,0)=0x2b01;
	WFIFOW(char_fd,2)=sizeof(sd->status)+12;
	WFIFOL(char_fd,4)=sd->bl.id;
	WFIFOL(char_fd,8)=sd->char_id;
	memcpy(WFIFOP(char_fd,12),&sd->status,sizeof(sd->status));
	WFIFOSET(char_fd,WFIFOW(char_fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_connect(int fd)
{
	WFIFOW(fd,0)=0x2af8;
	memcpy(WFIFOP(fd,2),userid,24);
	memcpy(WFIFOP(fd,26),passwd,24);
	WFIFOL(fd,50)=0;
	WFIFOL(fd,54)=clif_getip();
	WFIFOL(fd,58)=clif_getport();
	WFIFOSET(fd,60);

	return 0;
}

/*==========================================
 * �}�b�v���M
 *------------------------------------------
 */
int chrif_sendmap(int fd)
{
	int i;

	WFIFOW(fd,0)=0x2afa;
	for(i=0;i<map_num;i++)
		memcpy(WFIFOP(fd,4+i*16),map[i].name,16);
	WFIFOW(fd,2)=4+i*16;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}
/*==========================================
 * �}�b�v��M
 *------------------------------------------
 */
int chrif_recvmap(int fd)
{
	int i,j,ip,port;
	unsigned char *p=(unsigned char *)&ip;
	
	if(chrif_state<2)	// �܂�������
		return -1;
	
	ip=RFIFOL(fd,4);
	port=RFIFOW(fd,8);
	for(i=12,j=0;i<RFIFOW(fd,2);i+=16,j++){
		map_setipport(RFIFOP(fd,i),ip,port);
//		if(battle_config.etc_log)
//			printf("recv map %d %s\n",j,RFIFOP(fd,i));
	}
	if(battle_config.etc_log)
		printf("recv map on %d.%d.%d.%d:%d (%d maps)\n",p[0],p[1],p[2],p[3],port,j);

	return 0;
}
/*==========================================
 * �}�b�v�I�Ԉړ��̂��߂̃f�[�^�����v��
 *------------------------------------------
 */
int chrif_changemapserver(struct map_session_data *sd,char *name,int x,int y,int ip,short port)
{
	if(char_fd<0)
		return -1;

	WFIFOW(char_fd,0)=0x2b05;
	WFIFOL(char_fd,2)=sd->bl.id;
	WFIFOL(char_fd,6)=sd->login_id1;
	WFIFOL(char_fd,10)=sd->status.char_id;
	memcpy(WFIFOP(char_fd,14),name,16);
	WFIFOW(char_fd,30)=x;
	WFIFOW(char_fd,32)=y;
	WFIFOL(char_fd,34)=ip;
	WFIFOL(char_fd,38)=port;
	WFIFOB(char_fd,40)=sd->status.sex;
	WFIFOSET(char_fd,41);
	return 0;
}
/*==========================================
 * �}�b�v�I�Ԉړ�ack
 *------------------------------------------
 */
int chrif_changemapserverack(int fd)
{
	struct map_session_data *sd=map_id2sd(RFIFOL(fd,2));
	if(sd==NULL || sd->status.char_id!=RFIFOL(fd,10) )
		return -1;
	if(RFIFOL(fd,6)==1){
		if(battle_config.error_log)
			printf("map server change failed.\n");
		pc_authfail(sd->fd);
		return 0;
	}
	clif_changemapserver(sd,RFIFOP(fd,14),RFIFOW(fd,30),RFIFOW(fd,32),
		RFIFOL(fd,34),RFIFOW(fd,38));
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int chrif_connectack(int fd)
{
	if(RFIFOB(fd,2)){
		printf("chrif : connect char server failed %d\n",RFIFOB(fd,2));
		exit(1);
	}
	chrif_state = 1;

	chrif_sendmap(fd);

	// <Agit> Run Event [AgitInit]
	printf("NPC_Event:[OnAgitInit] do (%d) events (Agit Initialize).\n", npc_event_doall("OnAgitInit"));

	return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int chrif_sendmapack(int fd)
{
	if(RFIFOB(fd,2)){
		printf("chrif : send map list to char server failed %d\n",RFIFOB(fd,2));
		exit(1);
	}
	chrif_state = 2;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_authreq(struct map_session_data *sd)
{
	if(char_fd<0)
		return -1;

	WFIFOW(char_fd,0) = 0x2afc;
	WFIFOL(char_fd,2) = sd->bl.id;
	WFIFOL(char_fd,6) = sd->char_id;
	WFIFOL(char_fd,10)= sd->login_id1;

	WFIFOSET(char_fd,14);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_charselectreq(struct map_session_data *sd)
{
	if(char_fd<0)
		return -1;

	WFIFOW(char_fd,0)=0x2b02;
	WFIFOL(char_fd,2)=sd->bl.id;
	WFIFOL(char_fd,6)=sd->login_id1;
	WFIFOSET(char_fd,10);
	return 0;
}

/*==========================================
 * �L�������₢���킹
 *------------------------------------------
 */
int chrif_searchcharid(int char_id)
{
	if(char_fd<0)
		return -1;

	WFIFOW(char_fd,0)=0x2b08;
	WFIFOL(char_fd,2)=char_id;
	WFIFOSET(char_fd,6);
	return 0;
}
/*==========================================
 * GM�ɕω��v��
 *------------------------------------------
 */
int chrif_changegm(int id,const char *pass,int len)
{
	if(char_fd<0)
		return -1;

	WFIFOW(char_fd,0)=0x2b0a;
	WFIFOW(char_fd,2)=len+8;
	WFIFOL(char_fd,4)=id;
	memcpy(WFIFOP(char_fd,8),pass,len);
//	if(battle_config.etc_log)
//		printf("chrif_changegm: %d %s %d\n",WFIFOL(char_fd,4),WFIFOP(char_fd,8),WFIFOW(char_fd,2));
	WFIFOSET(char_fd,len+8);
	return 0;
}
/*==========================================
 * ���ʕω��v��
 *------------------------------------------
 */
int chrif_changesex(int id,int sex)
{
	if(char_fd<0)
		return -1;

	WFIFOW(char_fd,0)=0x2b0c;
	WFIFOW(char_fd,2)=9;
	WFIFOL(char_fd,4)=id;
	WFIFOB(char_fd,8)=sex;
	printf("chrif : sended 0x2b0c\n");
	WFIFOSET(char_fd,9);
	return 0;
}
/*==========================================
 * GM�ɕω��I��
 *------------------------------------------
 */
int chrif_changedgm(int fd)
{
	int oldacc,newacc;
	oldacc=RFIFOL(fd,2);
	newacc=RFIFOL(fd,6);
	if(battle_config.etc_log)
		printf("chrif_changedgm %d -> %d\n",oldacc,newacc);
	if(newacc>0){
		struct map_session_data *sd=map_id2sd(oldacc);
		if(sd!=NULL){	// GM�ύX�ɂ�鋭���ؒf
			clif_displaymessage(sd->fd,"GM�ύX�����B�Đڑ����ĉ������B");
			clif_setwaitclose(sd->fd);
		}
	}else{
		struct map_session_data *sd=map_id2sd(oldacc);
		if(sd!=NULL){
			clif_displaymessage(sd->fd,"GM�ύX���s");
		}
	}
	return 0;
}
/*==========================================
 * ���ʕω��I��
 *------------------------------------------
 */
int chrif_changedsex(int fd)
{
	int acc;
	struct map_session_data *sd;
	acc=RFIFOL(fd,2);
	if(battle_config.etc_log)
		printf("chrif_changedsex %d \n",acc);
	sd=map_id2sd(acc);
	if(acc>0){
		if(sd!=NULL){	// �ύX�ɂ�鋭���ؒf
			clif_setwaitclose(sd->fd);			
		}
	}else{
		if(sd!=NULL){
			printf("chrif_changedsex failed\n");
		}
	}
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int chrif_parse(int fd)
{
	int packet_len,cmd;

	if(session[fd]->eof){
		if(fd==char_fd)
			char_fd=-1;
		close(fd);
		delete_session(fd);
		return 0;
	}
	while(RFIFOREST(fd)>=2){
		cmd = RFIFOW(fd,0);
		if(cmd<0x2af8 || cmd>=0x2af8+(sizeof(packet_len_table)/sizeof(packet_len_table[0])) ||
		   packet_len_table[cmd-0x2af8]==0){
		   
		   	int r=intif_parse(fd);// intif�ɓn��
		   
			if( r==1 )	continue;	// intif�ŏ�������
			if( r==2 )	return 0;	// intif�ŏ����������A�f�[�^������Ȃ�
			
			close(fd);	// intif�ŏ����ł��Ȃ�����
			session[fd]->eof = 1;
			return 0;
		}
		packet_len = packet_len_table[cmd-0x2af8];
		if(packet_len==-1){
			if(RFIFOREST(fd)<4)
				return 0;
			packet_len = RFIFOW(fd,2);
		}
		if(RFIFOREST(fd)<packet_len)
			return 0;

		switch(cmd){
		case 0x2af9: chrif_connectack(fd); break;
		case 0x2afb: chrif_sendmapack(fd); break;
		case 0x2afd: pc_authok(RFIFOL(fd,4),(struct mmo_charstatus*)RFIFOP(fd,12)); break;
		case 0x2afe: pc_authfail(RFIFOL(fd,4)); break;
		case 0x2b00: map_setusers(RFIFOL(fd,2)); break;
		case 0x2b03: clif_charselectok(RFIFOL(fd,2)); break;
		case 0x2b04: chrif_recvmap(fd); break;
		case 0x2b06: chrif_changemapserverack(fd); break;
		case 0x2b09: map_addchariddb(RFIFOL(fd,2),RFIFOP(fd,6)); break;
		case 0x2b0b: chrif_changedgm(fd); break;
		case 0x2b0d: chrif_changedsex(fd); break;

		default:
			if(battle_config.error_log)
				printf("chrif_parse : unknown packet %d %d\n",fd,RFIFOW(fd,0));
			close(fd);
			session[fd]->eof=1;
			return 0;
		}
		RFIFOSKIP(fd,packet_len);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// timer�֐�
// ������map�I�Ɍq�����Ă���N���C�A���g�l����char�I�֑���
int send_users_tochar(int tid,unsigned int tick,int id,int data)
{
	if(char_fd<=0 || session[char_fd]==NULL)
		return 0;

	WFIFOW(char_fd,0)=0x2aff;
	WFIFOL(char_fd,2)=clif_countusers();
	WFIFOSET(char_fd,6);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// timer�֐�
// char�I�Ƃ̐ڑ����m�F���A�����؂�Ă�����ēx�ڑ�����
int check_connect_char_server(int tid,unsigned int tick,int id,int data)
{
	if(char_fd<=0 || session[char_fd]==NULL){
		chrif_state = 0;
		char_fd=make_connection(char_ip,char_port);
		session[char_fd]->func_parse=chrif_parse;

		chrif_connect(char_fd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_chrif(void)
{
	add_timer_func_list(check_connect_char_server,"check_connect_char_server");
	add_timer_func_list(send_users_tochar,"send_users_tochar");
	add_timer_interval(gettick()+1000,check_connect_char_server,0,0,10*1000);
	add_timer_interval(gettick()+1000,send_users_tochar,0,0,5*1000);

	return 0;
}
