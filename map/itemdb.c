// $Id: itemdb.c,v 1.11 2004/02/16 19:48:53 rovert Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "map.h"
#include "battle.h"
#include "itemdb.h"
#include "script.h"
#include "pc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MAX_RANDITEM	2000

static struct dbt* item_db;

static struct random_item_data blue_box[MAX_RANDITEM],violet_box[MAX_RANDITEM],card_album[MAX_RANDITEM],gift_box[MAX_RANDITEM],scroll[MAX_RANDITEM];
static int blue_box_count=0,violet_box_count=0,card_album_count=0,gift_box_count=0,scroll_count=0;
static int blue_box_default=0,violet_box_default=0,card_album_default=0,gift_box_default=0,scroll_default=0;

/*==========================================
 * ���O�Ō����p
 *------------------------------------------
 */
int itemdb_searchname_sub(void *key,void *data,va_list ap)
{
	struct item_data *item=(struct item_data *)data,**dst;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct item_data **);
	if( strcmpi(item->name,str)==0 || strcmp(item->jname,str)==0 ||
		memcmp(item->name,str,24)==0 || memcmp(item->jname,str,24)==0 )
		*dst=item;
	return 0;
}
/*==========================================
 * ���O�Ō���
 *------------------------------------------
 */
struct item_data* itemdb_searchname(const char *str)
{
	struct item_data *item=NULL;
	numdb_foreach(item_db,itemdb_searchname_sub,str,&item);
	return item;
}

/*==========================================
 * ���n�A�C�e������
 *------------------------------------------
 */
int itemdb_searchrandomid(int flags)
{
	int nameid=0,i,index,count;
	struct random_item_data *list=NULL;

	struct {
		int nameid,count;
		struct random_item_data *list;
	} data[] ={
		{ 0,0,NULL },
		{ blue_box_default	,blue_box_count		,blue_box	 },
		{ violet_box_default,violet_box_count	,violet_box	 },
		{ card_album_default,card_album_count	,card_album	 },
		{ gift_box_default	,gift_box_count		,gift_box	 },
		{ scroll_default	,scroll_count		,scroll		 },
	};

	if(flags>=1 && flags<=5){
		nameid=data[flags].nameid;
		count=data[flags].count;
		list=data[flags].list;

		if(count > 0) {
			for(i=0;i<1000;i++) {
				index = rand()%count;
				if(	rand()%1000000 < list[index].per) {
					nameid = list[index].nameid;
					break;
				}
			}
		}
	}
	return nameid;
}

/*==========================================
 * DB�̑��݊m�F
 *------------------------------------------
 */
struct item_data* itemdb_exists(int nameid)
{
	return numdb_search(item_db,nameid);
}
/*==========================================
 * DB�̌���
 *------------------------------------------
 */
struct item_data* itemdb_search(int nameid)
{
	struct item_data *id;

	id=numdb_search(item_db,nameid);
	if(id) return id;

	id=malloc(sizeof(struct item_data));
	if(id==NULL){
		printf("out of memory : itemdb_search\n");
		exit(1);
	}
	memset(id,0,sizeof(struct item_data));
	numdb_insert(item_db,nameid,id);

	id->nameid=nameid;
	id->value_buy=10;
	id->value_sell=id->value_buy/2;
	id->weight=10;
	id->sex=2;
	id->elv=0;
	id->class=0xffffffff;
	id->flag.available=0;
	id->flag.value_notdc=0;  //�ꉞ�E�E�E
	id->flag.value_notoc=0;
	id->flag.no_equip=0;	
	id->view_id=0;

	if(nameid>500 && nameid<600)
		id->type=0;   //heal item
	else if(nameid>900 && nameid<1100)
		id->type=3;   //loot
	else if(nameid>=1750 && nameid<1771)	// taulin: remove later
		id->type=10;  //arrow
	else if(nameid>1100 && nameid<1700)
		id->type=4;   //weapon
	else if(nameid>2100 && nameid<3000)
		id->type=5;   //armor
	else if(nameid>1700 && nameid<1750)
		id->type=10;   //bow
	return id;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip(int nameid)
{
	int type=itemdb_type(nameid);
	if(type==0 || type==3 || type==10)
		return 0;
	return 1;
}
/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip2(struct item_data *data)
{
	if(data) {
		int type=data->type;
		if(type==0 || type==3 || type==10)
			return 0;
		else
			return 1;
	}
	return 0;
}

//
// ������
//
/*==========================================
 *
 *------------------------------------------
 */

/*==========================================
 * �A�C�e���f�[�^�x�[�X�̓ǂݍ���
 *------------------------------------------
 */
static int itemdb_readdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p,*np;
	struct item_data *id;
	int i=0;
	char *filename[]={ "db/item_db.txt","db/item_db2.txt" };

