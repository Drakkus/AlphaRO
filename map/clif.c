// $Id: clif.c,v 1.34 2004/02/18 21:54:30 rovert Exp 

#define DUMP_UNKNOWN_PACKET	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"
#include "timer.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"
#include "itemdb.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "script.h"
#include "skill.h"
#include "atcommand.h"
#include "intif.h"
#include "battle.h"
#include "mob.h"
#include "party.h"
#include "guild.h"
#include "vending.h"
#include "pet.h"
#include "version.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static const int packet_len_table[0x200]={
   10,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15, 11,
    3,  0,  0,  0,  19, 18,  23,  25,   18,  2,  2,  6,  7,  3,  2,  2,
    0,  5,  0, 12, 10,  7,  17,-1,  -1, -1,  0,  7,  22,  28,  2,  6,
   38,  -1,  -1,  3, -1, -1,  3,  7,  30,  30,  6,  25,  6,  6, -1, -1,
//#0x0040
   -1, -1,   8,  7,  5,  6,  4,  6,  18, -1,  6,  6,  8,  3,  3,  -1,
    6,  6,  -1,  7,  6,  2,  5,  6,  20,  5,  3,  7,  2,  6,  8,  6,
    7, -1,  -1, -1, -1, 3,  3, 18,  3,  2, 19, 3,  4,  4,  2,  -1,
   -1,  3,  -1,  6, 14, 3, -1, 20,  21, -1, -1, 22, 22, 18, 2, 6,
//#0x0080
    18,  3,  3,  8, 22,  5, 2,  3,  2,  2,  2, 3,  2,  6,  8,  24,
     8,  8,  2,  2,  18, 3, -1, 6,  19, 22, 10,  2,  22, 63, 23, 10,
   10, -1, -1, 18, 6, 6, 2,  9,  -1,  6,  7,  4,  7,  0, -1,  6,
    0,  0,  3,  3, -1,  6,  6, -1,   7,  6,  2,  5,  6, 44,  5,  0,
//#0x00C0
    0,  0,  0,  8,  6,  7, -1, -1,  -1, -1,  3,  3,  6,  6,  2, 27,
    3,  4,  4,  2, -1, -1,  3, -1,   6, 14,  3, -1, 28, 29, -1, -1,
   30, 30, 26,  2,  6, 26,  3,  3,   8, 19,  5,  2,  3,  2,  2,  2,
    3,  2,  6,  8, 21,  8,  8,  2,   2, 26,  3, -1,  6, 27, 30, 10,

//#0x0100
    2,  6,  6, 30, 79, 31, 10, 10,  -1, -1,  4,  6,  6,  2, 11, -1,
   10, 39,  4, 10, 31, 35, 10, 18,   2, 13, 15, 20, 68,  2,  3, 16,
    6, 14, -1, -1, 21,  8,  8,  8,   8,  8,  2,  2,  3,  4,  2, -1,
    6, 86,  6, -1, -1,  7, -1,  6,   3, 16,  4,  4,  4,  6, 24, 26,
//#0x0140
   22, 14,  6, 10, 23, 19,  6, 39,   8,  9,  6, 27, -1,  2,  6,  6,
  110,  6, -1, -1, -1, -1, -1,  6,  -1, 54, 66, 54, 90, 42,  6, 42,
   -1, -1, -1, -1, -1, 30, -1,  3,  14,  3, 30, 10, 43, 14,186,182,
   14, 30, 10,  3, -1,  6,106, -1,   4,  5,  4, -1,  6,  7, -1, -1,
//#0x0180
    6,  3,106, 10, 10, 34,  0,  6,   8,  4,  4,  4, 29, -1, 10,  6,
#if PACKETVER < 1
   90, 86, 24,  6, 30,102,  8,  4,   8,  4, 14, 10, -1,  6,  2,  6,
#else	// 196 comodo以? 状態?示アイコン用
   90, 86, 24,  6, 30,102,  9,  4,   8,  4, 14, 10, -1,  6,  2,  6,
#endif
    3,  3, 35,  5, 11, 26, -1,  4,   4,  6, 10, 12,  6, -1,  4,  4,
   11,  7, -1, 67, 12, 18,114,  6,   3,  6, 26, 26, 26, 26,  2,  3,
//#0x01C0
    2, 14, 10, -1, 22, 22,  4,  2,  13, 97,  0,  9,  9, 30,  6, 28,
    8, 14, 10, 35,  6,  8,  4, 11,  54, 53, 60,  2, -1, 47, 33,  6,
    0,  8,  0,  0,  0,  0,  0,  0,  28,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  7,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
};

// local define
enum {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,
	AREA_WOS,
	AREA_WOC,
	AREA_WOSC,
	AREA_CHAT_WOC,
	CHAT,
	CHAT_WOS,
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	SELF };

#define WBUFPOS(p,pos,x,y) { unsigned char *__p = (p); __p+=(pos); __p[0] = (x)>>2; __p[1] = ((x)<<6) | (((y)>>4)&0x3f); __p[2] = (y)<<4; }
#define WBUFPOS2(p,pos,x0,y0,x1,y1) { unsigned char *__p = (p); __p+=(pos); __p[0] = (x0)>>2; __p[1] = ((x0)<<6) | (((y0)>>4)&0x3f); __p[2] = ((y0)<<4) | (((x1)>>6)&0x0f); __p[3]=((x1)<<2) | (((y1)>>8)&0x03); __p[4]=(y1); }

#define WFIFOPOS(fd,pos,x,y) { WBUFPOS (WFIFOP(fd,pos),0,x,y); }
#define WFIFOPOS2(fd,pos,x0,y0,x1,y1) { WBUFPOS2(WFIFOP(fd,pos),0,x0,y0,x1,y1); }

static char map_ip_str[16];
static in_addr_t map_ip;
static int map_port = 5121;

/*==========================================
 * map鯖のip設定
 *------------------------------------------
 */
void clif_setip(char *ip)
{
	memcpy(map_ip_str,ip,16);
	map_ip=inet_addr(map_ip_str);
}

/*==========================================
 * map鯖のport設定
 *------------------------------------------
 */
void clif_setport(int port)
{
	map_port=port;
}

/*==========================================
 * map鯖のip読み出し
 *------------------------------------------
 */
in_addr_t clif_getip(void)
{
	return map_ip;
}

/*==========================================
 * map鯖のport読み出し
 *------------------------------------------
 */
int clif_getport(void)
{
	return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void)
{
	int users=0,i;
	struct map_session_data *sd;

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth)
			users++;
	}
	return users;
}

/*==========================================
 * 全てのclientに対してfunc()実行
 *------------------------------------------
 */
int clif_foreachclient(int (*func)(struct map_session_data*,va_list),...)
{
	int i;
	va_list ap;
	struct map_session_data *sd;

	va_start(ap,func);
	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth)
			func(sd,ap);
	}
	va_end(ap);
	return 0;
}

/*==========================================
 * clif_sendでAREA*指定時用
 *------------------------------------------
 */
int clif_send_sub(struct block_list *bl,va_list ap)
{
	unsigned char *buf;
	int len;
	struct block_list *src_bl;
	int type;
	struct map_session_data *sd;

	buf=va_arg(ap,unsigned char*);
	len=va_arg(ap,int);
	src_bl=va_arg(ap,struct block_list*);
	type=va_arg(ap,int);

	sd=(struct map_session_data *)bl;
	switch(type){
	case AREA_WOS:
		if(bl==src_bl)
			return 0;
		break;
	case AREA_WOC:
		if(sd->chatID || bl==src_bl)
			return 0;
		break;
	case AREA_WOSC:
		if(sd->chatID && sd->chatID == ((struct map_session_data*)src_bl)->chatID)
			return 0;
		break;
	}
	memcpy(WFIFOP(sd->fd,0),buf,len);
	WFIFOSET(sd->fd,len);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send(unsigned char *buf,int len,struct block_list *bl,int type)
{
	int i;
	struct map_session_data *sd;
	struct chat_data *cd;
	struct party *p=NULL;
	struct guild *g=NULL;
	int x0=0,x1=0,y0=0,y1=0;

	switch(type){
	case ALL_CLIENT:	// 全クライアントに送信
		for(i=0;i<fd_max;i++){
			if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
				memcpy(WFIFOP(i,0),buf,len);
				WFIFOSET(i,len);
			}
		}
		break;
	case ALL_SAMEMAP:	// 同じ?ップの全クライアントに送信
		for(i=0;i<fd_max;i++){
			if(session[i] && (sd=session[i]->session_data) && sd->state.auth &&
			   sd->bl.m == bl->m){
				memcpy(WFIFOP(i,0),buf,len);
				WFIFOSET(i,len);
			}
		}
		break;
	case AREA:
	case AREA_WOS:
	case AREA_WOC:
	case AREA_WOSC:
		map_foreachinarea(clif_send_sub,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,BL_PC,buf,len,bl,type);
		break;
	case AREA_CHAT_WOC:
		map_foreachinarea(clif_send_sub,bl->m,bl->x-(AREA_SIZE-5),bl->y-(AREA_SIZE-5),bl->x+(AREA_SIZE-5),bl->y+(AREA_SIZE-5),BL_PC,buf,len,bl,AREA_WOC);
		break;
	case CHAT:
	case CHAT_WOS:
		cd=(struct chat_data*)bl;
		if(bl->type==BL_PC){
			sd=(struct map_session_data*)bl;
			cd=(struct chat_data*)map_id2bl(sd->chatID);
		} else if(bl->type!=BL_CHAT)
			break;
		if(cd==NULL)
			break;
		for(i=0;i<cd->users;i++){
			if(type==CHAT_WOS && cd->usersd[i]==(struct map_session_data*)bl)
				continue;
			memcpy(WFIFOP(cd->usersd[i]->fd,0),buf,len);
			WFIFOSET(cd->usersd[i]->fd,len);
		}
		break;

	case PARTY_AREA:		// 同じ画面内の全パ?ティ?メンバに送信
	case PARTY_AREA_WOS:	// 自分以外の同じ画面内の全パ?ティ?メンバに送信
		x0=bl->x-AREA_SIZE;
		y0=bl->y-AREA_SIZE;
		x1=bl->x+AREA_SIZE;
		y1=bl->y+AREA_SIZE;
	case PARTY:				// 全パ?ティ?メンバに送信
	case PARTY_WOS:			// 自分以外の全パ?ティ?メンバに送信
	case PARTY_SAMEMAP:		// 同じ?ップの全パ?ティ?メンバに送信
	case PARTY_SAMEMAP_WOS:	// 自分以外の同じ?ップの全パ?ティ?メンバに送信
		if(bl->type==BL_PC){
			sd=(struct map_session_data *)bl;
			if(sd->status.party_id>0)
				p=party_search(sd->status.party_id);
		}
		if(p){
			for(i=0;i<MAX_PARTY;i++){
				if((sd=p->member[i].sd)!=NULL){
					if( sd->bl.id==bl->id && (type==PARTY_WOS ||
						type==PARTY_SAMEMAP_WOS || type==PARTY_AREA_WOS))
						continue;
					if(type!=PARTY && type!=PARTY_WOS && bl->m!=sd->bl.m)	// ?ップ?ェック
						continue;
					if((type==PARTY_AREA || type==PARTY_AREA_WOS) &&
						(sd->bl.x<x0 || sd->bl.y<y0 ||
						 sd->bl.x>x1 || sd->bl.y>y1 ) )
						continue;
						
					memcpy(WFIFOP(sd->fd,0),buf,len);
					WFIFOSET(sd->fd,len);
//					if(battle_config.etc_log)
//						printf("send party %d %d %d\n",p->party_id,i,flag);
				}
			}
		}
		break;
	case SELF:
		sd=(struct map_session_data *)bl;
		memcpy(WFIFOP(sd->fd,0),buf,len);
		WFIFOSET(sd->fd,len);
		break;

	case GUILD:
	case GUILD_WOS:
		if(bl->type==BL_PC){
			sd=(struct map_session_data *)bl;
			if(sd->status.guild_id>0)
				g=guild_search(sd->status.guild_id);
		}
		if(g){
			for(i=0;i<g->max_member;i++){
				if((sd=g->member[i].sd)!=NULL){
					if(type==GUILD_WOS && sd->bl.id==bl->id)
						continue;
					memcpy(WFIFOP(sd->fd,0),buf,len);
					WFIFOSET(sd->fd,len);
				}
			}
		}
		break;
		
	default:
		if(battle_config.error_log)
			printf("clif_send まだ作ってないよ?\n");
		return -1;
	}
	return 0;
}

//
// パケット作って送信
//
/*==========================================
 *
 *------------------------------------------
 */
