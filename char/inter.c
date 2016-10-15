#include "mmo.h"
#include "char.h"
#include "socket.h"
#include "timer.h"
#include <string.h>
#include <stdlib.h>

#include "inter.h"
#include "int_party.h"
#include "int_guild.h"
#include "int_storage.h"
#include "int_pet.h"

#define WISLIST_TTL	(30*1000)	// Wis���X�g�̐�������(30�b)

// ���M�p�P�b�g�����X�g
int inter_send_packet_length[]={
	-1,-1,27, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	-1, 7, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	35,-1,11,15, 34,29, 7,-1,  0, 0, 0, 0,  0, 0,  0, 0,
	10,-1,15, 0, 79,19, 7,-1,  0,-1,-1,-1, 14,67,186,-1,
	 9, 9, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	11,-1, 7, 3,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
};
// ��M�p�P�b�g�����X�g
int inter_recv_packet_length[]={
	-1,-1, 5, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 6,-1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	72, 6,52,14, 10,29, 6,-1, 34, 0, 0, 0,  0, 0,  0, 0,
	-1, 6,-1, 0, 55,19, 6,-1, 14,-1,-1,-1, 14,19,186,-1,
	 5, 9, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
	48,14,-1, 6,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,
};

struct WisList {
	struct WisList* next;
	int id,fd;
	int count;
	unsigned long tick;
	unsigned char src[24];
	unsigned char dst[24];
	unsigned char msg[512];
	unsigned int len;
};
struct WisList *wis_list=NULL;
short wis_id=0;

// Wis���X�g�o�^
int add_wislist(struct WisList* list)
{
	
	list->next=wis_list;
	list->id=wis_id++;
	list->tick=gettick();
	wis_list=list;

	if(wis_id>30000)
		wis_id=0;
	return list->id;
}
// Wis���X�g����
struct WisList *search_wislist(int id)
{
	struct WisList* p;
	for(p=wis_list;p;p=p->next){
		if( p->id == id )
			return p;
	}
	return NULL;
}
// Wis���X�g�폜
int del_wislist(int id)
{
	struct WisList* p=wis_list, **prev=&wis_list;
//	printf("del_wislist:start\n");
	for( ; p; prev=&p->next,p=p->next ){
		if( p->id == id ){
			*prev=p->next;
			free(p);
//			printf("del_wislist:ok\n");
			return 1;
		}
	}
//	printf("del_wislist:not found\n");
	return 0;
}
// Wis���X�g�̐����`�F�b�N
int check_ttl_wislist()
{
	unsigned long tick=gettick();
	struct WisList* p=wis_list, **prev=&wis_list;
	for( ; p; prev=&p->next,p=p->next ){
		if( DIFF_TICK(tick,p->tick)>WISLIST_TTL ){
			*prev=p->next;
			free(p);
			p=*prev;
		}
	}
	return 0;
}

//--------------------------------------------------------

/*==========================================
 * �ݒ�t�@�C����ǂݍ���
 *------------------------------------------
 */
int inter_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		i=sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if(i!=2)
			continue;
		if(strcmpi(w1,"storage_txt")==0){
			strncpy(storage_txt,w2,sizeof(storage_txt));
		} else if(strcmpi(w1,"party_txt")==0){
			strncpy(party_txt,w2,sizeof(party_txt));
		} else if(strcmpi(w1,"guild_txt")==0){
			strncpy(guild_txt,w2,sizeof(guild_txt));
		} else if(strcmpi(w1,"pet_txt")==0){
			strncpy(pet_txt,w2,sizeof(pet_txt));
		} else if(strcmpi(w1,"castle_txt")==0){
			strncpy(castle_txt,w2,sizeof(castle_txt));
		}
	}
	fclose(fp);

	return 0;
}

// �Z�[�u
int inter_save()
{
	inter_storage_save();
	inter_party_save();
	inter_guild_save();
	inter_pet_save();

	return 0;
}

// ������
int inter_init(const char *file)
{
	inter_config_read(file);

	inter_storage_init();
	inter_party_init();
	inter_guild_init();
	inter_pet_init();

	return 0;
}

//--------------------------------------------------------

// GM���b�Z�[�W���M
int mapif_GMmessage(unsigned char *mes,int len)
{
	unsigned char buf[len];
	WBUFW(buf,0)=0x3800;
	WBUFW(buf,2)=len;
	memcpy(WBUFP(buf,4),mes,len-4);
	mapif_sendall(buf,len);
//	printf("inter server: GM:%d %s\n",len,mes);
	return 0;
}

