#include "inter.h"
#include "int_party.h"
#include "mmo.h"
#include "char.h"
#include "socket.h"

#include <string.h>
#include <stdlib.h>

#define STORAGE_MEMINC	16
// �t�@�C�����̃f�t�H���g
// inter_config_read()�ōĐݒ肳���
char storage_txt[1024]="storage.txt";


struct storage *storage=NULL;
int storage_num=0;

// �q�Ƀf�[�^�𕶎���ɕϊ�
int storage_tostr(char *str,struct storage *p)
{
	int i,f=1;
	sprintf(str,"%d,%d",p->account_id,p->storage_amount);

	strcat(str,"\t");
	for(i=0;i<MAX_STORAGE;i++)
	if( (p->storage[i].nameid) && (p->storage[i].amount) ){
		f=0;
		sprintf(str+strlen(str),"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",
		  p->storage[i].id,p->storage[i].nameid,p->storage[i].amount,p->storage[i].equip,
		  p->storage[i].identify,p->storage[i].refine,p->storage[i].attribute,
		  p->storage[i].card[0],p->storage[i].card[1],p->storage[i].card[2],p->storage[i].card[3]);
	}

	strcat(str,"\t");

	if(f)
		str[0]=0;
	return 0;
}

// �������q�Ƀf�[�^�ɕϊ�
int storage_fromstr(char *str,struct storage *p)
{
	int tmp_int[256];
	int set,next,len,i;

	set=sscanf(str,"%d,%d%n",&tmp_int[0],&tmp_int[1],&next);
	p->storage_amount=tmp_int[1];

	if(set!=2)
		return 0;
	if(str[next]=='\n' || str[next]=='\r')
		return 1;	
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		  &tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
		  &tmp_int[4],&tmp_int[5],&tmp_int[6],
		  &tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&len);
		if(set!=11)
			return 0;
		p->storage[i].id=tmp_int[0];
		p->storage[i].nameid=tmp_int[1];
		p->storage[i].amount=tmp_int[2];
		p->storage[i].equip=tmp_int[3];
		p->storage[i].identify=tmp_int[4];
		p->storage[i].refine=tmp_int[5];
		p->storage[i].attribute=tmp_int[6];
		p->storage[i].card[0]=tmp_int[7];
		p->storage[i].card[1]=tmp_int[8];
		p->storage[i].card[2]=tmp_int[9];
		p->storage[i].card[3]=tmp_int[10];

		next+=len;
		if(str[next]==' ')
			next++;
	}
	return 1;
}

// �A�J�E���g����q�Ƀf�[�^�C���f�b�N�X�𓾂�i�V�K�q�ɒǉ��\�j
int account2storage(int account_id)
{
	int i;
	for(i=0;i<storage_num;i++)
		if(account_id==storage[i].account_id)
			return i;
	if(i==0){
		storage=malloc(sizeof(struct storage));
		storage_num=1;
	}else{
		storage=realloc(storage,sizeof(struct storage)*(++storage_num));
	}
	memset(&storage[i],0,sizeof(struct storage));
	storage[i].account_id=account_id;
	return i;
}

//---------------------------------------------------------
// �q�Ƀf�[�^��ǂݍ���
int inter_storage_init()
{
	char line[65536];
	int i=0,set,tmp_int[2];
	FILE *fp;
	fp=fopen(storage_txt,"r");
	if(fp==NULL){
		printf("cant't read : %s\n",storage_txt);
		return 0;
	}
	while(fgets(line,65535,fp)){
		set=sscanf(line,"%d,%d",&tmp_int[0],&tmp_int[1]);
		if(set==2) {
			if(i==0){
				storage=malloc(sizeof(struct storage));
			}else{
				storage=realloc(storage,sizeof(struct storage)*(i+1));
			}
			memset(&storage[i],0,sizeof(struct storage));
			storage[i].account_id=tmp_int[0];
			storage_fromstr(line,&storage[i]);
			i++;
		}
	}
	storage_num=i;
	fclose(fp);
	return 1;

}
//---------------------------------------------------------
// �q�Ƀf�[�^����������
int inter_storage_save()
{
	char line[65536];
	int i;
	FILE *fp;

	fp=fopen(storage_txt,"w");
	if(fp==NULL)
		return 0;
	for(i=0;i<storage_num;i++){
		storage_tostr(line,&storage[i]);
		if(line[0])
			fprintf(fp,"%s" RETCODE,line);
	}
	fclose(fp);
	
	return 0;
}
//---------------------------------------------------------
// map server�ւ̒ʐM

// �q�Ƀf�[�^�̑��M
int mapif_load_storage(int fd,int account_id)
{
	int i=account2storage(account_id);
	WFIFOW(fd,0)=0x3810;
	WFIFOW(fd,2)=sizeof(struct storage)+8;
	WFIFOL(fd,4)=account_id;
	memcpy(WFIFOP(fd,8),&storage[i],sizeof(struct storage));
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
// �q�Ƀf�[�^�ۑ��������M
int mapif_save_storage_ack(int fd,int account_id)
{
	WFIFOW(fd,0)=0x3811;
	WFIFOL(fd,2)=account_id;
	WFIFOB(fd,6)=0;
	WFIFOSET(fd,7);
	return 0;
}

//---------------------------------------------------------
// map server����̒ʐM

// �q�Ƀf�[�^�v����M
int mapif_parse_LoadStorage(int fd)
{
	mapif_load_storage(fd,RFIFOL(fd,2));
	return 0;
}
// �q�Ƀf�[�^��M���ۑ�
int mapif_parse_SaveStorage(int fd)
{
	int account_id=RFIFOL(fd,4);
	int len=RFIFOW(fd,2);
	int i;
	if(sizeof(struct storage)!=len-8){
		printf("inter storage: data size error %d %d\n",sizeof(struct storage),len-8);
	}else{
		i=account2storage(account_id);
		memcpy(&storage[i],RFIFOP(fd,8),sizeof(struct storage));
		mapif_save_storage_ack(fd,account_id);
	}
	return 0;
}

// map server ����̒ʐM
// �E�P�p�P�b�g�̂݉�͂��邱��
// �E�p�P�b�g���f�[�^��inter.c�ɃZ�b�g���Ă�������
// �E�p�P�b�g���`�F�b�N��ARFIFOSKIP�͌Ăяo�����ōs����̂ōs���Ă͂Ȃ�Ȃ�
// �E�G���[�Ȃ�0(false)�A�����łȂ��Ȃ�1(true)���������Ȃ���΂Ȃ�Ȃ�
int inter_storage_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3010: mapif_parse_LoadStorage(fd); break;
	case 0x3011: mapif_parse_SaveStorage(fd); break;
	default:
		return 0;
	}
	return 1;
}