int clif_authok(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x0f;
	WFIFOL(fd,2)=gettick();
	WFIFOPOS(fd,6,sd->bl.x,sd->bl.y);
	WFIFOB(fd,9)=5;
	WFIFOB(fd,10)=5;
	WFIFOSET(fd,packet_len_table[0x0f]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_authfail_fd(int fd,int type)
{
	WFIFOW(fd,0)=0x1D;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x1D]);

	clif_setwaitclose(fd);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok(int id)
{
	struct map_session_data *sd;
	int fd;

	if((sd=map_id2sd(id))==NULL)
		return 1;

	fd=sd->fd;
	WFIFOW(fd,0)=0x4E;
	WFIFOB(fd,2)=1;
	WFIFOSET(fd,packet_len_table[0x4E]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set009e(struct flooritem_data *fitem,unsigned char *buf)
{
	struct item_data *id;
	//0038 <ID>.l <name>.w <X>.w <Y>.w <subX>.B <subY>.B <amount>.B
	id = itemdb_search(fitem->item_data.nameid);
	WBUFW(buf,0)=0x38;
	WBUFL(buf,2)=fitem->bl.id;
	WBUFW(buf,6)=fitem->bl.x;
	WBUFW(buf,8)=fitem->bl.y;
	WBUFB(buf,11)=0x00;	// taulin: fix occasional -16k item amount
	WBUFB(buf,10)=fitem->item_data.amount;
	WBUFB(buf,12)=fitem->subx;
	WBUFB(buf,13)=fitem->suby;
	memcpy(WBUFP(buf,14),id->name,16);
	//WBUFW(buf,6)=id->nameid;

	return packet_len_table[0x38];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem(struct flooritem_data *fitem)
{
	char buf[64];

	if(fitem->item_data.nameid <= 0)
		return 0;
	clif_set009e(fitem,buf);
	clif_send(buf,packet_len_table[0x38],&fitem->bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */

int clif_clearflooritem(struct flooritem_data *fitem,int fd)
{
	unsigned char buf[16];

	WBUFW(buf,0) = 0x3C;
	WBUFL(buf,2) = fitem->bl.id;

	if(fd==0){
		clif_send(buf,packet_len_table[0x3c],&fitem->bl,AREA);
	} else {
		memcpy(WFIFOP(fd,0),buf,6);
		WFIFOSET(fd,packet_len_table[0x3C]);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar(struct block_list *bl,int type)
{
	unsigned char buf[16];

	WBUFW(buf,0) = 0x1c;
	WBUFL(buf,2) = bl->id;
	WBUFB(buf,6) = type;
	clif_send(buf,packet_len_table[0x1c],bl,type==1 ? AREA : AREA_WOS);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar_id(int id,int type,int fd)
{
	unsigned char buf[16];

	WBUFW(buf,0) = 0x1c;
	WBUFL(buf,2) = id;
	WBUFB(buf,6) = type;
	memcpy(WFIFOP(fd,0),buf,7);
	WFIFOSET(fd,packet_len_table[0x1c]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0078(struct map_session_data *sd,unsigned char *buf)
{
	WBUFW(buf,0)=0x14;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFB(buf,8)=sd->view_class;
	WBUFB(buf,9)=sd->sex;
	WBUFPOS(buf,10,sd->bl.x,sd->bl.y);
	// taulin: fix char dir, hair, weapon and sit/stand/dead
	WBUFB(buf,12)|=sd->dir;
	WBUFW(buf,13)=0x00;
	WBUFB(buf,15)=sd->status.hair;
	WBUFB(buf,16)=sd->status.weapon;
	WBUFB(buf,17)=0x00;
	WBUFB(buf,18)=sd->state.dead_sit;
	
	return packet_len_table[0x14];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set007b(struct map_session_data *sd,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x17]);

	WBUFW(buf,0)=0x17;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFB(buf,8)=sd->view_class;
	WBUFB(buf,9)=sd->sex;
	WBUFPOS2(buf,10,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	// taulin: display hair and weapon
	WBUFB(buf,18)=sd->status.hair;
	WBUFB(buf,19)=sd->status.weapon;
	
	return packet_len_table[0x17];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mob_class_change(struct mob_data *md,int class)
{
	char buf[16];

	WBUFW(buf,0)=0x1b0;
	WBUFL(buf,2)=md->bl.id;
	WBUFB(buf,6)=1;
	WBUFL(buf,7)=mob_get_viewclass(class);

	clif_send(buf,packet_len_table[0x1b0],&md->bl,AREA);
	return 0;
}

/*==========================================
 * MOB?示1
 *------------------------------------------
 */
static int clif_mob0078(struct mob_data *md,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x15]);

	WBUFW(buf,0)=0x15;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=battle_get_speed(&md->bl);
	WBUFW(buf,8)=mob_get_viewclass(md->class);
	WBUFPOS(buf,10,md->bl.x,md->bl.y);
	// taulin: fix mob direction facing
	WBUFB(buf,12)|=md->dir;
	WBUFL(buf,14)=0;

	return packet_len_table[0x15];
}

/*==========================================
 * MOB?示2
 *------------------------------------------
 */
static int clif_mob007b(struct mob_data *md,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x17]);

	WBUFW(buf,0)=0x17;
	WBUFL(buf,2)=md->bl.id;
	WBUFL(buf,6)=battle_get_speed(&md->bl);
	WBUFW(buf,8)=mob_get_viewclass(md->class);
	WBUFPOS2(buf,10,md->bl.x,md->bl.y,md->to_x,md->to_y);	
		
	return packet_len_table[0x17];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_npc0078(struct npc_data *nd,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x18]);

	WBUFW(buf,0)=0x18;
	WBUFL(buf,2)=nd->bl.id;
	WBUFB(buf,6)=nd->speed;
	WBUFW(buf,8)=nd->class;
	WBUFPOS(buf,10,nd->bl.x,nd->bl.y);
	// taulin: makes npcs look the right direction
	/* i chose to use 9- here so that the values would coincide with the npceditor
	   i have which has 1 facing north and then goes clockwise to 8, but the client
	   has 0 north and goes counter clockwise to 7.
	   keeping this or reverting to "normal" is up to personal preference
	*/
	WBUFB(buf,12)|=9-nd->dir;
	WBUFL(buf,13)=0;
	WBUFB(buf,18)=0;

	return packet_len_table[0x18];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet0078(struct pet_data *pd,unsigned char *buf)
{
	int view;

	memset(buf,0,packet_len_table[0x18]);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=pd->bl.id;
	WBUFW(buf,6)=pd->speed;
	WBUFW(buf,14)=mob_get_viewclass(pd->class);
	WBUFW(buf,16)=0x14;
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,20)=view;
	else
		WBUFW(buf,20)=pd->equip;
	WBUFPOS(buf,46,pd->bl.x,pd->bl.y);
	WBUFB(buf,48)|=pd->dir&0x0f;
	WBUFB(buf,49)=0;
	WBUFB(buf,50)=0;

	return packet_len_table[0x18];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet007b(struct pet_data *pd,unsigned char *buf)
{
	int view;

	memset(buf,0,packet_len_table[0x7b]);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=pd->bl.id;
	WBUFW(buf,6)=pd->speed;
	WBUFW(buf,14)=mob_get_viewclass(pd->class);
	WBUFW(buf,16)=0x14;
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,20)=view;
	else
		WBUFW(buf,20)=pd->equip;
	WBUFL(buf,22)=gettick();
	WBUFPOS2(buf,50,pd->bl.x,pd->bl.y,pd->to_x,pd->to_y);
	WBUFB(buf,56)=0;
	WBUFB(buf,57)=0;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0192(int fd,int m,int x,int y,int type)
{
	WFIFOW(fd,0) = 0x192;
	WFIFOW(fd,2) = x;
	WFIFOW(fd,4) = y;
	WFIFOW(fd,6) = type;
	memcpy(WFIFOP(fd,8),map[m].name,16);
	WFIFOSET(fd,packet_len_table[0x192]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpc(struct map_session_data *sd)
{
	unsigned char buf[64];
	int len;

	len = clif_set0078(sd,buf);
	clif_send(buf,len,&sd->bl,AREA_WOS);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc(struct npc_data *nd)
{
	unsigned char buf[32];
	int len;

	if(nd->class == INVISIBLE_CLASS)
		return 0;

	len = clif_npc0078(nd,buf);
	clif_send(buf,len,&nd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob(struct mob_data *md)
{
	unsigned char buf[64];
	int len;

	len = clif_mob0078(md,buf);
	clif_send(buf,len,&md->bl,AREA);

	return 0;
}

// pet

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpet(struct pet_data *pd)
{
	unsigned char buf[64];
	int len;

	len = clif_pet0078(pd,buf);
	clif_send(buf,len,&pd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movepet(struct pet_data *pd)
{
	unsigned char buf[256];
	int len;

	len = clif_pet007b(pd,buf);
	clif_send(buf,len,&pd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_servertick(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1B;
	WFIFOL(fd,2)=sd->server_tick;
	WFIFOSET(fd,packet_len_table[0x1B]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x23;
	WFIFOL(fd,2)=gettick();;
	WFIFOPOS2(fd,6,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	WFIFOB(fd,11)=0;
	WFIFOSET(fd,packet_len_table[0x23]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar(struct map_session_data *sd)
{
	int len;
	unsigned char buf[64];
	
	len = clif_set007b(sd,buf);
	clif_send(buf,len,&sd->bl,AREA_WOS);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_quitsave(int fd,struct map_session_data *sd)
{
	map_quit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_waitclose(int tid,unsigned int tick,int id,int data)
{
	if(session[id])
		session[id]->eof=1;
	close(id);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose(int fd)
{
	add_timer(gettick()+5000,clif_waitclose,fd,0);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemap(struct map_session_data *sd,char *mapname,int x,int y)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x2c;
	memcpy(WFIFOP(fd,2),mapname,16);
	WFIFOW(fd,18)=x;
	WFIFOW(fd,20)=y;
	WFIFOSET(fd,packet_len_table[0x2c]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapserver(struct map_session_data *sd,char *mapname,int x,int y,int ip,int port)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x2d;
	memcpy(WFIFOP(fd,2),mapname,16);
	WFIFOW(fd,18)=x;
	WFIFOW(fd,20)=y;
	WFIFOL(fd,22)=ip;
	WFIFOW(fd,26)=port;
	WFIFOSET(fd,packet_len_table[0x2d]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpos(struct block_list *bl)
{
	char buf[16];

	WBUFW(buf,0)=0x24;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=bl->x;
	WBUFW(buf,8)=bl->y;

	clif_send(buf,packet_len_table[0x24],bl,AREA);
	return 0;
}

/*==========================================
 *0x5f
 *------------------------------------------
 */
int clif_npcbuysell(struct map_session_data* sd,int id)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x5f;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_len_table[0x5f]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_buylist(struct map_session_data *sd,struct npc_data *nd)
{
	struct item_data *id;
	int fd=sd->fd,i,val;

	WFIFOW(fd,0)=0x61;
	for(i=0;nd->u.shop_item[i].nameid > 0;i++){
		id = itemdb_search(nd->u.shop_item[i].nameid);
		val=nd->u.shop_item[i].value;

		WFIFOL(fd,6+i*21)=0;
		WFIFOL(fd,4+i*21)=val;
		WFIFOB(fd,8+i*21)=id->type;
		memcpy(WFIFOP(fd,9+i*21),id->name,16);
	}
	WFIFOW(fd,2)=i*21+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_selllist(struct map_session_data *sd)
{
	int fd=sd->fd,i,c=0,val;

	WFIFOW(fd,0)=0x62;
	for(i=0;i<MAX_INVENTORY;i++) {
		if((sd->status.inventory[i].nameid > 0 && sd->inventory_data[i]) && !sd->status.inventory[i].equip) {
			val=sd->inventory_data[i]->value_buy;

			if(val < 0)
				continue;
			WFIFOW(fd,4+c*12)=i+2;
			WFIFOL(fd,8+c*12)=0;
			WFIFOL(fd,10+c*12)=0;
			WFIFOL(fd,12+c*12)=0;
			WFIFOL(fd,14+c*12)=0;
			WFIFOL(fd,6+c*12)=val;
			c++;
		}
	}
	WFIFOW(fd,2)=c*12+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 * added by taulin
 * closes the buy or sell windows
 *------------------------------------------
 */
int clif_closebuysell(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x60;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x60]);
	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmes(struct map_session_data *sd,int npcid,char *mes)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x4f;
	WFIFOW(fd,2)=strlen(mes)+9;
	WFIFOL(fd,4)=npcid;
	strcpy(WFIFOP(fd,8),mes);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptnext(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x50;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x50]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptclose(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x51;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x51]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmenu(struct map_session_data *sd,int npcid,char *mes)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x52;
	WFIFOW(fd,2)=strlen(mes)+8;
	WFIFOL(fd,4)=npcid;
	strcpy(WFIFOP(fd,8),mes);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinput(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x142;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x142]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_viewpoint(struct map_session_data *sd,int npc_id,int type,int x,int y,int id,int color)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x144;
	WFIFOL(fd,2)=npc_id;
	WFIFOL(fd,6)=type;
	WFIFOL(fd,10)=x;
	WFIFOL(fd,14)=y;
	WFIFOB(fd,18)=id;
	WFIFOL(fd,19)=color;
	WFIFOSET(fd,packet_len_table[0x144]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_cutin(struct map_session_data *sd,char *image,int type)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x145;
	memcpy(WFIFOP(fd,2),image,16);
	WFIFOB(fd,18)=type;
	WFIFOSET(fd,packet_len_table[0x145]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem(struct map_session_data *sd,int n,int amount,int fail)
{
	//003B <index>.w <amount>.w <name>.16B <type>.B <equip type>.B <fail>.B
	struct item_data *id;
	int fd=sd->fd;
	//char *itemname;
	unsigned char *buf=WFIFOP(fd,0);

	id = itemdb_search(sd->status.inventory[n].nameid);
	if(n<0 || n>=MAX_INVENTORY || sd->status.inventory[n].nameid <=0 || sd->inventory_data[n] == NULL)
		return 1;

	WBUFW(buf,0)=0x3B;
	WBUFW(buf,2)=n+2;
	WBUFW(buf,4)=amount;
	memcpy(WFIFOP(fd,6),id->name,16);
	WBUFB(buf,22)=sd->inventory_data[n]->type;
	// taulin: makes it so you cant try equip items you shouldnt be able to
	if (pc_isequip(sd,n)){
                WBUFW(buf,23)=pc_equippoint(sd,n);}
        else {
                WBUFW(buf,23)=0;}
	WBUFB(buf,24)=fail;

	WFIFOSET(fd,packet_len_table[0x3B]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_delitem(struct map_session_data *sd,int n,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x4A;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=amount;

	WFIFOSET(fd,packet_len_table[0x4A]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_itemdescription(struct map_session_data *sd,char *p)
{
	struct item_data *id;
	int fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	id = itemdb_searchname(p);
	WBUFW(buf,0)=0x49;
	memcpy(WFIFOP(fd,4),id->jname,16);
	memcpy(WFIFOP(fd,20),id->desc,500);
	WBUFW(buf,2)=4+16+500;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * sends inventory data to client
 * incorrect but works for now
 * must do properly later
 * added by taulin
 *------------------------------------------
 */
int clif_inventitem(struct map_session_data *sd,int n,int amount, int fail)
{
	//003B <index>.w <amount>.w <name>.16B <type>.B <equip type>.B <fail>.B
        struct item_data *id;
        int fd=sd->fd;
        //char *itemname;
        unsigned char *buf=WFIFOP(fd,0);

        id = itemdb_search(sd->status.inventory[n].nameid);
	// || sd->status.inventory[n].equip
        if(n<0 || n>=MAX_INVENTORY || sd->status.inventory[n].nameid <=0 || sd->inventory_data[n] == NULL)
                return 1;

        WBUFW(buf,0)=0x3B;
        WBUFW(buf,2)=n+2;
        WBUFW(buf,4)=amount;
        memcpy(WFIFOP(fd,6),id->name,16);
        WBUFB(buf,22)=sd->inventory_data[n]->type;
	// taulin: makes it so you cant try equip items you shouldnt be able to
	if (pc_isequip(sd,n)){
		WBUFW(buf,23)=pc_equippoint(sd,n);}
	else {
		WBUFW(buf,23)=0;}
        WBUFB(buf,24)=fail;

        WFIFOSET(fd,25);
        return 0;
}


/*==========================================
 * massively modified by taulin
 *------------------------------------------
 */
int clif_itemlist(struct map_session_data *sd)
{
	int i;
//	int i,n,fd=sd->fd;
//	struct item_data *id;
//	unsigned char *buf = WFIFOP(fd,0);

//	WBUFW(buf,0)=0x3F;
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid <=0 || sd->inventory_data[i] == NULL)
			continue;
	        
		clif_inventitem(sd,i,sd->status.inventory[i].amount,0);
/*	        id = itemdb_search(sd->status.inventory[i].nameid);

		WBUFW(buf,n*26+4)=i+2;
	        memcpy(WFIFOP(fd,n*26+6),id->jname,16);
		WBUFW(buf,n*26+24)=sd->status.inventory[i].amount;
		WBUFB(buf,n*26+26)=sd->inventory_data[i]->type;
		WBUFW(buf,n*26+27)=pc_equippoint(sd,n);
		WBUFW(buf,n*26+28)=20;
		n++;
	
	}
	if(n){
		WBUFW(buf,2)=4+n*26;
		WFIFOSET(fd,WFIFOW(fd,2));*/
	}
	return 0;
}


/*==========================================
 * sends equiped data to client
 * wrong but will hopefully work later
 *------------------------------------------
 */
int clif_equipitem(struct map_session_data *sd,int n,int pos,int ok)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x45;
	WFIFOW(fd,2)=n+2;
	WFIFOB(fd,4)=pos;
	WFIFOB(fd,5)=ok;
	WFIFOSET(fd,packet_len_table[0x45]);

	return 0;
}


/*==========================================
 * massively modified by taulin
 *------------------------------------------
 */
int clif_equiplist(struct map_session_data *sd)
{
	int i;
        struct item_data *id;

	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(sd->inventory_data[i]) || sd->status.inventory[i].equip == 0)
			continue;
	
	        id = itemdb_search(sd->status.inventory[i].nameid);
		clif_equipitem(sd,i,pc_equippoint(sd,i),1);		

	}

	return 0;
}

/*==========================================
 * カプラさんに預けてある消耗品&収集品リスト
 *------------------------------------------
 */
int clif_storageitemlist(struct map_session_data *sd,struct storage *stor)
{
	struct item_data *id;
	int i,n,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xa5;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		id = itemdb_search(stor->storage[i].nameid);
		if(itemdb_isequip2(id))
			continue;

		WBUFW(buf,n*10+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*10+6)=id->view_id;
		else
			WBUFW(buf,n*10+6)=stor->storage[i].nameid;
		WBUFB(buf,n*10+8)=id->type;;
		WBUFB(buf,n*10+9)=stor->storage[i].identify;
		WBUFW(buf,n*10+10)=stor->storage[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * カプラさんに預けてある装備リスト
 *------------------------------------------
 */
int clif_storageequiplist(struct map_session_data *sd,struct storage *stor)
{
	struct item_data *id;
	int i,n,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xa6;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		id = itemdb_search(stor->storage[i].nameid);
		if(!itemdb_isequip2(id))
			continue;
		WBUFW(buf,n*20+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*20+6)=id->view_id;
		else
			WBUFW(buf,n*20+6)=stor->storage[i].nameid;
		WBUFB(buf,n*20+8)=id->type;
		WBUFB(buf,n*20+9)=stor->storage[i].identify;
		WBUFW(buf,n*20+10)=id->equip;
		WBUFW(buf,n*20+12)=stor->storage[i].equip;
		WBUFB(buf,n*20+14)=stor->storage[i].attribute;
		WBUFB(buf,n*20+15)=stor->storage[i].refine;
		WBUFW(buf,n*20+16)=stor->storage[i].card[0];
		WBUFW(buf,n*20+18)=stor->storage[i].card[1];
		WBUFW(buf,n*20+20)=stor->storage[i].card[2];
		WBUFW(buf,n*20+22)=stor->storage[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * ステ??スを送りつける
 * ?示専用数字はこの中で計算して送る
 *------------------------------------------
 */
int clif_updatestatus(struct map_session_data *sd,int type)
{
	int fd=sd->fd,len=6;

	WFIFOW(fd,0)=0x4b;
	WFIFOW(fd,2)=type;
	switch(type){
		// 00b0
// taulin: divide weights by 10 to send to client, dont check for 50 and 90% warnings
	case SP_WEIGHT:
		// pc_checkweighticon(sd);
		WFIFOW(fd,0)=0x4b;
		WFIFOW(fd,2)=type;
		WFIFOW(fd,4)=sd->weight/10;
		break;
	case SP_MAXWEIGHT:
		WFIFOW(fd,4)=sd->max_weight/10;
		break;
	case SP_SPEED:
		WFIFOW(fd,4)=sd->speed;
		break;
	case SP_BASELEVEL:
		WFIFOW(fd,4)=sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		WFIFOW(fd,4)=sd->status.job_level;
		break;
	case SP_STATUSPOINT:
		WFIFOW(fd,4)=sd->status.status_point;
		break;
	case SP_SKILLPOINT:
		WFIFOW(fd,4)=sd->status.skill_point;
		break;
	case SP_HIT:
		WFIFOW(fd,4)=sd->hit;
		break;
	case SP_FLEE1:
		WFIFOW(fd,4)=sd->flee;
		break;
	case SP_FLEE2:
		WFIFOW(fd,4)=sd->flee2/10;
		break;
	case SP_MAXHP:
		WFIFOW(fd,4)=sd->status.max_hp;
		break;
	case SP_MAXSP:
		WFIFOW(fd,4)=sd->status.max_sp;
		break;
	case SP_HP:
		WFIFOW(fd,4)=sd->status.hp;
		break;
	case SP_SP:
		WFIFOW(fd,4)=sd->status.sp;
		break;
	case SP_ASPD:
		//len=8;
		//WFIFOW(fd,0)=0x4c;
		//WFIFOW(fd,2)=31;
		//WFIFOW(fd,4)=180;
		WFIFOW(fd,4)=sd->aspd;
		//printf("aspd: %d\n", sd->aspd);
		break;
	case SP_ATK1:
		WFIFOW(fd,4)=sd->base_atk+sd->watk;
		break;
	case SP_DEF1:
		WFIFOW(fd,4)=sd->def;
		break;
	case SP_MDEF1:
		WFIFOW(fd,4)=sd->mdef;
		break;
	case SP_ATK2:
		WFIFOW(fd,4)=sd->watk2;
		break;
	case SP_DEF2:
		WFIFOW(fd,4)=sd->def2;
		break;
	case SP_MDEF2:
		WFIFOW(fd,4)=sd->mdef2;
		break;
	case SP_CRITICAL:
		WFIFOW(fd,4)=sd->critical/10;
		break;
	case SP_MATK1:
		WFIFOW(fd,4)=sd->matk1;
		break;
	case SP_MATK2:
		WFIFOW(fd,4)=sd->matk2;
		break;

		// 004c 
	case SP_STR:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.str;
		break;
	case SP_AGI:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.agi;
		break;
	case SP_VIT:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.vit;
		break;
	case SP_INT:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.int_;
		break;
	case SP_DEX:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.dex;
		break;
	case SP_LUK:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.luk;
		break;
	case SP_ZENY:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.zeny;
		break;
	case SP_BASEEXP:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.base_exp;
		break;
	case SP_JOBEXP:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		len=8;
		WFIFOW(fd,0)=0x4c;
		WFIFOL(fd,4)=pc_nextjobexp(sd);
		break;

		// taulin: up needed status point works now
	case SP_USTR:
	case SP_UAGI:
	case SP_UVIT:
	case SP_UINT:
	case SP_UDEX:
	case SP_ULUK:
		WFIFOW(fd,0)=0x59;
		WFIFOW(fd,2)=type;
		WFIFOB(fd,4)=pc_need_status_point(sd,type-SP_USTR+SP_STR);
		len=5;
		break;

	case SP_ATTACKRANGE:
		//len=5;
		//WFIFOW(fd,0)=0x4c;
		WFIFOW(fd,2)=10;
		WFIFOW(fd,4)=sd->attackrange;
		break;
	case SP_CARTINFO:
		return 0;
		break;
		
	default:
		if(battle_config.error_log)
			printf("clif_updatestatus : make %d routine\n",type);
		return 1;
	}
	WFIFOSET(fd,len);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook(struct block_list *bl,int type,int val)
{
	unsigned char buf[32];
	struct map_session_data *sd = NULL;

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if(sd && (type == LOOK_WEAPON || type == LOOK_SHIELD) && sd->view_class == 22)
		val =0;
	WBUFW(buf,0)=0x5e;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	WBUFB(buf,7)=val;
	clif_send(buf,packet_len_table[0x5e],bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_initialstatus(struct map_session_data *sd)
{
	int fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x58;
	WBUFW(buf,2)=sd->status.status_point;
	WBUFB(buf,4)=(sd->status.str > 255)? 255:sd->status.str;
	WBUFB(buf,5)=pc_need_status_point(sd,SP_STR);
	WBUFB(buf,6)=(sd->status.agi > 255)? 255:sd->status.agi;
	WBUFB(buf,7)=pc_need_status_point(sd,SP_AGI);
	WBUFB(buf,8)=(sd->status.vit > 255)? 255:sd->status.vit;
	WBUFB(buf,9)=pc_need_status_point(sd,SP_VIT);
	WBUFB(buf,10)=(sd->status.int_ > 255)? 255:sd->status.int_;
	WBUFB(buf,11)=pc_need_status_point(sd,SP_INT);
	WBUFB(buf,12)=(sd->status.dex > 255)? 255:sd->status.dex;
	WBUFB(buf,13)=pc_need_status_point(sd,SP_DEX);
	WBUFB(buf,14)=(sd->status.luk > 255)? 255:sd->status.luk;
	WBUFB(buf,15)=pc_need_status_point(sd,SP_LUK);
	WBUFB(buf,16) = sd->base_atk + sd->watk;
	WBUFB(buf,17) = sd->watk2; //atk bonus
	WBUFB(buf,18) = sd->def; // def
	WBUFB(buf,19) = sd->matk1;
	WFIFOSET(fd,packet_len_table[0x58]);
	
	// taulin: not required
	/*clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);*/

	return 0;
}
/*==========================================
 *矢装備
 *------------------------------------------
 */
int clif_arrowequip(struct map_session_data *sd,int val)
{
	int fd=sd->fd;
	
	WFIFOW(fd,0)=0x013c;
	WFIFOW(fd,2)=val+2;//矢のアイテ?ID

	WFIFOSET(fd,packet_len_table[0x013c]);
	
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int clif_arrow_fail(struct map_session_data *sd,int type)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x013b;
	WFIFOW(fd,2)=type;

	WFIFOSET(fd,packet_len_table[0x013b]);

	return 0;
}
/*==========================================
 * 作成可? 矢リスト送信
 *------------------------------------------
 */
int clif_arrow_create_list(struct map_session_data *sd)
{
	int i,c,view;
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1ad;

	for(i=0,c=0;i<MAX_SKILL_ARROW_DB;i++){
		if(skill_arrow_db[i].nameid > 0 && pc_search_inventory(sd,skill_arrow_db[i].nameid)>=0){
			if((view = itemdb_viewid(skill_arrow_db[i].nameid)) > 0)
				WFIFOW(fd,c*2+4) = view;
			else
				WFIFOW(fd,c*2+4) = skill_arrow_db[i].nameid;
			c++;
		}
	}
	WFIFOW(fd,2)=c*2+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack(struct map_session_data *sd,int type,int ok,int val)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x57;
	WFIFOW(fd,2)=type;
	WFIFOB(fd,4)=ok;
	WFIFOB(fd,5)=val;
	WFIFOSET(fd,packet_len_table[0x57]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack(struct map_session_data *sd,int n,int pos,int ok)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x45;
	WFIFOW(fd,2)=n+2;
	WFIFOB(fd,4)=pos;
	WFIFOB(fd,5)=ok;
	WFIFOSET(fd,packet_len_table[0x45]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack(struct map_session_data *sd,int n,int pos,int ok)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x47;
	WFIFOW(fd,2)=n+2;
	WFIFOB(fd,4)=pos;
	WFIFOB(fd,5)=ok;
	WFIFOSET(fd,packet_len_table[0x47]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect(struct block_list* bl,int type)
{
	return 0;
}

/*==========================================
 * ?示オプション変更
 *------------------------------------------
 */
int clif_changeoption(struct block_list* bl)
{
	char buf[32];
	short option = *battle_get_option(bl);
	struct status_change *sc_data = battle_get_sc_data(bl);
	static const int omask[]={ 0x10,0x20 };
	static const int scnum[]={ SC_FALCON, SC_RIDING };
	int i;

	WBUFW(buf,0) = 0x119;
	WBUFL(buf,2) = bl->id;
	WBUFW(buf,6) = *battle_get_opt1(bl);
	WBUFW(buf,8) = *battle_get_opt2(bl);
	WBUFW(buf,10) = option;
	WBUFB(buf,12) = 0;	// ??

	clif_send(buf,packet_len_table[0x119],bl,AREA);
	
	// アイコンの?示
	for(i=0;i<sizeof(omask)/sizeof(omask[0]);i++){
		if( option&omask[i] ){
			if( sc_data[scnum[i]].timer==-1)
				skill_status_change_start(bl,scnum[i],0,0,0,0);
		}else{
			skill_status_change_end(bl,scnum[i],-1);
		}
	}
	
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack(struct map_session_data *sd,int index,int amount,int ok)
{
//	taulin edit
	if (ok) {
		int fd=sd->fd;
		WFIFOW(fd,0)=0x43;
		WFIFOW(fd,2)=index+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,6)=ok;
		WFIFOSET(fd,packet_len_table[0x43]);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_createchat(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x71;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0x71]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dispchat(struct chat_data *cd,int fd)
{
	char buf[128];	// 最大title(60バイト)+17

	if(cd==NULL || *cd->owner==NULL)
		return 1;

	WBUFW(buf,0)=0x72;
	WBUFW(buf,2)=strlen(cd->title)+17;
	WBUFL(buf,4)=(*cd->owner)->id;
	WBUFL(buf,8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strcpy(WBUFP(buf,17),cd->title);
	if(fd){
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WBUFW(buf,2));
	} else {
		clif_send(buf,WBUFW(buf,2),*cd->owner,AREA_WOSC);
	}		
	return 0;
}

/*==========================================
 * chatの状態変更成功
 * 外部の人用と命令コ?ド(7A->82)が違うだけ
 *------------------------------------------
 */
int clif_changechatstatus(struct chat_data *cd)
{
	char buf[128];	// 最大title(60バイト)+17

	if(cd==NULL || cd->usersd[0]==NULL)
		return 1;

	WBUFW(buf,0)=0x7A;
	WBUFW(buf,2)=strlen(cd->title)+17;
	WBUFL(buf,4)=cd->usersd[0]->bl.id;
	WBUFL(buf,8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strcpy(WBUFP(buf,17),cd->title);
	clif_send(buf,WBUFW(buf,2),&cd->usersd[0]->bl,CHAT);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchat(struct chat_data *cd,int fd)
{
	char buf[32];

	WBUFW(buf,0)=0x73;
	WBUFL(buf,2)=cd->bl.id;
	if(fd){
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x73]);
		WFIFOSET(fd,packet_len_table[0x73]);
	} else {
		clif_send(buf,packet_len_table[0x73],&cd->usersd[0]->bl,AREA_WOSC);
	}		
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatfail(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x75;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0x75]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatok(struct map_session_data *sd,struct chat_data* cd)
{
	int fd=sd->fd;
	int i;

	WFIFOW(fd,0)=0x76;
	WFIFOW(fd,2)=8+(20*cd->users);
	WFIFOL(fd,4)=cd->bl.id;
	for(i = 0;i < cd->users;i++){
		WFIFOL(fd,8+i*20) = (i!=0)||((*cd->owner)->type==BL_NPC);
		memcpy(WFIFOP(fd,8+i*20+4),cd->usersd[i]->status.name,16);
	}
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_addchat(struct chat_data* cd,struct map_session_data *sd)
{
	char buf[32];

	WBUFW(buf, 0) = 0x77;
	WBUFW(buf, 2) = cd->users;
	memcpy(WBUFP(buf, 4),sd->status.name,16);
	clif_send(buf,packet_len_table[0x77],&sd->bl,CHAT_WOS);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changechatowner(struct chat_data* cd,struct map_session_data *sd)
{
	char buf[64];

	WBUFW(buf, 0) = 0x7c;
	WBUFL(buf, 2) = 1;
	memcpy(WBUFP(buf,6),cd->usersd[0]->status.name,16);
	WBUFW(buf,22) = 0x7c;
	WBUFL(buf,24) = 0;
	memcpy(WBUFP(buf,28),sd->status.name,16);

	clif_send(buf,packet_len_table[0x7c]*2,&sd->bl,CHAT);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_leavechat(struct chat_data* cd,struct map_session_data *sd)
{
	char buf[32];

	WBUFW(buf, 0) = 0x78;
	WBUFW(buf, 2) = cd->users-1;
	memcpy(WBUFP(buf,4),sd->status.name,16);
	WBUFB(buf,20) = 0;

	clif_send(buf,packet_len_table[0x78],&sd->bl,CHAT);
	return 0;
}

/*==========================================
 * 取り引き要請受け
 *------------------------------------------
 */
int clif_traderequest(struct map_session_data *sd,char *name)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x80;
	strcpy(WFIFOP(fd,2),name);
	WFIFOSET(fd,packet_len_table[0x80]);

	return 0;
}

/*==========================================
 * 取り引き要求応答
 *------------------------------------------
 */
int clif_tradestart(struct map_session_data *sd,int type)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x82;
	WFIFOB(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x82]);

	return 0;
}

/*==========================================
 * 相手方からのアイテ?追加
 *------------------------------------------
 */
int clif_tradeadditem(struct map_session_data *sd,struct map_session_data *tsd,int index,int amount)
{
// taulin: trade add item works, zeny not yet
	int fd=tsd->fd;
	struct item_data *id;

	if (index == 0)
	{
		WFIFOW(fd,0)=0x84;
		WFIFOB(fd,2)=0x00;
		WFIFOB(fd,3)=0x00;
		WFIFOB(fd,4)=0x00;
		WFIFOB(fd,5)=0x00;
		WFIFOL(fd,6)=amount;
		WFIFOSET(fd,packet_len_table[0x84]);
	}
	else 
	{
		id = itemdb_search(sd->status.inventory[index-2].nameid);
		WFIFOW(fd,0)=0x84;
		WFIFOW(fd,4)=0x00;
		WFIFOL(fd,2)=amount;
		memcpy(WFIFOP(fd,6),id->name,16);
		WFIFOSET(fd,packet_len_table[0x84]);
	}

	return 0;
}

/*==========================================
 * アイテ?追加成功/失敗
 *------------------------------------------
 */
int clif_tradeitemok(struct map_session_data *sd,int index,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x85;
	WFIFOW(fd,2)=index;
	WFIFOB(fd,4)=fail;
	WFIFOSET(fd,packet_len_table[0x85]);

	return 0;
}

/*==========================================
 * 取り引きok押し
 *------------------------------------------
 */
int clif_tradedeal_lock(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x87;
	WFIFOB(fd,2)=fail; // 0=you 1=the other person
	WFIFOSET(fd,packet_len_table[0x87]);

	return 0;
}

/*==========================================
 * 取り引きがキャンセルされました
 *------------------------------------------
 */
int clif_tradecancelled(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x89;
	WFIFOSET(fd,packet_len_table[0x89]);

	return 0;
}

/*==========================================
 * 取り引き完了
 *------------------------------------------
 */
int clif_tradecompleted(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x8b;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0x8b]);

	return 0;
}

/*==========================================
 * カプラ倉庫のアイテ?数を更新
 *------------------------------------------
 */
int clif_updatestorageamount(struct map_session_data *sd,struct storage *stor)
{
	int fd=sd->fd;

	WFIFOW(fd,0) = 0xf2;  // update storage amount
	WFIFOW(fd,2) = stor->storage_amount;  //items
	WFIFOW(fd,4) = MAX_STORAGE; //items max
	WFIFOSET(fd,packet_len_table[0xf2]);

	return 0;
}

/*==========================================
 * カプラ倉庫にアイテ?を追加する
 *------------------------------------------
 */
int clif_storageitemadded(struct map_session_data *sd,struct storage *stor,int index,int amount)
{
	int view,fd=sd->fd;

	WFIFOW(fd,0) =0xf4; // Storage item added
	WFIFOW(fd,2) =index+1; // index
	WFIFOL(fd,4) =amount; // amount
	if((view = itemdb_viewid(stor->storage[index].nameid)) > 0)
		WFIFOW(fd,8) =view;
	else
		WFIFOW(fd,8) =stor->storage[index].nameid; // id
	WFIFOB(fd,10)=stor->storage[index].identify; //identify flag
	WFIFOB(fd,11)=stor->storage[index].attribute; // attribute
	WFIFOB(fd,12)=stor->storage[index].refine; //refine
	WFIFOW(fd,13)=stor->storage[index].card[0]; //card (4w)
	WFIFOW(fd,15)=stor->storage[index].card[1]; //card (4w)
	WFIFOW(fd,17)=stor->storage[index].card[2]; //card (4w)
	WFIFOW(fd,19)=stor->storage[index].card[3]; //card (4w)
	WFIFOSET(fd,packet_len_table[0xf4]);

	return 0;
}

/*==========================================
 * カプラ倉庫からアイテ?を取り去る
 *------------------------------------------
 */
int clif_storageitemremoved(struct map_session_data *sd,int index,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xf6; // Storage item removed
	WFIFOW(fd,2)=index+1;
	WFIFOL(fd,4)=amount;
	WFIFOSET(fd,packet_len_table[0xf6]);

	return 0;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
int clif_storageclose(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xf8; // Storage Closed
	WFIFOSET(fd,packet_len_table[0xf8]);

	return 0;
}

//
// callback系 ?
//
/*==========================================
 * PC?示
 *------------------------------------------
 */
void clif_getareachar_pc(struct map_session_data* sd,struct map_session_data* dstsd)
{
	int len;
	if(dstsd->walktimer != -1){
		len = clif_set007b(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_set0078(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}

	if(dstsd->chatID){
		struct chat_data *cd;
		cd=(struct chat_data*)map_id2bl(dstsd->chatID);
		if(cd->usersd[0]==dstsd)
			clif_dispchat(cd,sd->fd);
	}
}

/*==========================================
 * NPC?示
 *------------------------------------------
 */
void clif_getareachar_npc(struct map_session_data* sd,struct npc_data* nd)
{
	int len;

	if(nd->class < 0 || nd->flag&1 || nd->class == INVISIBLE_CLASS)
		return;

	len = clif_npc0078(nd,WFIFOP(sd->fd,0));
	WFIFOSET(sd->fd,len);

	if(nd->chat_id){
		clif_dispchat((struct chat_data*)map_id2bl(nd->chat_id),sd->fd);
	}

}

/*==========================================
 * 移動停?
 *------------------------------------------
 */
int clif_movemob(struct mob_data *md)
{
	unsigned char buf[32];
	int len;

	len = clif_mob007b(md,buf);
	clif_send(buf,len,&md->bl,AREA);

	return 0;
}

/*==========================================
 * モンス??の位置修正
 *------------------------------------------
 */
int clif_fixmobpos(struct mob_data *md)
{
	unsigned char buf[32];
	int len;

	if(md->state.state == MS_WALK){
		len = clif_mob007b(md,buf);
		clif_send(buf,len,&md->bl,AREA);
	} else {
		len = clif_mob0078(md,buf);
		clif_send(buf,len,&md->bl,AREA);
	}

	return 0;
}
/*==========================================
 * PCの位置修正
 *------------------------------------------
 */
int clif_fixpcpos(struct map_session_data *sd)
{
	unsigned char buf[256];
	int len;

	if(sd->walktimer != -1){
		len = clif_set007b(sd,buf);
		clif_send(buf,len,&sd->bl,AREA);
	} else {
		len = clif_set0078(sd,buf);
		clif_send(buf,len,&sd->bl,AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpetpos(struct pet_data *pd)
{
	unsigned char buf[256];
	int len;

	if(pd->state.state == MS_WALK){
		len = clif_pet007b(pd,buf);
		clif_send(buf,len,&pd->bl,AREA);
	} else {
		len = clif_pet0078(pd,buf);
		clif_send(buf,len,&pd->bl,AREA);
	}

	return 0;
}

/*==========================================
 * 通常攻撃エフェクト＆?メ?ジ
 *------------------------------------------
 */
int clif_damage(struct block_list *src,struct block_list *dst,unsigned int tick,int sdelay,int ddelay,int damage,int div,int type,int damage2)
{
	unsigned char buf[256];

	WBUFW(buf,0)=0x26;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=tick;
	WBUFW(buf,14)=damage;
	WBUFB(buf,16)=type;
	clif_send(buf,packet_len_table[0x26],src,AREA);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_mob(struct map_session_data* sd,struct mob_data* md)
{
	int len;
	if(md->state.state == MS_WALK){
		len = clif_mob007b(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_mob0078(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}
}
/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_pet(struct map_session_data* sd,struct pet_data* pd)
{
	int len;
	if(pd->state.state == MS_WALK){
		len = clif_pet007b(pd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_pet0078(pd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_item(struct map_session_data* sd,struct flooritem_data* fitem)
{
	char buf[64];

	if(fitem->item_data.nameid > 0){
		clif_set009e(fitem,buf);
		clif_send(buf,packet_len_table[0x38],&fitem->bl,AREA);
	}

// taulin: same packet as set009e, set009e works better
/*	int view,fd=sd->fd;

	//009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
	WFIFOW(fd,0)=0x38;
	WFIFOL(fd,2)=fitem->bl.id;
	if((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WFIFOW(fd,6)=view;
	else
		WFIFOW(fd,6)=fitem->item_data.nameid;
	WFIFOB(fd,8)=fitem->item_data.identify;
	WFIFOW(fd,9)=fitem->bl.x;
	WFIFOW(fd,11)=fitem->bl.y;
	WFIFOW(fd,13)=fitem->item_data.amount;
	WFIFOB(fd,15)=fitem->subx;
	WFIFOB(fd,16)=fitem->suby;

	WFIFOSET(fd,packet_len_table[0x38]);*/
}
/*==========================================
 * 場所スキルエフェクトが視界に入る
 *------------------------------------------
 */
int clif_getareachar_skillunit(struct map_session_data *sd,struct skill_unit *unit)
{
	int fd=sd->fd;
#if PACKETVER < 3
	memset(WFIFOP(fd,0),0,packet_len_table[0x11f]);
	WFIFOW(fd, 0)=0x11f;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=0;
	WFIFOSET(fd,packet_len_table[0x11f]);
#else
	memset(WFIFOP(fd,0),0,packet_len_table[0x1c9]);
	WFIFOW(fd, 0)=0x1c9;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=0;
	WFIFOSET(fd,packet_len_table[0x1c9]);
#endif
	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,5);

	return 0;
}
/*==========================================
 * 場所スキルエフェクトが視界から消える
 *------------------------------------------
 */
int clif_clearchar_skillunit(struct skill_unit *unit,int fd)
{
	WFIFOW(fd, 0)=0x120;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOSET(fd,packet_len_table[0x120]);
	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,unit->val2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_01ac(struct block_list *bl)
{
	char buf[32];

	WBUFW(buf, 0) = 0x1ac;
	WBUFL(buf, 2) = bl->id;

	clif_send(buf,packet_len_table[0x1ac],bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
 int clif_getareachar(struct block_list* bl,va_list ap)
{
	struct map_session_data *sd;

	sd=va_arg(ap,struct map_session_data*);

	switch(bl->type){
	case BL_PC:
		if(sd==(struct map_session_data*)bl)
			break;
		clif_getareachar_pc(sd,(struct map_session_data*) bl);
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data*) bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data*) bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct pet_data*) bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data*) bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	default:
		if(battle_config.error_log)
			printf("get area char ??? %d\n",bl->type);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd,*dstsd;

	sd=va_arg(ap,struct map_session_data*);
	switch(bl->type){
	case BL_PC:
		dstsd = (struct map_session_data*) bl;
		if(sd != dstsd) {
			clif_clearchar_id(dstsd->bl.id,0,sd->fd);
			clif_clearchar_id(sd->bl.id,0,dstsd->fd);
			if(dstsd->chatID){
				struct chat_data *cd;
				cd=(struct chat_data*)map_id2bl(dstsd->chatID);
				if(cd->usersd[0]==dstsd)
					clif_dispchat(cd,sd->fd);
			}
		}
		break;
	case BL_NPC:
		if( ((struct npc_data *)bl)->class != INVISIBLE_CLASS )
			clif_clearchar_id(bl->id,0,sd->fd);
		break;
	case BL_MOB:
	case BL_PET:
		clif_clearchar_id(bl->id,0,sd->fd);
		break;
	case BL_ITEM:
// removed timer
// taulin
//		clif_clearflooritem((struct flooritem_data*)bl,sd->fd);
		break;
	case BL_SKILL:
		clif_clearchar_skillunit((struct skill_unit *)bl,sd->fd);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd,*dstsd;

	sd=va_arg(ap,struct map_session_data*);
	switch(bl->type){
	case BL_PC:
		dstsd = (struct map_session_data *)bl;
		if(sd != dstsd) {
			clif_getareachar_pc(sd,dstsd);
			clif_getareachar_pc(dstsd,sd);
		}
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data*)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data*)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct pet_data*)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data*)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_moboutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	md=va_arg(ap,struct mob_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data*) bl;
		clif_clearchar_id(md->bl.id,0,sd->fd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mobinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	md=va_arg(ap,struct mob_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data *)bl;
		clif_getareachar_mob(sd,md);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	pd=va_arg(ap,struct pet_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data*) bl;
		clif_clearchar_id(pd->bl.id,0,sd->fd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	pd=va_arg(ap,struct pet_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data *)bl;
		clif_getareachar_pet(sd,pd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillinfo(struct map_session_data *sd,int skillid,int type,int range)
{
	int fd=sd->fd,id;
	if( (id=sd->status.skill[skillid].id) <= 0 )
		return 0;
	WFIFOW(fd,0)=0x147;
	WFIFOW(fd,2) = id;
	if(type < 0)
		WFIFOW(fd,4) = skill_get_inf(id);
	else
		WFIFOW(fd,4) = type;
	WFIFOW(fd,6) = 0;
	WFIFOW(fd,8) = sd->status.skill[skillid].lv;
	WFIFOW(fd,10) = skill_get_sp(id,sd->status.skill[skillid].lv);
	if(range < 0) {
		range = skill_get_range(id,sd->status.skill[skillid].lv);
		if(range < 0)
			range = battle_get_range(&sd->bl) - (range + 1);
		WFIFOW(fd,12)= range;
	}
	else
		WFIFOW(fd,12)= range;
	memset(WFIFOP(fd,14),0,24);
	if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill) ) 
		WFIFOB(fd,38)= (sd->status.skill[skillid].lv < skill_get_max(id) && sd->status.skill[skillid].flag ==0 )? 1:0;
	else
		WFIFOB(fd,38) = 0;
	WFIFOSET(fd,packet_len_table[0x147]);
	return 0;
}

/*==========================================
 * スキルリストを送信する
 *------------------------------------------
 */
int clif_skillinfoblock(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,c,len=4,id,range;
	WFIFOW(fd,0)=0x10f;
	for ( i = c = 0; i < MAX_SKILL; i++){
		if( (id=sd->status.skill[i].id)!=0 ){
			WFIFOW(fd,len  ) = id;
			WFIFOW(fd,len+2) = skill_get_inf(id);
			WFIFOW(fd,len+4) = 0;
			WFIFOW(fd,len+6) = sd->status.skill[i].lv;
			WFIFOW(fd,len+8) = skill_get_sp(id,sd->status.skill[i].lv);
			range = skill_get_range(id,sd->status.skill[i].lv);
			if(range < 0)
				range = battle_get_range(&sd->bl) - (range + 1);
			WFIFOW(fd,len+10)= range;
			memset(WFIFOP(fd,len+12),0,24);
			if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill) )
				WFIFOB(fd,len+36)= (sd->status.skill[i].lv < skill_get_max(id) && sd->status.skill[i].flag ==0 )? 1:0;
			else
				WFIFOB(fd,len+36) = 0;
			len+=37;
			c++;
		}
	}
	WFIFOW(fd,2)=len;
	WFIFOSET(fd,len);
	return 0;
}

/*==========================================
 * スキル割り振り通知
 *------------------------------------------
 */
int clif_skillup(struct map_session_data *sd,int skill_num)
{
	int range,fd=sd->fd;
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = sd->status.skill[skill_num].lv;
	WFIFOW(fd,6) = skill_get_sp(skill_num,sd->status.skill[skill_num].lv);
	range = skill_get_range(skill_num,sd->status.skill[skill_num].lv);
	if(range < 0)
		range = battle_get_range(&sd->bl) - (range + 1);
	WFIFOW(fd,8) = range;
	WFIFOB(fd,10) = (sd->status.skill[skill_num].lv < skill_get_max(sd->status.skill[skill_num].id)) ? 1 : 0;
	WFIFOSET(fd,packet_len_table[0x10e]);
	return 0;
}

/*==========================================
 * スキル詠唱エフェクトを送信する
 *------------------------------------------
 */
int clif_skillcasting(struct block_list* bl,
	int src_id,int dst_id,int dst_x,int dst_y,int skill_num,int casttime)
{
	unsigned char buf[32];
	WBUFW(buf,0) = 0x13e;
	WBUFL(buf,2) = src_id;
	WBUFL(buf,6) = dst_id;
	WBUFW(buf,10) = dst_x;
	WBUFW(buf,12) = dst_y;
	WBUFW(buf,14) = skill_num;//魔?詠唱スキル
	WBUFL(buf,16) = skill_get_pl(skill_num);//属性
	WBUFL(buf,20) = casttime;//skill詠唱時間
	clif_send(buf,packet_len_table[0x13e], bl, AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel(struct block_list* bl)
{
	unsigned char buf[16];
	WBUFW(buf,0) = 0x1b9;
	WBUFL(buf,2) = bl->id;
	clif_send(buf,packet_len_table[0x1b9], bl, AREA);
	return 0;
}

/*==========================================
 * スキル詠唱失敗
 *------------------------------------------
 */
int clif_skill_fail(struct map_session_data *sd,int skill_id,int type,int btype)
{

	int fd=sd->fd;
	WFIFOW(fd,0) = 0x110;
	WFIFOW(fd,2) = skill_id;
	WFIFOW(fd,4) = btype;
	WFIFOW(fd,6) = 0;
	WFIFOB(fd,8) = 0;
	WFIFOB(fd,9) = type;
	WFIFOSET(fd,packet_len_table[0x110]);
	return 0;
}

/*==========================================
 * スキル攻撃エフェクト＆?メ?ジ
 *------------------------------------------
 */
int clif_skill_damage(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,int skill_id,int skill_lv,int type)
{
	unsigned char buf[64];
	struct status_change *sc_data = battle_get_sc_data(dst);

	if(sc_data && sc_data[SC_ENDURE].timer != -1)
		type = 9;

#if PACKETVER < 3
	WBUFW(buf,0)=0x114;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=damage;
	WBUFW(buf,26)=skill_lv;
	WBUFW(buf,28)=div;
	WBUFB(buf,30)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_len_table[0x114],src,AREA);
#else
	WBUFW(buf,0)=0x1de;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFL(buf,24)=damage;
	WBUFW(buf,28)=skill_lv;
	WBUFW(buf,30)=div;
	WBUFB(buf,32)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_len_table[0x1de],src,AREA);
#endif
	return 0;
}
/*==========================================
 * 吹き飛ばしスキル攻撃エフェクト＆?メ?ジ
 *------------------------------------------
 */
int clif_skill_damage2(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,int skill_id,int skill_lv,int type)
{
	unsigned char buf[64];
	struct status_change *sc_data = battle_get_sc_data(dst);

	if(sc_data && sc_data[SC_ENDURE].timer != -1)
		type = 9;

	WBUFW(buf,0)=0x115;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=dst->x;
	WBUFW(buf,26)=dst->y;
	WBUFW(buf,28)=damage;
	WBUFW(buf,30)=skill_lv;
	WBUFW(buf,32)=div;
	WBUFB(buf,34)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_len_table[0x115],src,AREA);

	return 0;
}
/*==========================================
 * 支援/回復スキルエフェクト
 *------------------------------------------
 */
int clif_skill_nodamage(struct block_list *src,struct block_list *dst,
	int skill_id,int heal,int fail)
{
	unsigned char buf[32];

	WBUFW(buf,0)=0x11a;
	WBUFW(buf,2)=skill_id;
	WBUFW(buf,4)=heal;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=src->id;
	WBUFB(buf,14)=fail;
	clif_send(buf,packet_len_table[0x11a],src,AREA);
	return 0;
}
/*==========================================
 * 場所スキルエフェクト
 *------------------------------------------
 */
int clif_skill_poseffect(struct block_list *src,int skill_id,int val,int x,int y,int tick)
{
	unsigned char buf[32];
	WBUFW(buf,0)=0x117;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFW(buf,8)=val;
	WBUFW(buf,10)=x;
	WBUFW(buf,12)=y;
	WBUFL(buf,14)=tick;
	clif_send(buf,packet_len_table[0x117],src,AREA);
	return 0;
}
/*==========================================
 * 場所スキルエフェクト?示
 *------------------------------------------
 */
int clif_skill_setunit(struct skill_unit *unit)
{
	unsigned char buf[128];
#if PACKETVER < 3
	memset(WBUFP(buf, 0),0,packet_len_table[0x11f]);
	WBUFW(buf, 0)=0x11f;
	WBUFL(buf, 2)=unit->bl.id;
	WBUFL(buf, 6)=unit->group->src_id;
	WBUFW(buf,10)=unit->bl.x;
	WBUFW(buf,12)=unit->bl.y;
	WBUFB(buf,14)=unit->group->unit_id;
	WBUFB(buf,15)=0;
	clif_send(buf,packet_len_table[0x11f],&unit->bl,AREA);
#else
	memset(WBUFP(buf, 0),0,packet_len_table[0x1c9]);
	WBUFW(buf, 0)=0x1c9;
	WBUFL(buf, 2)=unit->bl.id;
	WBUFL(buf, 6)=unit->group->src_id;
	WBUFW(buf,10)=unit->bl.x;
	WBUFW(buf,12)=unit->bl.y;
	WBUFB(buf,14)=unit->group->unit_id;
	WBUFB(buf,15)=0;
	clif_send(buf,packet_len_table[0x1c9],&unit->bl,AREA);
#endif
	return 0;
}
/*==========================================
 * 場所スキルエフェクト削除
 *------------------------------------------
 */
int clif_skill_delunit(struct skill_unit *unit)
{
	unsigned char buf[16];
	WBUFW(buf, 0)=0x120;
	WBUFL(buf, 2)=unit->bl.id;
	clif_send(buf,packet_len_table[0x120],&unit->bl,AREA);
	return 0;
}
/*==========================================
 * ワ?プ場所選択
 *------------------------------------------
 */
int clif_skill_warppoint(struct map_session_data *sd,int skill_num,
	const char *map1,const char *map2,const char *map3,const char *map4)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x11c;
	WFIFOW(fd,2)=skill_num;
	memcpy(WFIFOP(fd, 4),map1,16);
	memcpy(WFIFOP(fd,20),map2,16);
	memcpy(WFIFOP(fd,36),map3,16);
	memcpy(WFIFOP(fd,52),map4,16);
	WFIFOSET(fd,packet_len_table[0x11c]);
	return 0;
}
/*==========================================
 * メモ応答
 *------------------------------------------
 */
int clif_skill_memo(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x11e;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x11e]);
	return 0;
}

/*==========================================
 * モンス??情報
 *------------------------------------------
 */
int clif_skill_estimation(struct map_session_data *sd,struct block_list *dst)
{
	struct mob_data *md;
	unsigned char buf[64];
	int i;
	
	if(dst->type!=BL_MOB)
		return 0;
	md=(struct mob_data *)dst;

	WBUFW(buf, 0)=0x18c;
	WBUFW(buf, 2)=mob_get_viewclass(md->class);
	WBUFW(buf, 4)=mob_db[md->class].lv;
	WBUFW(buf, 6)=mob_db[md->class].size;
	WBUFL(buf, 8)=md->hp;
	WBUFW(buf,12)=battle_get_def2(&md->bl);
	WBUFW(buf,14)=mob_db[md->class].race;
	WBUFW(buf,16)=battle_get_mdef2(&md->bl) - (mob_db[md->class].vit>>1);
	WBUFW(buf,18)=battle_get_elem_type(&md->bl);
	for(i=0;i<9;i++)
		WBUFB(buf,20+i)= battle_attr_fix(100,i+1,mob_db[md->class].element);

	if(sd->status.party_id>0)
		clif_send(buf,packet_len_table[0x18c],&sd->bl,PARTY_AREA);
	else{
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x18c]);
		WFIFOSET(sd->fd,packet_len_table[0x18c]);
	}
	return 0;
}
/*==========================================
 * アイテ?合成可?リスト
 *------------------------------------------
 */
int clif_skill_produce_mix_list(struct map_session_data *sd,int trigger)
{
	int i,c,view,fd=sd->fd;
	WFIFOW(fd, 0)=0x18d;
	
	for(i=0,c=0;i<MAX_SKILL_PRODUCE_DB;i++){
		if( skill_can_produce_mix(sd,skill_produce_db[i].nameid,trigger) ){
			if((view = itemdb_viewid(skill_produce_db[i].nameid)) > 0)
				WFIFOW(fd,c*8+ 4)= view;
			else
				WFIFOW(fd,c*8+ 4)= skill_produce_db[i].nameid;
			WFIFOW(fd,c*8+ 6)= 0x0012;
			WFIFOL(fd,c*8+ 8)= sd->status.char_id;
			c++;
		}
	}
	WFIFOW(fd, 2)=c*8+8;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}


/*==========================================
 * 状態異常アイコン/メッセ?ジ?示
 *------------------------------------------
 */
int clif_status_change(struct block_list *bl,int type,int flag)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x0196;
	WBUFW(buf,2)=type;
	WBUFL(buf,4)=bl->id;
	WBUFB(buf,8)=flag;
	clif_send(buf,packet_len_table[0x196],bl,AREA);
	return 0;
}

/*==========================================
 * メッセ?ジ?示
 *------------------------------------------
 */
int clif_displaymessage(int fd,char* mes)
{
	WFIFOW(fd,0) = 0x29;
	WFIFOW(fd,2) = 4+1+strlen(mes);
	memcpy(WFIFOP(fd,4), mes, WFIFOW(fd,2)-4);
	WFIFOSET(fd, WFIFOW(fd,2));

	return 0;
}

/*==========================================
 * 天の声を送信する
 *------------------------------------------
 */
int clif_GMmessage(struct block_list *bl,char* mes,int len,int flag)
{
	unsigned char buf[len+16];
	int lp=(flag&0x10)?8:4;
	WBUFW(buf,0) = 0x35;
	WBUFW(buf,2) = len+lp;
	WBUFL(buf,4) = 0x65756c62;
	memcpy(WBUFP(buf,lp), mes, len);
	flag&=0x07;
	clif_send(buf, WBUFW(buf,2), bl,
		(flag==1)? ALL_SAMEMAP:
		(flag==2)? AREA:
		(flag==3)? SELF:
		ALL_CLIENT);
	return 0;
}


/*==========================================
 * HPSP回復エフェクトを送信する
 *------------------------------------------
 */
int clif_heal(int fd,int type,int val)
{
	WFIFOW(fd,0)=0x13d;
	WFIFOW(fd,2)=type;
	WFIFOW(fd,4)=val;
	WFIFOSET(fd,packet_len_table[0x13d]);

	return 0;
}

/*==========================================
 * 復活する
 *------------------------------------------
 */
int clif_resurrection(struct block_list *bl,int type)
{
	unsigned char buf[16];

	WBUFW(buf,0)=0x148;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=type;
	clif_send(buf,packet_len_table[0x148],bl,type==1 ? AREA : AREA_WOS);

	return 0;
}

/*==========================================
 * PVP実装？（仮）
 *------------------------------------------
 */
int clif_set0199(int fd,int type)
{
	WFIFOW(fd,0)=0x199;
	WFIFOW(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x199]);

	return 0;
}

/*==========================================
 * PVP実装？(仮)
 *------------------------------------------
 */
int clif_pvpset(struct map_session_data *sd,int pvprank,int pvpnum,int type)
{
	if(type == 2) {
		WFIFOW(sd->fd,0) = 0x19a;
		WFIFOL(sd->fd,2) = sd->bl.id;
		WFIFOL(sd->fd,6) = pvprank;
		WFIFOL(sd->fd,10) = pvpnum;
		WFIFOSET(sd->fd,packet_len_table[0x19a]);
	}
	else {
		char buf[32];

		WBUFW(buf,0) = 0x19a;
		WBUFL(buf,2) = sd->bl.id;
		WBUFL(buf,6) = pvprank;
		WBUFL(buf,10) = pvpnum;
		if(!type)
			clif_send(buf,packet_len_table[0x19a],&sd->bl,AREA);
		else
			clif_send(buf,packet_len_table[0x19a],&sd->bl,ALL_SAMEMAP);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send0199(int map,int type)
{
	struct block_list bl;
	char buf[16];

	bl.m = map;
	WBUFW(buf,0)=0x199;
	WBUFW(buf,2)=type;
	clif_send(buf,packet_len_table[0x199],&bl,ALL_SAMEMAP);

	return 0;
}

/*==========================================
 * 精錬エフェクトを送信する
 *------------------------------------------
 */
int clif_refine(int fd,struct map_session_data *sd,int fail,int index,int val)
{
	WFIFOW(fd,0)=0x188;
	WFIFOW(fd,2)=fail;
	WFIFOW(fd,4)=index;
	WFIFOW(fd,6)=val;
	WFIFOSET(fd,packet_len_table[0x188]);

	return 0;
}

/*==========================================
 * Wisを送信する
 *------------------------------------------
 */
int clif_wis_message(int fd,char *nick,char *mes,int mes_len)
{
	WFIFOW(fd,0)=0x32;
	WFIFOW(fd,2)=mes_len +20+ 4;
	memcpy(WFIFOP(fd,4),nick,16);
	memcpy(WFIFOP(fd,20),mes,mes_len);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * Wisの送信結果を送信する
 *------------------------------------------
 */
int clif_wis_end(int fd,int flag)
{
	WFIFOW(fd,0)=0x33;
	WFIFOW(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x33]);
	return 0;
}

/*==========================================
 * キャラID名前引き結果を送信する
 *------------------------------------------
 */
int clif_solved_charname(struct map_session_data *sd,int char_id)
{
	char *p= map_charid2nick(char_id);
	int fd=sd->fd;
	if(p!=NULL){
		WFIFOW(fd,0)=0x194;
		WFIFOL(fd,2)=char_id;
		memcpy(WFIFOP(fd,6), p,24 );
		WFIFOSET(fd,packet_len_table[0x194]);
	}else{
		map_reqchariddb(sd,char_id);
		chrif_searchcharid(char_id);
	}
	return 0;
}

/*==========================================
 * カ?ドの?入可?リストを返す
 *------------------------------------------
 */
int clif_use_card(struct map_session_data *sd,int idx)
{
	if(sd->inventory_data[idx]) {
		int i,c;
		int ep=sd->inventory_data[idx]->equip;
		int fd=sd->fd;
		WFIFOW(fd,0)=0x017b;

		for(i=c=0;i<MAX_INVENTORY;i++){
			int j;

			if(sd->inventory_data[i] == NULL)
				continue;
			if(sd->inventory_data[i]->type!=4 && sd->inventory_data[i]->type!=5)	// 武器防具じゃない
				continue;
			if( sd->status.inventory[i].card[0]==0x00ff)	// 製造武器
				continue;
			if( sd->status.inventory[i].card[0]==(short)0xff00)
				continue;
			if( sd->status.inventory[i].identify==0 )	// 未鑑定
				continue;

			if( (sd->inventory_data[i]->equip&ep)==0)	// 装備個所が違う
				continue;
			if(sd->inventory_data[i]->type==4 && ep==32)	// 盾カ?ドと両手武器
				continue;

			for(j=0;j<sd->inventory_data[i]->slot;j++){
				if( sd->status.inventory[i].card[j]==0 )
					break;
			}
			if(j==sd->inventory_data[i]->slot)	// すでにカ?ドが一杯
				continue;

			WFIFOW(fd,4+c*2)=i+2;
			c++;
		}
		WFIFOW(fd,2)=4+c*2;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}
/*==========================================
 * カ?ドの?入終了
 *------------------------------------------
 */
int clif_insert_card(struct map_session_data *sd,int idx_equip,int idx_card,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x17d;
	WFIFOW(fd,2)=idx_equip+2;
	WFIFOW(fd,4)=idx_card+2;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,packet_len_table[0x17d]);
	return 0;
}

/*==========================================
 * 鑑定可?アイテ?リスト送信
 *------------------------------------------
 */
int clif_item_identify_list(struct map_session_data *sd)
{
	int i,c;
	int fd=sd->fd;

	WFIFOW(fd,0)=0x177;
	for(i=c=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].identify!=1){
			WFIFOW(fd,c*2+4)=i+2;
			c++;
		}
	}
	if(c > 0) {
		WFIFOW(fd,2)=c*2+4;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * 鑑定結果
 *------------------------------------------
 */
int clif_item_identified(struct map_session_data *sd,int idx,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd, 0)=0x179;
	WFIFOW(fd, 2)=idx+2;
	WFIFOB(fd, 4)=flag;
	WFIFOSET(fd,packet_len_table[0x179]);
	return 0;
}

/*==========================================
 * アイテ?による一時的なスキル効果
 *------------------------------------------
 */
int clif_item_skill(struct map_session_data *sd,int skillid,int skilllv,const char *name)
{
	int range,fd=sd->fd;
	WFIFOW(fd, 0)=0x147;
	WFIFOW(fd, 2)=skillid;
	WFIFOW(fd, 4)=skill_get_inf(skillid);
	WFIFOW(fd, 6)=0;
	WFIFOW(fd, 8)=skilllv;
	WFIFOW(fd,10)=skill_get_sp(skillid,skilllv);
	range = skill_get_range(skillid,skilllv);
	if(range < 0)
		range = battle_get_range(&sd->bl) - (range + 1);
	WFIFOW(fd,12)=range;
	memcpy(WFIFOP(fd,14),name,24);
	WFIFOB(fd,38)=0;
	WFIFOSET(fd,packet_len_table[0x147]);
	return 0;
}

/*==========================================
 * カ?トにアイテ?追加
 *------------------------------------------
 */
int clif_cart_additem(struct map_session_data *sd,int n,int amount,int fail)
{
	int view,fd=sd->fd;
	unsigned char *buf=WFIFOP(fd,0);

	if(n<0 || n>=MAX_CART || sd->status.cart[n].nameid<=0)
		return 1;

	WBUFW(buf,0)=0x124;
	WBUFW(buf,2)=n+2;
	WBUFL(buf,4)=amount;
	if((view = itemdb_viewid(sd->status.cart[n].nameid)) > 0)
		WBUFW(buf,8)=view;
	else
		WBUFW(buf,8)=sd->status.cart[n].nameid;
	WBUFB(buf,10)=sd->status.cart[n].identify;
	WBUFB(buf,11)=sd->status.cart[n].attribute;
	WBUFB(buf,12)=sd->status.cart[n].refine;
	WBUFW(buf,13)=sd->status.cart[n].card[0];
	WBUFW(buf,15)=sd->status.cart[n].card[1];
	WBUFW(buf,17)=sd->status.cart[n].card[2];
	WBUFW(buf,19)=sd->status.cart[n].card[3];

	WFIFOSET(fd,packet_len_table[0x124]);
	return 0;
}

/*==========================================
 * カ?トからアイテ?削除
 *------------------------------------------
 */
int clif_cart_delitem(struct map_session_data *sd,int n,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x125;
	WFIFOW(fd,2)=n+2;
	WFIFOL(fd,4)=amount;

	WFIFOSET(fd,packet_len_table[0x125]);

	return 0;
}

/*==========================================
 * カ?トのアイテ?リスト
 *------------------------------------------
 */
int clif_cart_itemlist(struct map_session_data *sd)
{
	struct item_data *id;
	int i,n,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x123;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(itemdb_isequip2(id))
			continue;
		WBUFW(buf,n*10+4)=i+2;
		if(id->view_id > 0)
			WBUFW(buf,n*10+6)=id->view_id;
		else
			WBUFW(buf,n*10+6)=sd->status.cart[i].nameid;
		WBUFB(buf,n*10+8)=id->type;
		WBUFB(buf,n*10+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*10+10)=sd->status.cart[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * カ?トの装備品リスト
 *------------------------------------------
 */
int clif_cart_equiplist(struct map_session_data *sd)
{
	struct item_data *id;
	int i,n,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x122;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(!itemdb_isequip2(id))
			continue;
		WBUFW(buf,n*20+4)=i+2;
		if(id->view_id > 0)
			WBUFW(buf,n*20+6)=id->view_id;
		else
			WBUFW(buf,n*20+6)=sd->status.cart[i].nameid;
		WBUFB(buf,n*20+8)=id->type;
		WBUFB(buf,n*20+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*20+10)=id->equip;
		WBUFW(buf,n*20+12)=sd->status.cart[i].equip;
		WBUFB(buf,n*20+14)=sd->status.cart[i].attribute;
		WBUFB(buf,n*20+15)=sd->status.cart[i].refine;
		WBUFW(buf,n*20+16)=sd->status.cart[i].card[0];
		WBUFW(buf,n*20+18)=sd->status.cart[i].card[1];
		WBUFW(buf,n*20+20)=sd->status.cart[i].card[2];
		WBUFW(buf,n*20+22)=sd->status.cart[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
int clif_openvendingreq(struct map_session_data *sd,int num)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x12d;
	WFIFOW(fd,2)=num;
	WFIFOSET(fd,packet_len_table[0x12d]);

	return 0;
}

/*==========================================
 * 露店看板?示
 *------------------------------------------
 */
int clif_showvendingboard(struct block_list* bl,char *message,int fd)
{
	unsigned char buf[128];

	WBUFW(buf,0)=0x131;
	WBUFL(buf,2)=bl->id;
	strcpy(WBUFP(buf,6),message);
	if(fd){
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x131]);
		WFIFOSET(fd,packet_len_table[0x131]);
	}else{
		clif_send(buf,packet_len_table[0x131],bl,AREA_WOS);
	}
	return 0;
}

/*==========================================
 * 露店看板消去
 *------------------------------------------
 */
int clif_closevendingboard(struct block_list* bl,int fd)
{
	unsigned char buf[16];

	WBUFW(buf,0)=0x132;
	WBUFL(buf,2)=bl->id;
	if(fd){
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x132]);
		WFIFOSET(fd,packet_len_table[0x132]);
	}else{
		clif_send(buf,packet_len_table[0x132],bl,AREA_WOS);
	}
	if(bl->type == BL_PC && ((struct map_session_data *)bl)->npc_id != 0)
		npc_event_dequeue((struct map_session_data *)bl);

	return 0;
}
/*==========================================
 * 露店アイテ?リスト
 *------------------------------------------
 */
int clif_vendinglist(struct map_session_data *sd,int id,struct vending *vending)
{
	struct item_data *data;
	int i,n,index,fd=sd->fd;
	struct map_session_data *vsd=map_id2sd(id);
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x133;
	WBUFL(buf,4)=id;
	for(i=0,n=0;i<vsd->vend_num;i++){
		if(vending[i].amount<=0)
			continue;
		WBUFL(buf,8+n*22)=vending[i].value;
		WBUFW(buf,12+n*22)=vending[i].amount;
		WBUFW(buf,14+n*22)=(index=vending[i].index)+2;
		data = itemdb_search(vsd->status.cart[index].nameid);
		WBUFB(buf,16+n*22)=data->type;
		if(data->view_id > 0)
			WBUFW(buf,17+n*22)=data->view_id;
		else
			WBUFW(buf,17+n*22)=vsd->status.cart[index].nameid;
		WBUFB(buf,19+n*22)=vsd->status.cart[index].identify;
		WBUFB(buf,20+n*22)=vsd->status.cart[index].attribute;
		WBUFB(buf,21+n*22)=vsd->status.cart[index].refine;
		WBUFW(buf,22+n*22)=vsd->status.cart[index].card[0];
		WBUFW(buf,24+n*22)=vsd->status.cart[index].card[1];
		WBUFW(buf,26+n*22)=vsd->status.cart[index].card[2];
		WBUFW(buf,28+n*22)=vsd->status.cart[index].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}

/*==========================================
 * 露店アイテ?購入失敗
 *------------------------------------------
*/
int clif_buyvending(struct map_session_data *sd,int index,int amount,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x135;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOB(fd,6)=fail;
	WFIFOSET(fd,packet_len_table[0x135]);

	return 0;
}

/*==========================================
 * 露店開設成功
 *------------------------------------------
*/
int clif_openvending(struct map_session_data *sd,int id,struct vending *vending)
{
	struct item_data *data;
	int i,n,index,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x136;
	WBUFL(buf,4)=id;
	for(i=0,n=0;i<sd->vend_num;i++){
		WBUFL(buf,8+n*22)=vending[i].value;
		WBUFW(buf,12+n*22)=(index=vending[i].index)+2;
		WBUFW(buf,14+n*22)=vending[i].amount;
		data = itemdb_search(sd->status.cart[index].nameid);
		WBUFB(buf,16+n*22)=data->type;
		if(data->view_id > 0)
			WBUFW(buf,17+n*22)=data->view_id;
		else
			WBUFW(buf,17+n*22)=sd->status.cart[index].nameid;
		WBUFB(buf,19+n*22)=sd->status.cart[index].identify;
		WBUFB(buf,20+n*22)=sd->status.cart[index].attribute;
		WBUFB(buf,21+n*22)=sd->status.cart[index].refine;
		WBUFW(buf,22+n*22)=sd->status.cart[index].card[0];
		WBUFW(buf,24+n*22)=sd->status.cart[index].card[1];
		WBUFW(buf,26+n*22)=sd->status.cart[index].card[2];
		WBUFW(buf,28+n*22)=sd->status.cart[index].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}

/*==========================================
 * 露店アイテ?販売報告
 *------------------------------------------
*/
int clif_vendingreport(struct map_session_data *sd,int index,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x137;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOSET(fd,packet_len_table[0x137]);

	return 0;
}

/*==========================================
 * パ?ティ作成完了
 *------------------------------------------
 */
int clif_party_created(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x95;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x95]);
	return 0;
}
/*==========================================
 * パ?ティ情報送信
 *------------------------------------------
 */
int clif_party_info(struct party *p,int fd)
{
	unsigned char buf[1024];
	int i,c;
	struct map_session_data *sd=NULL;
	
	WBUFW(buf,0)=0x96;
	memcpy(WBUFP(buf,4),p->name,16);
	for(i=c=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if(m->account_id>0){
			if(sd==NULL) sd=m->sd;
			WBUFL(buf,20+c*38)=m->account_id;
			memcpy(WBUFP(buf,20+c*38+ 4),m->name,16);
			memcpy(WBUFP(buf,20+c*38+20),m->map,16);
			WBUFB(buf,20+c*38+36)=(m->leader)?0:1;
			WBUFB(buf,20+c*38+37)=(m->online)?0:1;
			c++;
		}
	}
	WBUFW(buf,2)=20+c*38;
	if(fd>=0){	// fdが設定されてるならそれに送る
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WFIFOW(fd,2));
		return 9;
	}
	if(sd!=NULL)
		clif_send(buf,WBUFW(buf,2),&sd->bl,PARTY);
	return 0;
}
/*==========================================
 * パ?ティ勧誘
 *------------------------------------------
 */
int clif_party_invite(struct map_session_data *sd,struct map_session_data *tsd)
{
	int fd=tsd->fd;
	struct party *p;
	if( (p=party_search(sd->status.party_id))==NULL )
		return 0;

	WFIFOW(fd,0)=0x99;
	WFIFOL(fd,2)=sd->status.account_id;
	memcpy(WFIFOP(fd,6),p->name,16);
	WFIFOSET(fd,packet_len_table[0x99]);
	return 0;
}

/*==========================================
 * パ?ティ勧誘結果
 *------------------------------------------
 */
int clif_party_inviteack(struct map_session_data *sd,char *nick,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x98;
	memcpy(WFIFOP(fd,2),nick,16);
	WFIFOB(fd,18)=flag;
	WFIFOSET(fd,packet_len_table[0x98]);
	return 0;
}

/*==========================================
 *------------------------------------------
 */
int clif_party_option(struct party *p,struct map_session_data *sd,int flag)
{
	return 0;
}
/*==========================================
 * パ?ティ場所移動（未使用）
 *------------------------------------------
 */
int clif_party_move(struct party *p,struct map_session_data *sd,int online)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x9D;
	WBUFL(buf, 2)=sd->status.account_id;
	WBUFL(buf,6)=0;
	WBUFW(buf,10)=sd->bl.x;
	WBUFW(buf,12)=sd->bl.y;
	WBUFB(buf,14)=!online;
	memcpy(WBUFP(buf,15),p->name,16);
	memcpy(WBUFP(buf,31),sd->status.name,16);
	memcpy(WBUFP(buf,47),map[sd->bl.m].name,16);
	clif_send(buf,packet_len_table[0x9D],&sd->bl,PARTY);
	return 0;
}
/*==========================================
 * パ?ティ脱退（脱退前に呼ぶこと）
 *------------------------------------------
 */
int clif_party_leaved(struct party *p,struct map_session_data *sd,int account_id,char *name,int flag)
{
	unsigned char buf[64];
	int i;
	
	WBUFW(buf,0)=0x9E;
	WBUFL(buf,2)=account_id;
	memcpy(WBUFP(buf,6),name,16);
	WBUFB(buf,22)=flag&0x0f;

	if((flag&0xf0)==0){
		if(sd==NULL)
			for(i=0;i<MAX_PARTY;i++)
				if((sd=p->member[i].sd)!=NULL)
					break;
		if(sd!=NULL)
			clif_send(buf,packet_len_table[0x9E],&sd->bl,PARTY);
	}else if(sd!=NULL){
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x9E]);
		WFIFOSET(sd->fd,packet_len_table[0x9E]);
	}
	return 0;
}
/*==========================================
 * パ?ティHP通知
 *------------------------------------------
 */
int clif_party_hp(struct party *p,struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x9F;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=(sd->status.hp > 0x7fff)? 0x7fff:sd->status.hp;
	WBUFW(buf,8)=(sd->status.max_hp > 0x7fff)? 0x7fff:sd->status.max_hp;
	clif_send(buf,packet_len_table[0x9F],&sd->bl,PARTY_AREA_WOS);
//	if(battle_config.etc_log)
//		printf("clif_party_hp %d\n",sd->status.account_id);
	return 0;
}
/*==========================================
 * パ?ティ座標通知
 *------------------------------------------
 */
int clif_party_xy(struct party *p,struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0xA0;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=sd->bl.x;
	WBUFW(buf,8)=sd->bl.y;
	clif_send(buf,packet_len_table[0xA0],&sd->bl,PARTY_SAMEMAP_WOS);
//	if(battle_config.etc_log)
//		printf("clif_party_xy %d\n",sd->status.account_id);
	return 0;
}
/*==========================================
 * パ?ティメッセ?ジ送信
 *------------------------------------------
 */
int clif_party_message(struct party *p,int account_id,char *mes,int len)
{
	struct map_session_data *sd;
	int i;
	for(i=0;i<MAX_PARTY;i++){
		if((sd=p->member[i].sd)!=NULL)
			break;
	}
	if(sd!=NULL){
		unsigned char buf[1024];
		WBUFW(buf,0)=0xa2;
		WBUFW(buf,2)=len+8;
		WBUFL(buf,4)=account_id;
		memcpy(WBUFP(buf,8),mes,len);
		clif_send(buf,len+8,&sd->bl,PARTY);
	}
	return 0;
}

/*==========================================
 * 攻撃するために移動が必要
 *------------------------------------------
 */
int clif_movetoattack(struct map_session_data *sd,struct block_list *bl)
{
	// Removed for now, it's either a different packet or is not used in Alpha [Valaris]
	//int fd=sd->fd;
	//WFIFOW(fd, 0)=0x139;
	//WFIFOL(fd, 2)=bl->id;
	//WFIFOW(fd, 6)=bl->x;
	//WFIFOW(fd, 8)=bl->y;
	//WFIFOW(fd,10)=sd->bl.x;
	//WFIFOW(fd,12)=sd->bl.y;
	//WFIFOW(fd,14)=sd->attackrange;
	//WFIFOSET(fd,packet_len_table[0x139]);
	return 0;
}
/*==========================================
 * 製造エフェクト
 *------------------------------------------
 */
int clif_produceeffect(struct map_session_data *sd,int flag,int nameid)
{
	int view,fd=sd->fd;
	
	// 名前の登?と送信を先にしておく
	if( map_charid2nick(sd->status.char_id)==NULL )
		map_addchariddb(sd->status.char_id,sd->status.name);
	clif_solved_charname(sd,sd->status.char_id);

	WFIFOW(fd, 0)=0x18f;
	WFIFOW(fd, 2)=flag;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd, 4)=view;
	else
		WFIFOW(fd, 4)=nameid;
	WFIFOSET(fd,packet_len_table[0x18f]);
	return 0;
}

// pet
int clif_catch_process(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x19e;
	WFIFOSET(fd,packet_len_table[0x19e]);

	return 0;
}

int clif_pet_rulet(struct map_session_data *sd,int data)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a0;
	WFIFOB(fd,2)=data;
	WFIFOSET(fd,packet_len_table[0x1a0]);

	return 0;
}

/*==========================================
 * pet卵リスト作成
 *------------------------------------------
 */
int clif_sendegg(struct map_session_data *sd)
{
	//R 01a6 <len>.w <index>.w*
	int i,n=0,fd=sd->fd;

	WFIFOW(fd,0)=0x1a6;
	if(sd->status.pet_id <= 0) {
		for(i=0,n=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL ||
			   sd->inventory_data[i]->type!=7 ||
		  	 sd->status.inventory[i].amount<=0)
				continue;
			WFIFOW(fd,n*2+4)=i+2;
			n++;
		}
	}
	WFIFOW(fd,2)=4+n*2;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

int clif_send_petdata(struct map_session_data *sd,int type,int param)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a4;
	WFIFOB(fd,2)=type;
	WFIFOL(fd,3)=sd->pd->bl.id;
	WFIFOL(fd,7)=param;
	WFIFOSET(fd,packet_len_table[0x1a4]);

	return 0;
}

int clif_send_petstatus(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a2;
	memcpy(WFIFOP(fd,2),sd->pet.name,24);
	WFIFOB(fd,26)=(battle_config.pet_rename == 1)? 0:sd->pet.rename_flag;
	WFIFOW(fd,27)=sd->pet.level;
	WFIFOW(fd,29)=sd->pet.hungry;
	WFIFOW(fd,31)=sd->pet.intimate;
	WFIFOW(fd,33)=sd->pet.equip;
	WFIFOSET(fd,packet_len_table[0x1a2]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pet_emotion(struct pet_data *pd,int param)
{
	unsigned char buf[16];
	struct map_session_data *sd = pd->msd;

	memset(buf,0,packet_len_table[0x1aa]);

	WBUFW(buf,0)=0x1aa;
	WBUFL(buf,2)=pd->bl.id;
	if(param >= 100 && sd->petDB->talk_convert_class) {
		if(sd->petDB->talk_convert_class < 0)
			return 0;
		else if(sd->petDB->talk_convert_class > 0) {
			param -= (pd->class - 100)*100;
			param += (sd->petDB->talk_convert_class - 100)*100;
		}
	}
	WBUFL(buf,6)=param;

	clif_send(buf,packet_len_table[0x1aa],&pd->bl,AREA);

	return 0;
}

int clif_pet_performance(struct pet_data *pd,int param)
{
	unsigned char buf[16];

	memset(buf,0,packet_len_table[0x1a4]);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=4;
	WBUFL(buf,3)=pd->bl.id;
	WBUFL(buf,7)=param;

	clif_send(buf,packet_len_table[0x1a4],&pd->bl,AREA);

	return 0;
}

int clif_pet_equip(struct pet_data *pd,int nameid)
{
	unsigned char buf[16];
	int view;

	memset(buf,0,packet_len_table[0x1a4]);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=3;
	WBUFL(buf,3)=pd->bl.id;
	if((view = itemdb_viewid(nameid)) > 0)
		WBUFL(buf,7)=view;
	else
		WBUFL(buf,7)=nameid;

	clif_send(buf,packet_len_table[0x1a4],&pd->bl,AREA);

	return 0;
}

int clif_pet_food(struct map_session_data *sd,int foodid,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a3;
	WFIFOB(fd,2)=fail;
	WFIFOW(fd,3)=foodid;
	WFIFOSET(fd,packet_len_table[0x1a3]);

	return 0;
}
/*==========================================
 * ディ??ションの青い糸
 *------------------------------------------
 */
int clif_devotion(struct map_session_data *sd,int target)
{
	unsigned char buf[56];
	int n;
	WBUFW(buf,0)=0x1cf;
	WBUFL(buf,2)=sd->bl.id;
//	WBUFL(buf,6)=target;
	for(n=0;n<5;n++)
		WBUFL(buf,6+4*n)=sd->dev.val2[n];
//		WBUFL(buf,10+4*n)=0;
	WBUFB(buf,26)=8;
	WBUFB(buf,27)=0;

	clif_send(buf,packet_len_table[0x1cf],&sd->bl,AREA);
	return 0;
}

/*==========================================
 * 氣球 
 *------------------------------------------
 */
int clif_spiritball(struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x1d0;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->spiritball;
	clif_send(buf,packet_len_table[0x1d0],&sd->bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_combo_delay(struct block_list *bl,int wait)
{
	unsigned char buf[32];

	WBUFW(buf,0)=0x1d2;
	WBUFL(buf,2)=bl->id;
	WBUFL(buf,6)=wait;
	clif_send(buf,packet_len_table[0x1d2],bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapcell(int m,int x,int y,int cell_type,int type)
{
	struct block_list bl;
	char buf[32];

	bl.m = m;
	bl.x = x;
	bl.y = y;
	WBUFW(buf,0) = 0x192;
	WBUFW(buf,2) = x;
	WBUFW(buf,4) = y;
	WBUFW(buf,6) = cell_type;
	memcpy(WBUFP(buf,8),map[m].name,16);
	if(!type)
		clif_send(buf,packet_len_table[0x192],&bl,AREA);
	else
		clif_send(buf,packet_len_table[0x192],&bl,ALL_SAMEMAP);

	return 0;
}

/*==========================================
 * MVPエフェクト
 *------------------------------------------
 */
int clif_mvp_effect(struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x10c;
	WBUFL(buf,2)=sd->bl.id;
	clif_send(buf,packet_len_table[0x10c],&sd->bl,AREA);
	return 0;
}
/*==========================================
 * MVPアイテ?所得
 *------------------------------------------
 */
int clif_mvp_item(struct map_session_data *sd,int nameid)
{
	int view,fd=sd->fd;
	WFIFOW(fd,0)=0x10a;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd,2)=view;
	else
		WFIFOW(fd,2)=nameid;
	WFIFOSET(fd,packet_len_table[0x10a]);
	return 0;
}
/*==========================================
 * MVP経験値所得
 *------------------------------------------
 */
int clif_mvp_exp(struct map_session_data *sd,int exp)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x10b;
	WFIFOL(fd,2)=exp;
	WFIFOSET(fd,packet_len_table[0x10b]);
	return 0;
}

/*==========================================
 * ギルド作成可否通知
 *------------------------------------------
 */
int clif_guild_created(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x167;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x167]);
	return 0;
}
/*==========================================
 * ギルド所属通知
 *------------------------------------------
 */
int clif_guild_belonginfo(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	int ps=guild_getposition(sd,g);

	memset(WFIFOP(fd,0),0,packet_len_table[0x16c]);
	WFIFOW(fd,0)=0x16c;
	WFIFOL(fd,2)=g->guild_id;
	WFIFOL(fd,6)=g->emblem_id;
	WFIFOL(fd,10)=g->position[ps].mode;
	memcpy(WFIFOP(fd,19),g->name,24);
	WFIFOSET(fd,packet_len_table[0x16c]);
	return 0;
}
/*==========================================
 * ギルドメンバログイン通知
 *------------------------------------------
 */
int clif_guild_memberlogin_notice(struct guild *g,int idx,int flag)
{
	unsigned char buf[64];
	WBUFW(buf, 0)=0x16d;
	WBUFL(buf, 2)=g->member[idx].account_id;
	WBUFL(buf, 6)=g->member[idx].char_id;
	WBUFL(buf,10)=flag;
	if(g->member[idx].sd==NULL){
		struct map_session_data *sd=guild_getavailablesd(g);
		if(sd!=NULL)
			clif_send(buf,packet_len_table[0x16d],&sd->bl,GUILD);
	}else
		clif_send(buf,packet_len_table[0x16d],&g->member[idx].sd->bl,GUILD_WOS);
	return 0;
}
/*==========================================
 * ギルド?ス??通知(14dへの応答)
 *------------------------------------------
 */
int clif_guild_masterormember(struct map_session_data *sd)
{
	int type=0x57,fd=sd->fd;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g!=NULL && strcmp(g->master,sd->status.name)==0)
		type=0xd7;
	WFIFOW(fd,0)=0x14e;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x14e]);
	return 0;
}
/*==========================================
 * ギルド基?情報
 *------------------------------------------
 */
int clif_guild_basicinfo(struct map_session_data *sd)
{
	int fd=sd->fd;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	
	WFIFOW(fd, 0)=0x1b6;//0x150;
	WFIFOL(fd, 2)=g->guild_id;
	WFIFOL(fd, 6)=g->guild_lv;
	WFIFOL(fd,10)=g->connect_member;
	WFIFOL(fd,14)=g->max_member;
	WFIFOL(fd,18)=g->average_lv;
	WFIFOL(fd,22)=g->exp;
	WFIFOL(fd,26)=g->next_exp;
	WFIFOL(fd,30)=0;	// 上?
	WFIFOL(fd,34)=0;	// VW（性格の悪さ？：性向グラフ左右）
	WFIFOL(fd,38)=0;	// RF（正?の度合い？：性向グラフ上下）
	WFIFOL(fd,42)=0;	// 人数？
	memcpy(WFIFOP(fd,46),g->name,24);
	memcpy(WFIFOP(fd,70),g->master,24);
	memcpy(WFIFOP(fd,94),"",20);	// ?拠地
	WFIFOSET(fd,packet_len_table[WFIFOW(fd,0)]);
	return 0;
}
/*==========================================
 * ギルド同盟/敵対情報
 *------------------------------------------
 */
int clif_guild_allianceinfo(struct map_session_data *sd)
{
	int fd=sd->fd,i,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x14c;
	for(i=c=0;i<MAX_GUILDALLIANCE;i++){
		struct guild_alliance *a=&g->alliance[i];
		if(a->guild_id>0){
			WFIFOL(fd,c*32+4)=a->opposition;
			WFIFOL(fd,c*32+8)=a->guild_id;
			memcpy(WFIFOP(fd,c*32+12),a->name,24);
			c++;
		}
	}
	WFIFOW(fd, 2)=c*32+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * ギルドメンバ?リスト
 *------------------------------------------
 */
int clif_guild_memberlist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;

	WFIFOW(fd, 0)=0x154;
	for(i=0,c=0;i<g->max_member;i++){
		struct guild_member *m=&g->member[i];
		if(m->account_id==0)
			continue;
		WFIFOL(fd,c*104+ 4)=m->account_id;
		WFIFOL(fd,c*104+ 8)=m->char_id;
		WFIFOW(fd,c*104+12)=m->hair;
		WFIFOW(fd,c*104+14)=m->hair_color;
		WFIFOW(fd,c*104+16)=m->gender;
		WFIFOW(fd,c*104+18)=m->class;
		WFIFOW(fd,c*104+20)=m->lv;
		WFIFOL(fd,c*104+22)=m->exp;
		WFIFOL(fd,c*104+26)=m->online;
		WFIFOL(fd,c*104+30)=m->position;
		memset(WFIFOP(fd,c*104+34),0,50);	// メモ？
		memcpy(WFIFOP(fd,c*104+84),m->name,24);
		c++;
	}
	WFIFOW(fd, 2)=c*104+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド役職名リスト
 *------------------------------------------
 */
int clif_guild_positionnamelist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x166;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		WFIFOL(fd,i*28+4)=i;
		memcpy(WFIFOP(fd,i*28+8),g->position[i].name,24);
	}
	WFIFOW(fd,2)=i*28+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド役職情報リスト
 *------------------------------------------
 */
int clif_guild_positioninfolist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x160;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p=&g->position[i];
		WFIFOL(fd,i*16+ 4)=i;
		WFIFOL(fd,i*16+ 8)=p->mode;
		WFIFOL(fd,i*16+12)=i;
		WFIFOL(fd,i*16+16)=p->exp_mode;
	}
	WFIFOW(fd, 2)=i*16+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド役職変更通知
 *------------------------------------------
 */
int clif_guild_positionchanged(struct guild *g,int idx)
{
	struct map_session_data *sd;
	unsigned char buf[128];
	WBUFW(buf, 0)=0x174;
	WBUFW(buf, 2)=44;
	WBUFL(buf, 4)=idx;
	WBUFL(buf, 8)=g->position[idx].mode;
	WBUFL(buf,12)=idx;
	WBUFL(buf,16)=g->position[idx].exp_mode;
	memcpy(WBUFP(buf,20),g->position[idx].name,24);
	if( (sd=guild_getavailablesd(g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドメンバ変更通知
 *------------------------------------------
 */
int clif_guild_memberpositionchanged(struct guild *g,int idx)
{
	struct map_session_data *sd;
	unsigned char buf[64];
	WBUFW(buf, 0)=0x156;
	WBUFW(buf, 2)=16;
	WBUFL(buf, 4)=g->member[idx].account_id;
	WBUFL(buf, 8)=g->member[idx].char_id;
	WBUFL(buf,12)=g->member[idx].position;
	if( (sd=guild_getavailablesd(g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドエンブレ?送信
 *------------------------------------------
 */
int clif_guild_emblem(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	if(g->emblem_len<=0)
		return 0;
	WFIFOW(fd,0)=0x152;
	WFIFOW(fd,2)=g->emblem_len+12;
	WFIFOL(fd,4)=g->guild_id;
	WFIFOL(fd,8)=g->emblem_id;
	memcpy(WFIFOP(fd,12),g->emblem_data,g->emblem_len);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルドスキル送信
 *------------------------------------------
 */
int clif_guild_skillinfo(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,id,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd,0)=0x0162;
	WFIFOW(fd,4)=g->skill_point;
	for(i=c=0;i<MAX_GUILDSKILL;i++){
		if(g->skill[i].id>0){
			WFIFOW(fd,c*37+ 6) = id = g->skill[i].id;
			WFIFOW(fd,c*37+ 8) = guild_skill_get_inf(id);
			WFIFOW(fd,c*37+10) = 0;
			WFIFOW(fd,c*37+12) = g->skill[i].lv;
			WFIFOW(fd,c*37+14) = guild_skill_get_sp(id,g->skill[i].lv);
			WFIFOW(fd,c*37+16) = guild_skill_get_range(id);
			memset(WFIFOP(fd,c*37+18),0,24);
			WFIFOB(fd,c*37+42)= //up;
				(g->skill[i].lv < guild_skill_get_max(id))? 1:0;
			c++;
		}
	}
	WFIFOW(fd,2)=c*37+6;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド告知送信
 *------------------------------------------
 */
int clif_guild_notice(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	if(*g->mes1==0 && *g->mes2==0)
		return 0;
	WFIFOW(fd,0)=0x16f;
	memcpy(WFIFOP(fd,2),g->mes1,60);
	memcpy(WFIFOP(fd,62),g->mes2,120);
	WFIFOSET(fd,packet_len_table[0x16f]);
	return 0;
}


/*==========================================
 * ギルドメンバ勧誘
 *------------------------------------------
 */
int clif_guild_invite(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x16a;
	WFIFOL(fd,2)=g->guild_id;
	memcpy(WFIFOP(fd,6),g->name,24);
	WFIFOSET(fd,packet_len_table[0x16a]);
	return 0;
}
/*==========================================
 * ギルドメンバ勧誘結果
 *------------------------------------------
 */
int clif_guild_inviteack(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x169;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x169]);
	return 0;
}
/*==========================================
 * ギルドメンバ脱退通知
 *------------------------------------------
 */
int clif_guild_leave(struct map_session_data *sd,const char *name,const char *mes)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x15a;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	clif_send(buf,packet_len_table[0x15a],&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドメンバ追放通知
 *------------------------------------------
 */
int clif_guild_explusion(struct map_session_data *sd,const char *name,const char *mes,
	int account_id)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x15c;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	memcpy(WBUFP(buf,66),"dummy",24);
	clif_send(buf,packet_len_table[0x15c],&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルド追放メンバリスト
 *------------------------------------------
 */
int clif_guild_explusionlist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd,0)=0x163;
	for(i=c=0;i<MAX_GUILDEXPLUSION;i++){
		struct guild_explusion *e=&g->explusion[i];
		if(e->account_id>0){
			memcpy(WFIFOP(fd,c*88+ 4),e->name,24);
			memcpy(WFIFOP(fd,c*88+28),e->acc,24);
			memcpy(WFIFOP(fd,c*88+52),e->mes,44);
			c++;
		}
	}
	WFIFOW(fd,2)=c*88+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * ギルド会話
 *------------------------------------------
 */
int clif_guild_message(struct guild *g,int account_id,const char *mes,int len)
{
	struct map_session_data *sd;
	unsigned char buf[len+32];
	WBUFW(buf, 0)=0x17f;
	WBUFW(buf, 2)=len+4;
	memcpy(WBUFP(buf,4),mes,len);
	
	if( (sd=guild_getavailablesd(g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドスキル割り振り通知
 *------------------------------------------
 */
int clif_guild_skillup(struct map_session_data *sd,int skill_num,int lv)
{
	int fd=sd->fd;
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = lv;
	WFIFOW(fd,6) = guild_skill_get_sp(skill_num,lv);
	WFIFOW(fd,8) = guild_skill_get_range(skill_num);
	WFIFOB(fd,10) = 1;
	WFIFOSET(fd,11);
	return 0;
}
/*==========================================
 * ギルド同盟要請
 *------------------------------------------
 */
int clif_guild_reqalliance(struct map_session_data *sd,int account_id,const char *name)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x171;
	WFIFOL(fd,2)=account_id;
	memcpy(WFIFOP(fd,6),name,24);
	WFIFOSET(fd,packet_len_table[0x171]);
	return 0;
}
/*==========================================
 * ギルド同盟結果
 *------------------------------------------
 */
int clif_guild_allianceack(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x173;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x173]);
	return 0;
}
/*==========================================
 * ギルド関係解消通知
 *------------------------------------------
 */
int clif_guild_delalliance(struct map_session_data *sd,int guild_id,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x184;
	WFIFOL(fd,2)=guild_id;
	WFIFOL(fd,6)=flag;
	WFIFOSET(fd,packet_len_table[0x184]);
	return 0;
}
/*==========================================
 * ギルド敵対結果
 *------------------------------------------
 */
int clif_guild_oppositionack(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x181;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x181]);
	return 0;
}
/*==========================================
 * ギルド関係追加
 *------------------------------------------
 */
/*int clif_guild_allianceadded(struct guild *g,int idx)
{
	unsigned char buf[64];
	WBUFW(fd,0)=0x185;
	WBUFL(fd,2)=g->alliance[idx].opposition;
	WBUFL(fd,6)=g->alliance[idx].guild_id;
	memcpy(WBUFP(fd,10),g->alliance[idx].name,24);
	clif_send(buf,packet_len_table[0x185],guild_getavailablesd(g),GUILD);
	return 0;
}*/
/*==========================================
 * ギルド解散通知
 *------------------------------------------
 */
int clif_guild_broken(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x15e;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x15e]);
	return 0;
}

/*==========================================
 * エモ?ション
 *------------------------------------------
 */
void clif_emotion(struct block_list *bl,int type)
{
	unsigned char buf[8];
	WBUFW(buf,0)=0x5b;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	clif_send(buf,packet_len_table[0x5b],bl,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */

int clif_GM_kickack(struct map_session_data *sd,int id)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x68;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_len_table[0x68]);
	return 0;
}

void clif_parse_QuitGame(int fd,struct map_session_data *sd);

int clif_GM_kick(struct map_session_data *sd,struct map_session_data *tsd,int type)
{
	if(type)
		clif_GM_kickack(sd,tsd->status.account_id);
	tsd->opt1 = tsd->opt2 = 0;
	clif_parse_QuitGame(tsd->fd,tsd);

	return 0;
}

// ------------
// clif_parse_*
// ------------
// パケット読み取って色々?作
/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WantToConnection(int fd,struct map_session_data *sd)
{
	struct map_session_data *old_sd;

	if(sd){
		if(battle_config.error_log)
			printf("clif_parse_WantToConnection : invalid request?\n");
		return;
	}

	sd=session[fd]->session_data=malloc(sizeof(*sd));
	if(sd==NULL){
		printf("out of memory : clif_parse_WantToConnection\n");
		exit(1);
	}
	memset(sd,0,sizeof(*sd));
	sd->fd = fd;

	pc_setnewpc(sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),0,RFIFOB(fd,14),fd);

	if((old_sd=map_id2sd(RFIFOL(fd,2))) != NULL){
		// 2重loginなので切断用のデ??を保存する
		old_sd->new_fd=fd;
	} else {
		map_addiddb(&sd->bl);
	}

	chrif_authreq(sd);
}

/*==========================================
 * 0019
 * Map load success
 *------------------------------------------
 */
void clif_parse_LoadEndAck(int fd,struct map_session_data *sd)
{
	if(sd->bl.prev != NULL)
		return;

	//pc_motd(sd->bl.id);

	pc_checkitem(sd);

	// next exp
	clif_updatestatus(sd,SP_NEXTBASEEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);
	
	//skill point
	//clif_updatestatus(sd,SP_SKILLPOINT);

	//item			// taulin
	clif_itemlist(sd);	// item and equip display probelm
	clif_equiplist(sd);

	// param all
	clif_initialstatus(sd);

	//party
	party_send_movemap(sd);

	map_addblock(&sd->bl);	// ブロック登?
	clif_spawnpc(sd);	// spawn

	//weight max , now
	clif_updatestatus(sd,SP_MAXWEIGHT);
	clif_updatestatus(sd,SP_WEIGHT);
	//clif_updatestatus(sd,SP_ASPD);
	//clif_updatestatus(sd,SP_ATTACKRANGE);

	// view equipment item
	//clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);

	// option
	//clif_changeoption(&sd->bl);
	map_foreachinarea(clif_getareachar,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,0,sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TickSend(int fd,struct map_session_data *sd)
{
	sd->client_tick=RFIFOL(fd,2);
	sd->server_tick=gettick();
	clif_servertick(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WalkToXY(int fd,struct map_session_data *sd)
{
	int x,y;

	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	if(sd->npc_id != 0 || sd->vender_id != 0) return;

	if(sd->skilltimer != -1 && pc_checkskill(sd,SA_FREECAST) <= 0) // フリ?キャスト
		return;

	if(sd->chatID)
		return;

	if(sd->canmove_tick > gettick())
		return;

	// ステ??ス異常やハイディング中(トンネルドライブ無)で動けない
	if((sd->opt1 > 0 && sd->opt1 != 6) ||
		sd->sc_data[SC_ANKLE].timer !=-1 || sd->sc_data[SC_AUTOCOUNTER].timer!=-1 || sd->sc_data[SC_TRICKDEAD].timer!=-1)
		return;
	if( (sd->status.option&2) && pc_checkskill(sd,RG_TUNNELDRIVE) <= 0)
		return;

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	pc_stopattack(sd);

	x = RFIFOB(fd,2)*4+(RFIFOB(fd,3)>>6);
	y = ((RFIFOB(fd,3)&0x3f)<<4)+(RFIFOB(fd,4)>>4);

	pc_walktoxy(sd,x,y);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_QuitGame(int fd,struct map_session_data *sd)
{
	if(sd->opt1 || sd->opt2)
		return;
	clif_setwaitclose(fd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GetCharNameRequest(int fd,struct map_session_data *sd)
{
	struct block_list *bl;
	char accid[10];
	int i;

	bl=map_id2bl(RFIFOL(fd,2));
	if(bl==NULL)
		return;

	WFIFOW(fd,0)=0x30;
// taulin: fixes junk data after monster/npc/player(?) names
	WFIFOW(fd,6)=0x00;
	WFIFOL(fd,2)=RFIFOL(fd,2);

	switch(bl->type){
	case BL_PC:
		// taulin: account id display after name
		i = sprintf(accid, "%d", RFIFOL(fd,2));
		memcpy(WFIFOP(fd,6),accid,10);		
		memcpy(WFIFOP(fd,22),((struct map_session_data*)bl)->status.name,16);
		WFIFOSET(fd,packet_len_table[0x30]);
		break;
	case BL_PET:	// taulin: remove this at some point
		memcpy(WFIFOP(fd,22),((struct pet_data*)bl)->name,16);
		WFIFOSET(fd,packet_len_table[0x30]);
		break;
	case BL_NPC:
		memcpy(WFIFOP(fd,22),((struct npc_data*)bl)->name,16);
		WFIFOSET(fd,packet_len_table[0x30]);
		break;
	case BL_MOB:
		memcpy(WFIFOP(fd,22),((struct mob_data*)bl)->name,16);
		WFIFOSET(fd,packet_len_table[0x30]);
		break;
	default:
		if(battle_config.error_log)
			printf("clif_parse_GetCharNameRequest : bad type %d(%d)\n",bl->type,RFIFOL(fd,2));
		break;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GlobalMessage(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,4))) return;
	WFIFOW(fd,0)=0x28;
	WFIFOW(fd,2)=RFIFOW(fd,2)+4;
	WFIFOL(fd,4)=sd->bl.id;
	memcpy(WFIFOP(fd,8),RFIFOP(fd,4),RFIFOW(fd,2)-4);
	clif_send(WFIFOP(fd,0),WFIFOW(fd,2),&sd->bl,sd->chatID ? CHAT_WOS : AREA_CHAT_WOC);

	memcpy(WFIFOP(fd,0),RFIFOP(fd,0),RFIFOW(fd,2));
	WFIFOW(fd,0)=0x29;
	WFIFOSET(fd,WFIFOW(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_MapMove(int fd,struct map_session_data *sd)
{
	char mapname[32];

	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.mapmove) {
		memcpy(mapname,RFIFOP(fd,2),16);
		mapname[16]=0;
		pc_setpos(sd,mapname,RFIFOW(fd,18),RFIFOW(fd,20),2);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeDir(int fd,struct map_session_data *sd)
{
	// taulin: fixed remaining facing direction problems
	pc_setdir(sd,RFIFOB(fd,2),0);

	WFIFOW(fd,0)=0x37;
	WFIFOL(fd,2)=sd->bl.id;
	WFIFOB(fd,6)=RFIFOB(fd,2);
	clif_send(WFIFOP(fd,0),packet_len_table[0x37],&sd->bl,AREA_WOS);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Emotion(int fd,struct map_session_data *sd)
{
	WFIFOW(fd,0)=0x5b;
	WFIFOL(fd,2)=sd->bl.id;
	WFIFOB(fd,6)=RFIFOB(fd,2);
	clif_send(WFIFOP(fd,0),packet_len_table[0x5b],&sd->bl,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_HowManyConnections(int fd,struct map_session_data *sd)
{
	WFIFOW(fd,0)=0x5d;
	WFIFOL(fd,2)=map_getusers();
	WFIFOSET(fd,packet_len_table[0x5d]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ActionRequest(int fd,struct map_session_data *sd)
{
	unsigned int tick;
	unsigned char buf[64];

	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0 || sd->opt1 > 0 || sd->sc_data[SC_AUTOCOUNTER].timer != -1) return;

	tick=gettick();

	pc_stop_walking(sd,0);
	pc_stopattack(sd);
	switch(RFIFOB(fd,6)){
	case 0x00:	// once attack
	case 0x07:	// continuous attack
		if(sd->vender_id != 0) return;
		if(sd->invincible_timer != -1)
			pc_delinvincibletimer(sd);
		pc_attack(sd,RFIFOL(fd,2),RFIFOB(fd,6)!=0);
		break;
	case 0x02:	// sitdown
		pc_setsit(sd);
		WBUFW(buf,0)=0x26;
		WBUFL(buf,2)=sd->bl.id;
		WBUFB(buf,16)=2;
		clif_send(buf,packet_len_table[0x26],&sd->bl,AREA);
		break;
	case 0x03:	// standup
		pc_setstand(sd);
		WBUFW(buf,0)=0x26;
		WBUFL(buf,2)=sd->bl.id;
		WBUFB(buf,16)=3;						
		clif_send(buf,packet_len_table[0x26],&sd->bl,AREA);
		break;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Restart(int fd,struct map_session_data *sd)
{
	switch(RFIFOB(fd,2)){
	case 0x00:
		if(pc_isdead(sd)){
			pc_setstand(sd);
			pc_setrestartvalue(sd,3);
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,2);
		}
		break;
	case 0x01:
		if (battle_config.prevent_logout && (gettick() - sd->canlog_tick) >= 10000) {
			chrif_charselectreq(sd);
		}
		else {
			WFIFOW(fd,0)=0x18b;
			WFIFOW(fd,2)=1;

			WFIFOSET(fd,packet_len_table[0x018b]);
		}
		break;
	}
}

/*==========================================
 * Wisの送信
 *------------------------------------------
 */
void clif_parse_Wis(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,20))) return;
	intif_wis_message(sd,RFIFOP(fd,4),RFIFOP(fd,20),RFIFOW(fd,2)-20);

/*	struct map_session_data *dstsd;
	int dstfd;

	dstsd=map_nick2sd(RFIFOP(fd,4));

	if(dstsd==NULL){
		WFIFOW(fd,0)=0x33;
		WFIFOB(fd,2)=1;
		WFIFOSET(fd,packet_len_table[0x33]);
	} else {
		dstfd = dstsd->fd;

		WFIFOW(dstfd,0)=0x32;
		WFIFOW(dstfd,2)=RFIFOW(fd,2);
		memcpy(WFIFOP(dstfd,4),sd->status.name,16);
		memcpy(WFIFOP(dstfd,28),RFIFOP(fd,20),RFIFOW(fd,2)-20);
		WFIFOSET(dstfd,WFIFOW(dstfd,2));

		WFIFOW(fd,0)=0x33;
		WFIFOB(fd,2)=0;
		WFIFOSET(fd,packet_len_table[0x98]);
	}*/
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GMmessage(int fd,struct map_session_data *sd)
{
	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.broadcast)
		intif_GMmessage(RFIFOP(fd,4),RFIFOW(fd,2)-4,0);
/*	WFIFOW(fd,0)=0x35;
	WFIFOW(fd,2)=RFIFOW(fd,2);
	memcpy(WFIFOP(fd,4),RFIFOP(fd,4),RFIFOW(fd,2)-4);
	clif_send(WFIFOP(fd,0),RFIFOW(fd,2),&sd->bl,ALL_CLIENT);*/
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TakeItem(int fd,struct map_session_data *sd)
{
	struct flooritem_data *fitem=(struct flooritem_data*)map_id2bl(RFIFOL(fd,2));

	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0|| sd->sc_data[SC_AUTOCOUNTER].timer!=-1) return;

	if(fitem==NULL || fitem->bl.m != sd->bl.m)
		return;

	pc_takeitem(sd,fitem);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_DropItem(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0|| sd->sc_data[SC_AUTOCOUNTER].timer!=-1)return;
	pc_dropitem(sd,RFIFOW(fd,2)-2,RFIFOW(fd,4));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UseItem(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0 || sd->sc_data[SC_TRICKDEAD].timer != -1) return;

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	pc_useitem(sd,RFIFOW(fd,2)-2);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_EquipItem(int fd,struct map_session_data *sd)
{
	int index;
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	index = RFIFOW(fd,2)-2;
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0) return;

// taulin
/* no identify
	if(sd->status.inventory[index].identify != 1) {		// 未鑑定
		clif_equipitemack(sd,index,0,0);	// fail
		return;
	}*/

	if(sd->inventory_data[index]) {
		if(sd->inventory_data[index]->type != 8){
			if(sd->inventory_data[index]->type == 10)
				RFIFOW(fd,4)=0x8000;	// 矢を無理やり装備できるように（??；
			pc_equipitem(sd,index,RFIFOW(fd,4));
		}
		else
			pet_equipitem(sd,index);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UnequipItem(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0) return;
	pc_unequipitem(sd,RFIFOW(fd,2)-2,0);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ItemDescription(int fd,struct map_session_data *sd)
{
	clif_itemdescription(sd,RFIFOP(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcClicked(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->vender_id!=0) return;
	npc_click(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuySellSelected(int fd,struct map_session_data *sd)
{
	npc_buysellsel(sd,RFIFOL(fd,2),RFIFOB(fd,6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuyListSend(int fd,struct map_session_data *sd)
{
	int fail=0,n;
// taulin
	unsigned char *item_list;

	n = (RFIFOW(fd,2)-4) /18;
	item_list = (unsigned char*)RFIFOP(fd,4);
// end changes
	fail = npc_buylist(sd,n,item_list);

	WFIFOW(fd,0)=0xca;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xca]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSellListSend(int fd,struct map_session_data *sd)
{
	int fail=0,n;
	unsigned short *item_list;

	n = (RFIFOW(fd,2)-4) /4;
	item_list = (unsigned short*)RFIFOP(fd,4);

	fail = npc_selllist(sd,n,item_list);

	WFIFOW(fd,0)=0xcb;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xcb]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_CreateChatRoom(int fd,struct map_session_data *sd)
{
	chat_createchat(sd,RFIFOW(fd,4),RFIFOB(fd,6),RFIFOP(fd,7),RFIFOP(fd,15),RFIFOW(fd,2)-15);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatAddMember(int fd,struct map_session_data *sd)
{
	chat_joinchat(sd,RFIFOL(fd,2),RFIFOP(fd,6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatRoomStatusChange(int fd,struct map_session_data *sd)
{
	chat_changechatstatus(sd,RFIFOW(fd,4),RFIFOB(fd,6),RFIFOP(fd,7),RFIFOP(fd,15),RFIFOW(fd,2)-15);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeChatOwner(int fd,struct map_session_data *sd)
{
	chat_changechatowner(sd,RFIFOP(fd,6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_KickFromChat(int fd,struct map_session_data *sd)
{
	chat_kickchat(sd,RFIFOP(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatLeave(int fd,struct map_session_data *sd)
{
	chat_leavechat(sd);
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void clif_parse_TradeRequest(int fd,struct map_session_data *sd)
{
	trade_traderequest(sd,RFIFOL(sd->fd,2));
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void clif_parse_TradeAck(int fd,struct map_session_data *sd)
{
	trade_tradeack(sd,RFIFOB(sd->fd,2));
}

/*==========================================
 * アイテ?追加
 *------------------------------------------
 */
void clif_parse_TradeAddItem(int fd,struct map_session_data *sd)
{
	trade_tradeadditem(sd,RFIFOW(sd->fd,2),RFIFOL(sd->fd,4));
}

/*==========================================
 * アイテ?追加完了(ok押し)
 *------------------------------------------
 */
void clif_parse_TradeOk(int fd,struct map_session_data *sd)
{
	trade_tradeok(sd);
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void clif_parse_TradeCancel(int fd,struct map_session_data *sd)
{
	trade_tradecancel(sd);
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void clif_parse_TradeCommit(int fd,struct map_session_data *sd)
{
	trade_tradecommit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_StopAttack(int fd,struct map_session_data *sd)
{
	pc_stopattack(sd);
}

/*==========================================
 * カ?トへアイテ?を移す
 *------------------------------------------
 */
void clif_parse_PutItemToCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0) return;
	pc_putitemtocart(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}
/*==========================================
 * カ?トからアイテ?を出す
 *------------------------------------------
 */
void clif_parse_GetItemFromCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0) return;
	pc_getitemfromcart(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}

/*==========================================
 * 付属品(鷹,ペコ,カ?ト)をはずす
 *------------------------------------------
 */
void clif_parse_RemoveOption(int fd,struct map_session_data *sd)
{
	pc_setoption(sd,0);
}

/*==========================================
 * ?ェンジカ?ト
 *------------------------------------------
 */
void clif_parse_ChangeCart(int fd,struct map_session_data *sd)
{
	pc_setcart(sd,RFIFOW(fd,2));
}

/*==========================================
 * ステ??スアップ
 *------------------------------------------
 */
void clif_parse_StatusUp(int fd,struct map_session_data *sd)
{
	pc_statusup(sd,RFIFOW(fd,2));
}

/*==========================================
 * スキルレベルアップ
 *------------------------------------------
 */
void clif_parse_SkillUp(int fd,struct map_session_data *sd)
{
	pc_skillup(sd,RFIFOW(fd,2));
}

/*==========================================
 * スキル使用（ID指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToId(int fd,struct map_session_data *sd)
{
	int skillnum,skilllv,lv;
	unsigned int tick=gettick();

	if(sd->npc_id!=0 || sd->vender_id != 0) return;

	skillnum = RFIFOW(fd,4);
	skilllv = RFIFOW(fd,2);

	if(sd->skilltimer != -1) {
		if(skillnum != SA_CASTCANCEL)
			return;
	}
	else if(DIFF_TICK(tick , sd->canact_tick) < 0) {
		clif_skill_fail(sd,RFIFOW(fd,4),4,0);
		return;
	}

	if(sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) return;
	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);
	if(sd->skillitem == -1) {
		if(skillnum == MO_EXTREMITYFIST) {
			if((sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH)) {
				if(!sd->state.skill_flag ) {
					sd->state.skill_flag = 1;
					clif_skillinfo(sd,MO_EXTREMITYFIST,1,-1);
					return;
				}
				else if(sd->bl.id == RFIFOL(fd,6)) {
					clif_skillinfo(sd,MO_EXTREMITYFIST,1,-1);
					return;
				}
			}
		}
		if( (lv = pc_checkskill(sd,skillnum)) > 0) {
			if(skilllv > lv)
				skilllv = lv;
			skill_use_id(sd,RFIFOL(fd,6),skillnum,skilllv);
			if(sd->state.skill_flag)
				sd->state.skill_flag = 0;
		}
	}
	else {
		if(skilllv > sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_id(sd,RFIFOL(fd,6),skillnum,skilllv);
	}
}

/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToPos(int fd,struct map_session_data *sd)
{
	int skillnum,skilllv,lv;
	unsigned int tick=gettick();

	if(sd->npc_id!=0 || sd->vender_id != 0) return;

	skillnum = RFIFOW(fd,4);
	skilllv = RFIFOW(fd,2);

	if(sd->skilltimer != -1)
		return;
	else if(DIFF_TICK(tick , sd->canact_tick) < 0) {
		clif_skill_fail(sd,RFIFOW(fd,4),4,0);
		return;
	}

	if(sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) return;
	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);
	if(sd->skillitem == -1) {
		if( (lv = pc_checkskill(sd,skillnum)) > 0) {
			if(skilllv > lv)
				skilllv = lv;
			skill_use_pos(sd,RFIFOW(fd,6),RFIFOW(fd,8),skillnum,skilllv);
		}
	}
	else {
		if(skilllv > sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_pos(sd,RFIFOW(fd,6),RFIFOW(fd,8),skillnum,skilllv);
	}
}

/*==========================================
 * スキル使用（map指定）
 *------------------------------------------
 */
void clif_parse_UseSkillMap(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->sc_data[SC_TRICKDEAD].timer != -1) return;

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	skill_castend_map(sd,RFIFOW(fd,2),RFIFOP(fd,4));
}
/*==========================================
 * メモ要求
 *------------------------------------------
 */
void clif_parse_RequestMemo(int fd,struct map_session_data *sd)
{
	pc_memo(sd,-1);
}
/*==========================================
 * アイテ?合成
 *------------------------------------------
 */
void clif_parse_ProduceMix(int fd,struct map_session_data *sd)
{
	skill_produce_mix(sd,RFIFOW(fd,2),RFIFOW(fd,4),RFIFOW(fd,6),RFIFOW(fd,8));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSelectMenu(int fd,struct map_session_data *sd)
{
	sd->npc_menu=RFIFOB(fd,6);
	npc_scriptcont(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcNextClicked(int fd,struct map_session_data *sd)
{
	npc_scriptcont(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcAmountInput(int fd,struct map_session_data *sd)
{
	sd->npc_amount=RFIFOL(fd,6);
	npc_scriptcont(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcCloseClicked(int fd,struct map_session_data *sd)
{
	// nop
}

/*==========================================
 * アイテ?鑑定
 *------------------------------------------
 */
void clif_parse_ItemIdentify(int fd,struct map_session_data *sd)
{
	pc_item_identify(sd,RFIFOW(fd,2)-2);
}
/*==========================================
 * 矢作成
 *------------------------------------------
 */
void clif_parse_SelectArrow(int fd,struct map_session_data *sd)
{
	skill_arrow_create(sd,RFIFOW(fd,2));
}
/*==========================================
 * カ?ド使用
 *------------------------------------------
 */
void clif_parse_UseCard(int fd,struct map_session_data *sd)
{
	clif_use_card(sd,RFIFOW(fd,2)-2);
}
/*==========================================
 * カ?ド?入装備選択
 *------------------------------------------
 */
void clif_parse_InsertCard(int fd,struct map_session_data *sd)
{
	pc_insert_card(sd,RFIFOW(fd,2)-2,RFIFOW(fd,4)-2);
}


/*==========================================
 * 0193 キャラID名前引き
 *------------------------------------------
 */
void clif_parse_SolveCharName(int fd,struct map_session_data *sd)
{
	clif_solved_charname(sd,RFIFOL(fd,2));
}

/*==========================================
 * 0197 /resetskill /resetstate
 *------------------------------------------
 */
void clif_parse_ResetChar(int fd,struct map_session_data *sd)
{
	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.resetstate) {
		switch(RFIFOW(fd,2)){
		case 0:
			pc_resetstate(sd);
			break;
		case 1:
			pc_resetskill(sd);
			break;
		}
	}
}

/*==========================================
 * 019c /lb等
 *------------------------------------------
 */
void clif_parse_LGMmessage(int fd,struct map_session_data *sd)
{
	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.local_broadcast) {
		WFIFOW(fd,0)=0x35;
		WFIFOW(fd,2)=RFIFOW(fd,2);
		memcpy(WFIFOP(fd,4),RFIFOP(fd,4),RFIFOW(fd,2)-4);
		clif_send(WFIFOP(fd,0),RFIFOW(fd,2),&sd->bl,ALL_SAMEMAP);
	}
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafra(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0) return;
	storage_storageadd(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafra(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0) return;
	storage_storageget(sd,RFIFOW(fd,2)-1,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫へカ?トから入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafraFromCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0) return;
	storage_storageaddfromcart(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafraToCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0 || sd->vender_id != 0) return;
	storage_storagegettocart(sd,RFIFOW(fd,2)-1,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
void clif_parse_CloseKafra(int fd,struct map_session_data *sd)
{
	storage_storageclose(sd);
}

/*==========================================
 * パ?ティを作る
 *------------------------------------------
 */
void clif_parse_CreateParty(int fd,struct map_session_data *sd)
{
	party_create(sd,RFIFOP(fd,2));
}

/*==========================================
 * パ?ティを作る
 *------------------------------------------
 */
void clif_parse_CreateParty2(int fd,struct map_session_data *sd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 7){
		party_create(sd,RFIFOP(fd,2));
	}
	else
		clif_skill_fail(sd,1,0,4);
}

/*==========================================
 * パ?ティに勧誘
 *------------------------------------------
 */
void clif_parse_PartyInvite(int fd,struct map_session_data *sd)
{
	party_invite(sd,RFIFOL(fd,2));
}
/*==========================================
 * パ?ティ勧誘返答
 *------------------------------------------
 */
void clif_parse_ReplyPartyInvite(int fd,struct map_session_data *sd)
{
	party_reply_invite(sd,RFIFOL(fd,2),RFIFOL(fd,6));
}
/*==========================================
 * パ?ティ脱退要求
 *------------------------------------------
 */
void clif_parse_LeaveParty(int fd,struct map_session_data *sd)
{
	party_leave(sd);
}
/*==========================================
 * パ?ティ除名要求
 *------------------------------------------
 */
void clif_parse_RemovePartyMember(int fd,struct map_session_data *sd)
{
	party_removemember(sd,RFIFOL(fd,2),RFIFOP(fd,6));
}
/*==========================================
 * パ?ティ設定変更要求
 *------------------------------------------
 */
void clif_parse_PartyChangeOption(int fd,struct map_session_data *sd)
{
	party_changeoption(sd,RFIFOW(fd,2),RFIFOW(fd,4));
}
/*==========================================
 * パ?ティメッセ?ジ送信要求
 *------------------------------------------
 */
void clif_parse_PartyMessage(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,4))) return;
	party_send_message(sd,RFIFOP(fd,4),RFIFOW(fd,2)-4);
}

/*==========================================
 * 露店閉鎖
 *------------------------------------------
 */
void clif_parse_CloseVending(int fd,struct map_session_data *sd)
{
	vending_closevending(sd);
}

/*==========================================
 * 露店アイテ?リスト要求
 *------------------------------------------
 */
void clif_parse_VendingListReq(int fd,struct map_session_data *sd)
{
	vending_vendinglistreq(sd,RFIFOL(fd,2));
}

/*==========================================
 * 露店アイテ?購入
 *------------------------------------------
 */
void clif_parse_PurchaseReq(int fd,struct map_session_data *sd)
{
	vending_purchasereq(sd,RFIFOW(fd,2),RFIFOL(fd,4),RFIFOP(fd,8));
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
void clif_parse_OpenVending(int fd,struct map_session_data *sd)
{
	vending_openvending(sd,RFIFOW(fd,2),RFIFOP(fd,4),RFIFOB(fd,84),RFIFOP(fd,85));
}

/*==========================================
 * ギルドを作る
 *------------------------------------------
 */
void clif_parse_CreateGuild(int fd,struct map_session_data *sd)
{
	guild_create(sd,RFIFOP(fd,6));
}
/*==========================================
 * ギルド?ス??かどうか確認
 *------------------------------------------
 */
void clif_parse_GuildCheckMaster(int fd,struct map_session_data *sd)
{
	clif_guild_masterormember(sd);
}
/*==========================================
 * ギルド情報要求
 *------------------------------------------
 */
void clif_parse_GuildReqeustInfo(int fd,struct map_session_data *sd)
{
	switch(RFIFOL(fd,2)){
	case 0:	// ギルド基?情報、同盟敵対情報
		clif_guild_basicinfo(sd);
		clif_guild_allianceinfo(sd);
		break;
	case 1:	// メンバ?リスト、役職名リスト
		clif_guild_positionnamelist(sd);
		clif_guild_memberlist(sd);
		break;
	case 2:	// 役職名リスト、役職情報リスト
		clif_guild_positionnamelist(sd);
		clif_guild_positioninfolist(sd);
		break;
	case 3:	// スキルリスト
		clif_guild_skillinfo(sd);
		break;
	case 4:	// 追放リスト
		clif_guild_explusionlist(sd);
		break;
	default:
		if(battle_config.error_log)
			printf("clif: guild request info: unknown type %d\n",RFIFOL(fd,2));
		break;
	}
}
/*==========================================
 * ギルド役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangePositionInfo(int fd,struct map_session_data *sd)
{
	int i;
	for(i=4;i<RFIFOW(fd,2);i+=40){
		guild_change_position(sd,RFIFOL(fd,i),
			RFIFOL(fd,i+4),RFIFOL(fd,i+12),RFIFOP(fd,i+16));
	}
}
/*==========================================
 * ギルドメンバ役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangeMemberPosition(int fd,struct map_session_data *sd)
{
	int i;
	for(i=4;i<RFIFOW(fd,2);i+=12){
		guild_change_memberposition(sd->status.guild_id,
			RFIFOL(fd,i),RFIFOL(fd,i+4),RFIFOL(fd,i+8));
	}
}

/*==========================================
 * ギルドエンブレ?要求
 *------------------------------------------
 */
void clif_parse_GuildRequestEmblem(int fd,struct map_session_data *sd)
{
	struct guild *g=guild_search(RFIFOL(fd,2));
	if(g!=NULL)
		clif_guild_emblem(sd,g);
}
/*==========================================
 * ギルドエンブレ?変更
 *------------------------------------------
 */
void clif_parse_GuildChangeEmblem(int fd,struct map_session_data *sd)
{
	guild_change_emblem(sd,RFIFOW(fd,2)-4,RFIFOP(fd,4));
}
/*==========================================
 * ギルド告知変更
 *------------------------------------------
 */
void clif_parse_GuildChangeNotice(int fd,struct map_session_data *sd)
{
	guild_change_notice(sd,RFIFOL(fd,2),RFIFOP(fd,6),RFIFOP(fd,66));
}

/*==========================================
 * ギルド勧誘
 *------------------------------------------
 */
void clif_parse_GuildInvite(int fd,struct map_session_data *sd)
{
	guild_invite(sd,RFIFOL(fd,2));
}
/*==========================================
 * ギルド勧誘返信
 *------------------------------------------
 */
void clif_parse_GuildReplyInvite(int fd,struct map_session_data *sd)
{
	guild_reply_invite(sd,RFIFOL(fd,2),RFIFOB(fd,6));
}
/*==========================================
 * ギルド脱退
 *------------------------------------------
 */
void clif_parse_GuildLeave(int fd,struct map_session_data *sd)
{
	guild_leave(sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOP(fd,14));
}
/*==========================================
 * ギルド追放
 *------------------------------------------
 */
void clif_parse_GuildExplusion(int fd,struct map_session_data *sd)
{
	guild_explusion(sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOP(fd,14));
}
/*==========================================
 * ギルド会話
 *------------------------------------------
 */
void clif_parse_GuildMessage(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,4))) return;
	guild_send_message(sd,RFIFOP(fd,4),RFIFOW(fd,2)-4);
}
/*==========================================
 * ギルド同盟要求
 *------------------------------------------
 */
void clif_parse_GuildRequestAlliance(int fd,struct map_session_data *sd)
{
	guild_reqalliance(sd,RFIFOL(fd,2));
}
/*==========================================
 * ギルド同盟要求返信
 *------------------------------------------
 */
void clif_parse_GuildReplyAlliance(int fd,struct map_session_data *sd)
{
	guild_reply_reqalliance(sd,RFIFOL(fd,2),RFIFOL(fd,6));
}
/*==========================================
 * ギルド関係解消
 *------------------------------------------
 */
void clif_parse_GuildDelAlliance(int fd,struct map_session_data *sd)
{
	guild_delalliance(sd,RFIFOL(fd,2),RFIFOL(fd,6));
}
/*==========================================
 * ギルド敵対
 *------------------------------------------
 */
void clif_parse_GuildOpposition(int fd,struct map_session_data *sd)
{
	guild_opposition(sd,RFIFOL(fd,2));
}
/*==========================================
 * ギルド解散
 *------------------------------------------
 */
void clif_parse_GuildBreak(int fd,struct map_session_data *sd)
{
	guild_break(sd,RFIFOP(fd,2));
}

// pet
void clif_parse_PetMenu(int fd,struct map_session_data *sd)
{
	pet_menu(sd,RFIFOB(fd,2));
}
void clif_parse_CatchPet(int fd,struct map_session_data *sd)
{
	pet_catch_process2(sd,RFIFOL(fd,2));
}

void clif_parse_SelectEgg(int fd,struct map_session_data *sd)
{
	pet_select_egg(sd,RFIFOW(fd,2)-2);
}

void clif_parse_SendEmotion(int fd,struct map_session_data *sd)
{
	if(sd->pd)
		clif_pet_emotion(sd->pd,RFIFOL(fd,2));
}

void clif_parse_ChangePetName(int fd,struct map_session_data *sd)
{
	pet_change_name(sd,RFIFOP(fd,2));
}

// Kick
void clif_parse_GMKick(int fd,struct map_session_data *sd)
{
	struct block_list *target;
	int tid = RFIFOL(fd,2);

	if(pc_isGM(sd) >= atcommand_config.kick) {
		target = map_id2bl(tid);
		if(target) {
			if(target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if(pc_isGM(sd) > pc_isGM(tsd))
					clif_GM_kick(sd,tsd,1);
				else
					clif_GM_kickack(sd,0);
			}
			else if(target->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)target;
				sd->state.attack_type = 0;
				mob_damage(&sd->bl,md,md->hp,2);
			}
			else
				clif_GM_kickack(sd,0);
		}
		else
			clif_GM_kickack(sd,0);
	}
}

/*==========================================
 * GM funtions
 *------------------------------------------
 */
void clif_parse_Shift(int fd,struct map_session_data *sd)	// Added by RoVeRT
{
	struct map_session_data *pl_sd;

	if ((pl_sd=map_nick2sd(RFIFOP(fd,2)))!=NULL && pc_isGM(sd) >= atcommand_config.jumpto)
		pc_setpos(sd, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y, 3);
}

void clif_parse_Recall(int fd,struct map_session_data *sd)	// Added by RoVeRT
{
	struct map_session_data *pl_sd;

	if ((pl_sd=map_nick2sd(RFIFOP(fd,2)))!=NULL && pc_isGM(sd) >= atcommand_config.recall) {
		if(pc_isGM(sd) > pc_isGM(pl_sd))
			pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2);
	}
}

void clif_parse_GMHide(int fd,struct map_session_data *sd)	// Added by RoVeRT
{
	if(pc_isGM(sd) >= atcommand_config.hide) {
		if(sd->status.option&0x40){
			sd->status.option&=~0x40;
			clif_displaymessage(fd,"Invisible: Off.");
		}else{
			sd->status.option|=0x40;
			clif_displaymessage(fd,"Invisible: On.");
		}
		clif_changeoption(&sd->bl);
	}
}

void clif_parse_PMIgnore(int fd,struct map_session_data *sd)	// Added by RoVeRT
{
	char *c = RFIFOP(fd,2);
	int t = RFIFOB(fd,12),i;
	t = t==0?1:0;

	for(i=0;i<sizeof(sd->ignore)/sizeof(sd->ignore[0]);i++)
		if(sd->ignore[i].state==-1 || strcmp(sd->ignore[i].name,c)==0)
			break;

	strcpy(sd->ignore[i].name,c);
	sd->ignore[i].state=t;


//name search
	for(i=0;i<sizeof(sd->ignore)/sizeof(sd->ignore[0]);i++){
		if(sd->ignore[i].state==-1)
			break;
		printf("ig:%i %s %d\n",i, sd->ignore[i].name,sd->ignore[i].state);
	}


}

void clif_parse_PMIgnoreAll(int fd,struct map_session_data *sd)		// Added by RoVeRT
{
	int t = RFIFOB(fd,2);
	if (t==0)
		sd->ignoreAll=1;
	else
		sd->ignoreAll=0;
}


int sendtalky(int fd,char *mes,int x,int y)
{
	// not working
	WFIFOW(fd,0)=0x191;
	WFIFOL(fd,2)=10002047;
	memcpy(WFIFOP(fd,6),mes,80);
	WFIFOSET(fd,packet_len_table[0x191]);

	return 0;
}

int monk(struct map_session_data *sd,struct block_list *target,int type)
{
//R 01d1 <Monk id>L <Target monster id>L <Bool>L
	int fd=sd->fd;
	WFIFOW(fd,0)=0x1d1;
	WFIFOL(fd,2)=sd->bl.id;
	WFIFOL(fd,6)=target->id;
	WFIFOL(fd,10)=type;
	WFIFOSET(fd,packet_len_table[0x1d1]);

	return 0;
}

void clif_parse_skillMessage(int fd,struct map_session_data *sd)	// Added by RoVeRT
{
	int skillid,skilllv,x,y;
	char *mes;

	skilllv = RFIFOW(fd,2);
	skillid = RFIFOW(fd,4);

	y = RFIFOB(fd,6);
	x = RFIFOB(fd,8);

	mes = RFIFOP(fd,10);

if (skillid== 125)
	sendtalky(fd,mes,x,y);	// or whatever

// skill 220 = graffiti


//printf("skill: %d %d location: %3d %3d message: %s\n",skillid,skilllv,x,y,(char*)mes);
}

int clif_skill_list_send(struct map_session_data *sd,int skills[],int limit)
{
	int i;
	WFIFOW(sd->fd,0)=0x1cd;

	for(i=0;i<7;i++)
		WFIFOL(sd->fd,2+4*i)=(i < limit && pc_checkskill(sd,skills[i]) > 0)?skills[i]:0;

	WFIFOSET(sd->fd,packet_len_table[0x1cd]);

	return 0;
}

void clif_parse_selected_spell(int fd,struct map_session_data *sd)
{
	int skilllv=RFIFOL(fd,2);
	skill_status_change_start((struct block_list*) sd, SC_AUTOSPELL, pc_checkskill(sd,SA_AUTOSPELL), skilllv, 1000 * (90 + 30 * skilllv),0 );
}

/*==========================================
 * GMによる?ャット禁?時間参照（？）
 *------------------------------------------
 */
void clif_parse_GMReqNoChatCount(int fd,struct map_session_data *sd)
{
	/*	返信パケットが不明だが、放置でもエラ?にはならない。
		ただ、何度も呼ばれるので鯖の負荷がほんの少し上がるかも知れない。
		0xb0のtype4ではない模様(0xb0は禁?される側に送られると?想)。 */
	return;
}

/*==========================================
 * クライアントからのパケット解析
 * socket.cのdo_parsepacketから呼び出される
 *------------------------------------------
 */
static int clif_parse(int fd)
{
	int packet_len=0,cmd;
	struct map_session_data *sd;
	static void (*clif_parse_func_table[0x200])() = {
		// 00
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,clif_parse_WantToConnection,NULL,
		// 10
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,clif_parse_LoadEndAck,clif_parse_TickSend,NULL,NULL,NULL,NULL,NULL,
		// 20
		NULL,clif_parse_WalkToXY,NULL,NULL,NULL,clif_parse_ActionRequest,NULL,clif_parse_GlobalMessage, NULL,NULL,NULL,clif_parse_NpcClicked,NULL,NULL,NULL,clif_parse_GetCharNameRequest,
		// 30
		NULL,clif_parse_Wis,NULL,NULL,clif_parse_GMmessage,NULL,clif_parse_ChangeDir,NULL, NULL,NULL,clif_parse_TakeItem,NULL,NULL,clif_parse_DropItem,NULL,NULL,
		// 40
		NULL,NULL,clif_parse_UseItem,NULL,clif_parse_EquipItem,NULL,clif_parse_UnequipItem,NULL, clif_parse_ItemDescription,NULL,NULL,NULL,NULL,clif_parse_Restart,NULL,NULL,
		// 50
		NULL,NULL,NULL,clif_parse_NpcSelectMenu,clif_parse_NpcNextClicked,NULL,clif_parse_StatusUp,NULL, NULL,NULL,clif_parse_Emotion,NULL,clif_parse_HowManyConnections,NULL,NULL,NULL,
		// 60
		clif_parse_NpcBuySellSelected,NULL,NULL,clif_parse_NpcBuyListSend,clif_parse_NpcSellListSend,NULL,NULL,clif_parse_GMKick, NULL,NULL,NULL,clif_parse_PMIgnore,clif_parse_PMIgnoreAll,NULL,NULL,NULL,
		// 70
		clif_parse_CreateChatRoom,NULL,NULL,NULL,clif_parse_ChatAddMember,NULL,NULL,NULL, NULL,clif_parse_ChatRoomStatusChange,NULL,clif_parse_ChangeChatOwner,NULL,clif_parse_KickFromChat,clif_parse_ChatLeave,clif_parse_TradeRequest,
		// 80
		NULL,clif_parse_TradeAck,NULL,clif_parse_TradeAddItem,NULL,NULL,clif_parse_TradeOk,NULL, clif_parse_TradeCancel,NULL,clif_parse_TradeCommit,NULL,NULL,NULL,NULL,NULL,
		// 90
		NULL,
		NULL,NULL,NULL,
		clif_parse_CreateParty,
		NULL,NULL,
		clif_parse_PartyInvite, 
		NULL,NULL,
		clif_parse_ReplyPartyInvite,
		clif_parse_LeaveParty,
		clif_parse_RemovePartyMember,
		NULL,NULL,NULL,
		// a0
		NULL,
		clif_parse_PartyMessage,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// b0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// c0
		NULL,
		NULL,
		NULL,NULL,NULL,
		NULL,
		NULL,NULL,
		NULL,
		NULL,
		NULL,NULL,
		NULL,
		NULL,NULL,
		NULL,
		// d0
		NULL,
		NULL,NULL,NULL,NULL,
		NULL,
		NULL,NULL, NULL,
		NULL,
		NULL,NULL,NULL,NULL,
		NULL,
		NULL,
		// e0
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		// f0
		NULL,NULL,NULL,
		clif_parse_MoveToKafra,
		NULL,
		clif_parse_MoveFromKafra,
		NULL,
		clif_parse_CloseKafra,
		NULL,
		NULL,
		NULL,NULL,
		NULL,
		NULL,NULL,
		NULL,

		// 100
		NULL,
		NULL,
		clif_parse_PartyChangeOption,
		NULL,
		NULL,NULL,NULL,NULL, 
		NULL,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// 110
		NULL,NULL,
		clif_parse_SkillUp,
		clif_parse_UseSkillToId,
		NULL,NULL,
		clif_parse_UseSkillToPos,
		NULL,
		clif_parse_StopAttack,
		NULL,NULL,
		clif_parse_UseSkillMap,
		NULL,
		clif_parse_RequestMemo,
		NULL,NULL,
		// 120
		NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_PutItemToCart,
		clif_parse_GetItemFromCart,
		clif_parse_MoveFromKafraToCart,
		clif_parse_MoveToKafraFromCart,
		clif_parse_RemoveOption,
		NULL,NULL,NULL,
		clif_parse_CloseVending,
		NULL,
		// 130
		clif_parse_VendingListReq,
		NULL,NULL,NULL,
		clif_parse_PurchaseReq,
		NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

		// 140
		clif_parse_MapMove,
		NULL,NULL,
		clif_parse_NpcAmountInput,
		NULL,NULL,
		clif_parse_NpcCloseClicked,
		NULL, NULL,NULL,NULL,NULL,NULL,
		clif_parse_GuildCheckMaster,
		NULL,
		clif_parse_GuildReqeustInfo,
		// 150
		NULL,
		clif_parse_GuildRequestEmblem,
		NULL,
		clif_parse_GuildChangeEmblem,
		NULL,
		clif_parse_GuildChangeMemberPosition,
		NULL,NULL, NULL,
		clif_parse_GuildLeave,
		NULL,
		clif_parse_GuildExplusion,
		NULL,
		clif_parse_GuildBreak,
		NULL,NULL,
		// 160
		NULL,
		clif_parse_GuildChangePositionInfo,
		NULL,NULL,NULL,
		clif_parse_CreateGuild,
		NULL,NULL, 
		clif_parse_GuildInvite,
		NULL,NULL,
		clif_parse_GuildReplyInvite,
		NULL,NULL,
		clif_parse_GuildChangeNotice,
		NULL,
		// 170
		clif_parse_GuildRequestAlliance,
		NULL,
		clif_parse_GuildReplyAlliance,
		NULL,NULL,NULL,NULL,NULL, 
		clif_parse_ItemIdentify,
		NULL,
		clif_parse_UseCard,
		NULL,
		clif_parse_InsertCard,
		NULL,
		clif_parse_GuildMessage,
		NULL,

		// 180
		clif_parse_GuildOpposition,
		NULL,NULL,
		clif_parse_GuildDelAlliance,
		NULL,NULL,NULL,NULL, NULL,NULL,
		clif_parse_QuitGame,
		NULL,NULL,NULL,
		clif_parse_ProduceMix,
		NULL,
		// 190
		clif_parse_skillMessage,
		NULL,NULL,
		clif_parse_SolveCharName,
		NULL,NULL,NULL,
		clif_parse_ResetChar,
		NULL,NULL,NULL,NULL,
		clif_parse_LGMmessage,
		clif_parse_GMHide,
		NULL,
		clif_parse_CatchPet,
		// 1a0
		NULL,
		clif_parse_PetMenu,
		NULL,NULL,NULL,
		clif_parse_ChangePetName,
		NULL,
		clif_parse_SelectEgg,
		NULL,
		clif_parse_SendEmotion,
		NULL,
		NULL,NULL,NULL,
		clif_parse_SelectArrow,
		clif_parse_ChangeCart,
		// 1b0
		NULL,NULL,
		clif_parse_OpenVending,
		NULL,NULL,NULL,NULL,NULL, NULL,NULL,
		clif_parse_Shift,	// Added by RoVeRT
		clif_parse_Shift,
		clif_parse_Recall,
		clif_parse_Recall,	// End Addition
		NULL,NULL,

		// 1c0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_selected_spell,
		NULL,
		// 1d0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_GMReqNoChatCount,
		// 1e0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_CreateParty2,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// 1f0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

#if 0
	case 0xcc:	clif_parse_GMkill
	case 0xce:	clif_parse_GMkillall
	case 0xcf:	clif_parse_Ignore
	case 0xd0:	clif_parse_IgnoreAll
	case 0xd3:	clif_parse_IgnoreList
//	case 0xf3:	clif_parse_MoveToKafra
//	case 0xf5:	clif_parse_MoveFromKafra
//	case 0xf7:	clif_parse_CloseKafra
	case 0xf9:	clif_parse_CreateParty
	case 0xfc:	clif_parse_InviteParty
	case 0xff:	clif_parse_InvitePartyAck
	case 0x108:	clif_parse_PartyMessage
	case 0x11b:	clif_parse_TeleportSelect
	case 0x11d:	clif_parse_MemoRequest
	case 0x126:	clif_parse_MoveToCart
	case 0x127:	clif_parse_MoveFromCart
	case 0x128:	clif_parse_MoveToCartFromKafra
	case 0x129:	clif_parse_MoveFromCartToKafra
	case 0x12a:	clif_parse_RemoveOption
#endif
	};

	// char鯖に繋がってない間は接続禁?
	if(!chrif_isconnect())
		session[fd]->eof = 1;

	sd = session[fd]->session_data;

	// 接続が切れてるので後始末
	if(session[fd]->eof){
		if(sd && sd->state.auth)
			clif_quitsave(fd,sd);
		close(fd);
		delete_session(fd);
		return 0;
	}

	if(RFIFOREST(fd)<2)
		return 0;
	cmd = RFIFOW(fd,0);
	
	// 管理用パケット処理
	if(cmd>=30000){
		switch(cmd){
		case 0x7530:	// Athena情報所得
			WFIFOW(fd,0)=0x7531;
			WFIFOB(fd,2)=ATHENA_MAJOR_VERSION;
			WFIFOB(fd,3)=ATHENA_MINOR_VERSION;
			WFIFOB(fd,4)=ATHENA_REVISION;
			WFIFOB(fd,5)=ATHENA_RELEASE_FLAG;
			WFIFOB(fd,6)=ATHENA_OFFICIAL_FLAG;
			WFIFOB(fd,7)=ATHENA_SERVER_MAP;
			WFIFOW(fd,8)=ATHENA_MOD_VERSION;
			WFIFOSET(fd,10);
			RFIFOSKIP(fd,2);
			break;
		case 0x7532:	// 接続の切断
			close(fd);
			session[fd]->eof=1;
			break;
		}
		return 0;
	}
	
	// ゲ??用以外パケットか、認証を終える前に0072以外が来たら、切断する
	if(cmd>=0x200 || packet_len_table[cmd]==0 ||
	   ((!sd || (sd && sd->state.auth==0)) && cmd!=0x0e) ){
		close(fd);
		session[fd]->eof = 1;
		if(packet_len_table[cmd]==0) {
			printf("clif_parse : %d %d %x\n",fd,packet_len_table[cmd],cmd);
			printf("%x length 0 packet disconnect %d\n",cmd,fd);
		}
		return 0;
	}
	// パケット長を計算
	packet_len = packet_len_table[cmd];
	if(packet_len==-1){
		if(RFIFOREST(fd)<4)
			return 0;	// 可変長パケットで長さの所までデ??が来てない
		packet_len = RFIFOW(fd,2);
		if(packet_len<4 || packet_len>32768){
			close(fd);
			session[fd]->eof =1;
			return 0;
		}
	}
	if(RFIFOREST(fd)<packet_len)
		return 0;	// まだ1パケット分デ??が揃ってない

	if(sd && sd->state.auth==1 &&
		sd->state.waitingdisconnect==1 ){// 切断待ちの場合パケットを処理しない
		
	}else if(clif_parse_func_table[cmd]){
		// パケット処理
		clif_parse_func_table[cmd](fd,sd);
	} else {
		// 不明なパケット
		if(battle_config.error_log) {
			printf("clif_parse : %d %d %x\n",fd,packet_len,cmd);
#ifdef DUMP_UNKNOWN_PACKET
			{
				int i;
				printf("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
				for(i=0;i<packet_len;i++){
					if((i&15)==0)
						printf("\n%04X ",i);
					printf("%02X ",RFIFOB(fd,i));
				}
				printf("\n");
			}
#endif
		}
	}
	RFIFOSKIP(fd,packet_len);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_clif(void)
{
	int i;

	set_defaultparse(clif_parse);
	for(i=0;i<10;i++){
		if(make_listen_port(map_port))
			break;
		sleep(20);
	}
	if(i==10){
		printf("cant bind game port\n");
		exit(1);
	}
	add_timer_func_list(clif_waitclose,"clif_waitclose");

	return 0;
}