// Wis���M
int mapif_wis_message(struct WisList *wl)
{
	unsigned char buf[1024];
	
	WBUFW(buf, 0)=0x3801;
	WBUFW(buf, 2)=6 + 48 +wl->len;
	WBUFW(buf, 4)=wl->id;
	memcpy(WBUFP(buf, 6),wl->src,24);
	memcpy(WBUFP(buf,30),wl->dst,24);
	memcpy(WBUFP(buf,54),wl->msg,wl->len);
	wl->count = mapif_sendall(buf,WBUFW(buf,2));
//	printf("inter server wis: %d %d %ld\n", wl->id, wl->count, wl->tick);
	return 0;
}
// Wis���M����
int mapif_wis_end(struct WisList *wl,int flag)
{
	unsigned char buf[32];
	
	WBUFW(buf, 0)=0x3802;
	memcpy(WBUFP(buf, 2),wl->src,24);
	WBUFB(buf,26)=flag;
	mapif_send(wl->fd,buf,27);
//	printf("inter server wis_end %d\n",flag);
	return 0;
}

//--------------------------------------------------------

// GM���b�Z�[�W���M
int mapif_parse_GMmessage(int fd)
{
	mapif_GMmessage(RFIFOP(fd,4),RFIFOW(fd,2));
	return 0;
}


// Wis���M�v��
int mapif_parse_WisRequest(int fd)
{
	struct WisList* wl = (struct WisList *)malloc(sizeof(struct WisList));
	if(wl==NULL){
		// Wis���M���s�i�p�P�b�g�𑗂�K�v���肩���j
		RFIFOSKIP(fd,RFIFOW(fd,2));
		return 0;
	}
	check_ttl_wislist();
	
	wl->fd=fd;	// WisList�Z�b�g
	memcpy(wl->src,RFIFOP(fd, 4),24);
	memcpy(wl->dst,RFIFOP(fd,28),24);
	wl->len=RFIFOW(fd,2)-52;
	memcpy(wl->msg,RFIFOP(fd,52),wl->len);
	
	add_wislist(wl);
	
	mapif_wis_message(wl);
	return 0;
}

// Wis���M����
int mapif_parse_WisReply(int fd)
{
	int id=RFIFOW(fd,2),flag=RFIFOB(fd,4);
	
	struct WisList* wl=search_wislist(id);
	
	if(wl==NULL){
		RFIFOSKIP(fd,5);
		return 0;	// �^�C���A�E�g������ID�����݂��Ȃ�
	}
	
	if((--wl->count)==0 || flag!=1){
		// �S�I�I��������A����I����I�����錋�ʂ��Ԃ���
		mapif_wis_end(wl,flag);
		del_wislist(wl->id);
	}
	
	return 0;
}

//--------------------------------------------------------

// map server ����̒ʐM�i�P�p�P�b�g�̂݉�͂��邱�Ɓj
// �G���[�Ȃ�0(false)�A�����ł����Ȃ�1�A
// �p�P�b�g��������Ȃ����2���������Ȃ���΂Ȃ�Ȃ�
int inter_parse_frommap(int fd)
{
	int cmd=RFIFOW(fd,0);
	int len=0;

	// inter�I�Ǌ����𒲂ׂ�
	if(cmd<0x3000 || cmd>=0x3000+( sizeof(inter_recv_packet_length)/
		sizeof(inter_recv_packet_length[0]) ) )
		return 0;

	// �p�P�b�g���𒲂ׂ�
	if(	(len=inter_check_length(fd,inter_recv_packet_length[cmd-0x3000]))==0 )
		return 2;
	
	switch(cmd){
	case 0x3000: mapif_parse_GMmessage(fd); break;
	case 0x3001: mapif_parse_WisRequest(fd); break;
	case 0x3002: mapif_parse_WisReply(fd); break;
	default:
		if( inter_party_parse_frommap(fd) )
			break;
		if( inter_guild_parse_frommap(fd) )
			break;
		if( inter_storage_parse_frommap(fd) )
			break;
		if( inter_pet_parse_frommap(fd) )
			break;
		return 0;
	}
	RFIFOSKIP(fd, len );
	return 1;
}

// RFIFO�̃p�P�b�g���m�F
// �K�v�p�P�b�g��������΃p�P�b�g���A�܂�����Ȃ����0
int inter_check_length(int fd,int length)
{
	if(length==-1){	// �σp�P�b�g��
		if(RFIFOREST(fd)<4)	// �p�P�b�g��������
			return 0;
		length = RFIFOW(fd,2);
	}
	
	if(RFIFOREST(fd)<length)	// �p�P�b�g������
		return 0;
	
	return length;
}