	for(i=0;i<2;i++){

		fp=fopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			printf("can't read %s\n",filename[i]);
			return -1;
		}

		while(fgets(line,1020,fp)){
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,np=p=line;j<17 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p){ *p++=0; np=p; }
			}
			if(str[0]==NULL)
				continue;
			
			nameid=atoi(str[0]);
			if(nameid<=0 || nameid>=20000)
				continue;
			ln++;

			//ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Gender,Loc,wLV,eLV,View
			id=itemdb_search(nameid);
			memcpy(id->name,str[1],24);
			memcpy(id->jname,str[2],24);
			id->type=atoi(str[3]);
			// buy��sell*2 �� item_value_db.txt �Ŏw�肵�Ă��������B
			if (atoi(str[5])) {		// sell�l��D��Ƃ���
				id->value_buy=atoi(str[5])*2;
				id->value_sell=atoi(str[5]);
			} else {
				id->value_buy=atoi(str[4]);
				id->value_sell=atoi(str[4])/2;
			}
			id->weight=atoi(str[6]);
			id->atk=atoi(str[7]);
			id->def=atoi(str[8]);
			id->range=atoi(str[9]);
			id->slot=atoi(str[10]);
			id->class=atoi(str[11]);
			id->sex=atoi(str[12]);
			if(id->equip != atoi(str[13])){
				id->equip=atoi(str[13]);
			}
			id->wlv=atoi(str[14]);
			id->elv=atoi(str[15]);
			id->look=atoi(str[16]);
			id->flag.available=1;
			id->flag.value_notdc=0;
			id->flag.value_notoc=0;
			id->view_id=0;

			id->use_script=NULL;
			id->equip_script=NULL;

			if((p=strchr(np,'{'))==NULL)
				continue;
			id->use_script = parse_script(p,0);
			if((p=strchr(p+1,'{'))==NULL)
				continue;
			id->equip_script = parse_script(p,0);
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[i],ln);
	}
	return 0;
}

/*==========================================
 * �A�C�e�����i�e�[�u���̃I�[�o�[���C�h
 *------------------------------------------
 */
// taulin: fuck this db
/*static int itemdb_read_itemvaluedb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;
	
	if( (fp=fopen("db/item_value_db.txt","r"))==NULL){
		printf("can't read db/item_value_db.txt\n");
		return -1;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<7 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;

		if( !(id=itemdb_exists(nameid)) )
			continue;

		ln++;
		// ���ꂼ��L�q�������̂݃I�[�o�[���C�g
		if(str[3]!=NULL && *str[3]){
			id->value_buy=atoi(str[3]);
		}
		if(str[4]!=NULL && *str[4]){
			id->value_sell=atoi(str[4]);
		}
		if(str[5]!=NULL && *str[5]){
			id->flag.value_notdc=(atoi(str[5])==0)? 0:1;
		}
		if(str[6]!=NULL && *str[6]){
			id->flag.value_notoc=(atoi(str[6])==0)? 0:1;
		}
	}
	fclose(fp);
	printf("read db/item_value_db.txt done (count=%d)\n",ln);
	return 0;
}*/

/*==========================================
 * �����\�E�ƃe�[�u���̃I�[�o�[���C�h
 *------------------------------------------
 */
static int itemdb_read_classequipdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,i,j;
	char *str[32],*p;
	struct item_data *id;
	
	if( (fp=fopen("db/class_equip_db.txt","r"))==NULL ){
		printf("can't read db/class_equip_db.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<MAX_PC_CLASS+4 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;

		ln++;

		//ID,Name,class1,class2,class3, ...... ,class22
		if( !(id=itemdb_exists(nameid)) )
			continue;
			
		id->class = 0;
		if(str[2] && str[2][0]) {
			j = atoi(str[2]);
			id->sex = j;
		}
		if(str[3] && str[3][0]) {
			j = atoi(str[3]);
			id->elv = j;
		}
		for(i=4;i<MAX_PC_CLASS+4;i++) {
			if(str[i]!=NULL && str[i][0]) {
				j = atoi(str[i]);
				if(j == 99) {
					for(j=0;j<MAX_PC_CLASS;j++)
						id->class |= 1<<j;
					break;
				}
				else if(j == 9999)
					break;
				else if(j >= 0 && j < MAX_PC_CLASS) {
					id->class |= 1<<j;
					if(j == 7)
						id->class |= 1<<13;
					if(j == 14)
						id->class |= 1<<21;
				}
			}
		}
	}
	fclose(fp);
	printf("read db/class_equip_db.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * �����_���A�C�e���o���f�[�^�̓ǂݍ���
 *------------------------------------------
 */
static int itemdb_read_randomitem()
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,i,j;
	char *str[10],*p;
	
	const struct {
		char filename[64];
		struct random_item_data *pdata;
		int *pcount,*pdefault;
	} data[] = {
		{"db/item_bluebox.txt",		blue_box,	&blue_box_count, &blue_box_default		},
		{"db/item_violetbox.txt",	violet_box,	&violet_box_count, &violet_box_default	},
		{"db/item_cardalbum.txt",	card_album,	&card_album_count, &card_album_default	},
		{"db/item_giftbox.txt",	gift_box,	&gift_box_count, &gift_box_default	},
		{"db/item_scroll.txt",	scroll,	&scroll_count, &scroll_default	},
	};

	for(i=0;i<sizeof(data)/sizeof(data[0]);i++){
		struct random_item_data *pd=data[i].pdata;
		int *pc=data[i].pcount;
		int *pdefault=data[i].pdefault;
		char *fn=data[i].filename;

		*pdefault = 0;
		if( (fp=fopen(fn,"r"))==NULL ){
			printf("can't read %s\n",fn);
			continue;
		}

		while(fgets(line,1020,fp)){
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			if(str[0]==NULL)
				continue;

			nameid=atoi(str[0]);
			if(nameid<0 || nameid>=20000)
				continue;
			if(nameid == 0) {
				if(str[2])
					*pdefault = atoi(str[2]);
				continue;
			}

			if(str[2]){
				pd[ *pc   ].nameid = nameid;
				pd[(*pc)++].per = atoi(str[2]);
			}

			if(ln >= MAX_RANDITEM)
				break;
			ln++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",fn,*pc);
	}

	return 0;
}
/*==========================================
 * �A�C�e���g�p�\�t���O�̃I�[�o�[���C�h
 *------------------------------------------
 */
static int itemdb_read_itemavail(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j,k;
	char *str[10],*p;
	
	if( (fp=fopen("db/item_avail.txt","r"))==NULL ){
		printf("can't read db/item_avail.txt\n");
		return -1;
	}
	
	while(fgets(line,1020,fp)){
		struct item_data *id;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<2 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}

		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<0 || nameid>=20000 || !(id=itemdb_exists(nameid)) )
			continue;
		k=atoi(str[1]);
		if(k > 0) {
			id->flag.available = 1;
			id->view_id = k;
		}
		else
			id->flag.available = 0;
		ln++;
	}
	fclose(fp);
	printf("read db/item_avail.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * ���������t�@�C���ǂݏo��
 *------------------------------------------
 */
static int itemdb_read_noequip(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;
	
	if( (fp=fopen("db/item_noequip.txt","r"))==NULL ){
		printf("can't read db/item_noequip.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<2 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000 || !(id=itemdb_exists(nameid)))
			continue;

		id->flag.no_equip=atoi(str[1]);

		ln++;

	}
	fclose(fp);
	printf("read db/item_noequip.txt done (count=%d)\n",ln);
	return 0;
}

static int itemdb_read_descdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;
	
	if( (fp=fopen("db/item_description.txt","r"))==NULL ){
		printf("can't read db/item_noequip.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
// taulin modify
// adding return ability, extending length of description
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<2 && p;j++){
			str[j]=p;
			printf("itemid: %s",str[j]);
			p=strchr(p,'#');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000 || !(id=itemdb_exists(nameid)))
			continue;

		memcpy(id->desc,str[1],500);
		ln++;

	}
	fclose(fp);
	printf("read db/item_description.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_final(void *key,void *data,va_list ap)
{
	struct item_data *id;

	id=data;
	if(id->use_script)
		free(id->use_script);
	if(id->equip_script)
		free(id->equip_script);
	free(id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void do_final_itemdb(void)
{
	if(item_db){
		numdb_final(item_db,itemdb_final);
		item_db=NULL;
	}
}

/*
static FILE *dfp;
static int itemdebug(void *key,void *data,va_list ap){
//	struct item_data *id=(struct item_data *)data;
	fprintf(dfp,"%6d",(int)key);
	return 0;
}
void itemdebugtxt()
{
	dfp=fopen("itemdebug.txt","wt");
	numdb_foreach(item_db,itemdebug);
	fclose(dfp);
}
*/

/*==========================================
 *
 *------------------------------------------
 */
int do_init_itemdb(void)
{
	item_db = numdb_init();

	itemdb_readdb();
	itemdb_read_classequipdb();
	// taulin: fuck this db off	
	// itemdb_read_itemvaluedb();
	itemdb_read_randomitem();
	itemdb_read_itemavail();
	itemdb_read_noequip();
	itemdb_read_descdb();
	
	return 0;
}
