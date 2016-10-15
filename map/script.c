// $Id: script.c,v 1.27 2004/02/17 20:00:42 rovert Exp $
//#define DEBUG_FUNCIN
//#define DEBUG_DISP
//#define DEBUG_RUN

//following by RoVeRT
//updated getitem, delitem, countiem, checkweight to use either item names or numbers (ex.  717 = blue gem or Blue_Gemstone = blue gem)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "socket.h"
#include "timer.h"

#include "map.h"
#include "clif.h"
#include "chrif.h"
#include "itemdb.h"
#include "pc.h"
#include "script.h"
#include "storage.h"
#include "mob.h"
#include "npc.h"
#include "pet.h"
#include "intif.h"
#include "db.h"
#include "skill.h"
#include "chat.h"
#include "battle.h"
#include "party.h"
#include "guild.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define SCRIPT_BLOCK_SIZE 256
enum { LABEL_NEXTLINE=1,LABEL_START };
static unsigned char * script_buf;
static int script_pos,script_size;

char *str_buf;
int str_pos,str_size;
static struct {
	int type;
	int str;
	int backpatch;
	int label;
	int (*func)();
	int val;
	int next;
} *str_data;
int str_num=LABEL_START,str_data_size;
int str_hash[16];

static struct dbt *mapval_db;

static struct dbt *scriptlabel_db=NULL;
struct dbt* script_get_label_db(){ return scriptlabel_db; }
static int scriptlabel_final(void *k,void *d,va_list ap){ return 0; }
static char pos[11][100] = {"Head","Body","Left hand","Right hand","Robe","Shoes","Accessory 1","Accessory 2","Head 2","Head 3","Not Equipped"};

/*==========================================
 * ���[�J���v���g�^�C�v�錾 (�K�v�ȕ��̂�)
 *------------------------------------------
 */
unsigned char* parse_subexpr(unsigned char *,int);
int buildin_mes(struct script_state *st);
int buildin_goto(struct script_state *st);
int buildin_next(struct script_state *st);
int buildin_close(struct script_state *st);
int buildin_menu(struct script_state *st);
int buildin_rand(struct script_state *st);
int buildin_warp(struct script_state *st);
int buildin_areawarp(struct script_state *st);
int buildin_heal(struct script_state *st);
int buildin_itemheal(struct script_state *st);
int buildin_percentheal(struct script_state *st);
int buildin_jobchange(struct script_state *st);
int buildin_input(struct script_state *st);
int buildin_setlook(struct script_state *st);
int buildin_set(struct script_state *st);
int buildin_if(struct script_state *st);
int buildin_getitem(struct script_state *st);
int buildin_delitem(struct script_state *st);
int buildin_viewpoint(struct script_state *st);
int buildin_countitem(struct script_state *st);
int buildin_checkweight(struct script_state *st);
int buildin_readparam(struct script_state *st);
int buildin_getcharid(struct script_state *st);
int buildin_getpartyname(struct script_state *st);
int buildin_getguildname(struct script_state *st);
int buildin_getguildmaster(struct script_state *st);
int buildin_strcharinfo(struct script_state *st);
int buildin_getequipname(struct script_state *st);
int buildin_getequipisequiped(struct script_state *st);
int buildin_getequipisenableref(struct script_state *st);
int buildin_getequipisidentify(struct script_state *st);
int buildin_getequiprefinerycnt(struct script_state *st);
int buildin_getequipweaponlv(struct script_state *st);
int buildin_getequippercentrefinery(struct script_state *st);
int buildin_successrefitem(struct script_state *st);
int buildin_failedrefitem(struct script_state *st);
int buildin_cutin(struct script_state *st);
int buildin_statusup(struct script_state *st);
int buildin_statusup2(struct script_state *st);
int buildin_bonus(struct script_state *st);
int buildin_bonus2(struct script_state *st);
int buildin_bonus3(struct script_state *st);
int buildin_skill(struct script_state *st);
int buildin_getskilllv(struct script_state *st);
int buildin_basicskillcheck(struct script_state *st);
int buildin_getgmlevel(struct script_state *st);
int buildin_end(struct script_state *st);
int buildin_checkoption(struct script_state *st);
int buildin_setoption(struct script_state *st);
int buildin_setcart(struct script_state *st);
int buildin_setfalcon(struct script_state *st);
int buildin_setriding(struct script_state *st);
int buildin_savepoint(struct script_state *st);
int buildin_openstorage(struct script_state *st);
int buildin_itemskill(struct script_state *st);
int buildin_produce(struct script_state *st);
int buildin_monster(struct script_state *st);
int buildin_areamonster(struct script_state *st);
int buildin_killmonster(struct script_state *st);
int buildin_doevent(struct script_state *st);
int buildin_addtimer(struct script_state *st);
int buildin_deltimer(struct script_state *st);
int buildin_addtimercount(struct script_state *st);
int buildin_announce(struct script_state *st);
int buildin_mapannounce(struct script_state *st);
int buildin_areaannounce(struct script_state *st);
int buildin_getusers(struct script_state *st);
int buildin_getmapusers(struct script_state *st);
int buildin_getareausers(struct script_state *st);
int buildin_enablenpc(struct script_state *st);
int buildin_disablenpc(struct script_state *st);
int buildin_enablearena(struct script_state *st);	// Added by RoVeRT
int buildin_disablearena(struct script_state *st);	// Added by RoVeRT
int buildin_sc_start(struct script_state *st);
int buildin_sc_end(struct script_state *st);
int buildin_debugmes(struct script_state *st);
int buildin_catchpet(struct script_state *st);
int buildin_birthpet(struct script_state *st);
int buildin_resetstatus(struct script_state *st);
int buildin_resetskill(struct script_state *st);
int buildin_changebase(struct script_state *st);
int buildin_changesex(struct script_state *st);
int buildin_waitingroom(struct script_state *st);
int buildin_warpwaitingpc(struct script_state *st);
int buildin_setmapflagnosave(struct script_state *st);
int buildin_setmapflag(struct script_state *st);
int buildin_removemapflag(struct script_state *st);
int buildin_pvpon(struct script_state *st);
int buildin_pvpoff(struct script_state *st);
int buildin_gvgon(struct script_state *st);
int buildin_gvgoff(struct script_state *st);
int buildin_emotion(struct script_state *st);
int buildin_maprespawnguildid(struct script_state *st);
int buildin_agitstart(struct script_state *st);		// <Agit>
int buildin_agitend(struct script_state *st);
int buildin_getcastlename(struct script_state *st);
int buildin_getcastledata(struct script_state *st);
int buildin_setcastledata(struct script_state *st);
int buildin_mapwarp(struct script_state *st);
int buildin_inittimer(struct script_state *st);
int buildin_stoptimer(struct script_state *st);
int buildin_cmdothernpc(struct script_state *st);
int buildin_mobcount(struct script_state *st);
int buildin_getequipcardcnt(struct script_state *st);
int buildin_successremovecards(struct script_state *st);
int buildin_failedremovecards(struct script_state *st);

void push_val(struct script_stack *stack,int type,int val);
int run_func(struct script_state *st);

struct {
	int (*func)();
	char *name;
	char *arg;
} buildin_func[]={
	{buildin_mes,"mes","s"},
	{buildin_next,"next",""},
	{buildin_close,"close",""},
	{buildin_menu,"menu","*"},
	{buildin_goto,"goto","l"},
	{buildin_jobchange,"jobchange","i"},
	{buildin_input,"input",""},
	{buildin_warp,"warp","sii"},
	{buildin_areawarp,"areawarp","siiiisii"},
	{buildin_setlook,"setlook","ii"},
	{buildin_set,"set","ii"},
	{buildin_if,"if","igi"},
	{buildin_getitem,"getitem","si"},	// Modified by RoVeRT
	{buildin_delitem,"delitem","si"},	// Modified by RoVeRT
	{buildin_cutin,"cutin","si"},
	{buildin_viewpoint,"viewpoint","iiiii"},
	{buildin_heal,"heal","ii"},
	{buildin_itemheal,"itemheal","ii"},
	{buildin_percentheal,"percentheal","ii"},
	{buildin_rand,"rand","i*"},
	{buildin_countitem,"countitem","s"},		// Modified by RoVeRT
	{buildin_checkweight,"checkweight","si"},	// Modified by RoVeRT
	{buildin_readparam,"readparam","i"},
	{buildin_getcharid,"getcharid","i"},
	{buildin_getpartyname,"getpartyname","i"},
	{buildin_getguildname,"getguildname","i"},
	{buildin_getguildmaster,"getguildmaster","i"},
	{buildin_strcharinfo,"strcharinfo","i"},
	{buildin_getequipname,"getequipname","i"},
	{buildin_getequipisequiped,"getequipisequiped","i"},
	{buildin_getequipisenableref,"getequipisenableref","i"},
	{buildin_getequipisidentify,"getequipisidentify","i"},
	{buildin_getequiprefinerycnt,"getequiprefinerycnt","i"},
	{buildin_getequipweaponlv,"getequipweaponlv","i"},
	{buildin_getequippercentrefinery,"getequippercentrefinery","i"},
	{buildin_successrefitem,"successrefitem","i"},
	{buildin_failedrefitem,"failedrefitem","i"},
	{buildin_statusup,"statusup","i"},
	{buildin_statusup2,"statusup2","ii"},
	{buildin_bonus,"bonus","ii"},
	{buildin_bonus2,"bonus2","iii"},
	{buildin_bonus3,"bonus3","iiii"},
	{buildin_skill,"skill","ii*"},
	{buildin_getskilllv,"getskilllv","i"},
	{buildin_basicskillcheck,"basicskillcheck","*"},
	{buildin_getgmlevel,"getgmlevel","*"},
	{buildin_end,"end",""},
	{buildin_checkoption,"checkoption","i"},
	{buildin_setoption,"setoption","i"},
	{buildin_setcart,"setcart",""},
	{buildin_setfalcon,"setfalcon",""},
	{buildin_setriding,"setriding",""},
	{buildin_savepoint,"savepoint","sii"},
	{buildin_openstorage,"openstorage",""},
	{buildin_itemskill,"itemskill","iis"},
	{buildin_produce,"produce","i"},
	{buildin_monster,"monster","siisii*"},
	{buildin_areamonster,"areamonster","siiiisii*"},
	{buildin_killmonster,"killmonster","ss"},
	{buildin_doevent,"doevent","s"},
	{buildin_addtimer,"addtimer","is"},
	{buildin_deltimer,"deltimer","s"},
	{buildin_addtimer,"addtimercount","si"},
	{buildin_announce,"announce","si"},
	{buildin_mapannounce,"mapannounce","ssi"},
	{buildin_areaannounce,"areaannounce","siiiisi"},
	{buildin_getusers,"getusers","i"},
	{buildin_getmapusers,"getmapusers","s"},
	{buildin_getareausers,"getareausers","siiii"},
	{buildin_enablenpc,"enablenpc","s"},
	{buildin_disablenpc,"disablenpc","s"},
	{buildin_enablearena,"enablearena",""},		// Added by RoVeRT
	{buildin_disablearena,"disablearena",""},	// Added by RoVeRT
	{buildin_sc_start,"sc_start","iii"},
	{buildin_sc_end,"sc_end","i"},
	{buildin_debugmes,"debugmes","s"},
	{buildin_catchpet,"pet","i"},
	{buildin_birthpet,"bpet",""},
	{buildin_resetstatus,"resetstatus",""},
	{buildin_resetskill,"resetskill",""},
	{buildin_changebase,"changebase","i"},
	{buildin_changesex,"changesex",""},
	{buildin_waitingroom,"waitingroom","si*"},
	{buildin_warpwaitingpc,"warpwaitingpc","sii"},
	{buildin_setmapflag,"setmapflagnosave","ssii"},
	{buildin_setmapflag,"setmapflag","si"},
	{buildin_removemapflag,"removemapflag","si"},
	{buildin_pvpon,"pvpon","s"},
	{buildin_pvpoff,"pvpoff","s"},
	{buildin_gvgon,"gvgon","s"},
	{buildin_gvgoff,"gvgoff","s"},
	{buildin_emotion,"emotion","i"},
	{buildin_maprespawnguildid,"maprespawnguildid","sii"},
	{buildin_agitstart,"agitstart",""},	// <Agit>
	{buildin_agitend,"agitend",""},
	{buildin_getcastlename,"getcastlename","s"},
	{buildin_getcastledata,"getcastledata","si"},
	{buildin_setcastledata,"setcastledata","sii"},
	{buildin_mapwarp,"mapwarp","ssii"},		// Added by RoVeRT
	{buildin_inittimer,"inittimer",""},
	{buildin_stoptimer,"stoptimer",""},
	{buildin_cmdothernpc,"cmdothernpc","ss"},
	{buildin_mobcount,"mobcount","s"},		// End Additions
	{buildin_getequipcardcnt,"getequipcardcnt","i"},
	{buildin_successremovecards,"successremovecards","i"},
	{buildin_failedremovecards,"failedremovecards","ii"},
	{NULL,NULL,NULL},
};
enum {
	C_NOP,C_POS,C_INT,C_PARAM,C_FUNC,C_STR,C_CONSTSTR,C_ARG,C_NAME,C_EOL,

	C_LOR,C_LAND,C_LE,C_LT,C_GE,C_GT,C_EQ,C_NE,   //operator
	C_XOR,C_OR,C_AND,C_ADD,C_SUB,C_MUL,C_DIV,C_MOD,C_NEG,C_LNOT,C_NOT
};

/*==========================================
 *
 *------------------------------------------
 */
static int calc_hash(unsigned char *p)
{
	int h=0;
	while(*p){
		h=(h<<1)+(h>>3)+(h>>5)+(h>>8);
		h+=*p++;
	}
	return h&15;
}

/*==========================================
 *
 *------------------------------------------
 */
// �����̂ł���Δԍ��A�������-1
static int search_str(unsigned char *p)
{
	int i;
	i=str_hash[calc_hash(p)];
	while(i){
		if(strcmp(str_buf+str_data[i].str,p)==0){
			return i;
		}
		i=str_data[i].next;
	}
	return -1;
}

/*==========================================
 *
 *------------------------------------------
 */
// �����̂ł���Δԍ��A������Γo�^���ĐV�K�ԍ�
static int add_str(unsigned char *p)
{
	int i;
	char *lowcase;

	lowcase=strdup(p);
	for(i=0;lowcase[i];i++)
		lowcase[i]=tolower(lowcase[i]);
	if((i=search_str(lowcase))>=0){
		free(lowcase);
		return i;
	}
	free(lowcase);

	i=calc_hash(p);
	if(str_hash[i]==0){
		str_hash[i]=str_num;
	} else {
		i=str_hash[i];
		for(;;){
			if(strcmp(str_buf+str_data[i].str,p)==0){
				return i;
			}
			if(str_data[i].next==0)
				break;
			i=str_data[i].next;
		}
		str_data[i].next=str_num;
	}
	if(str_num>=str_data_size){
		str_data_size+=128;
		str_data=realloc(str_data,sizeof(str_data[0])*str_data_size);
		if(str_data==NULL){
			printf("out of memory : add_str str_data\n");
			exit(1);
		}
	}
	while(str_pos+strlen(p)+1>=str_size){
		str_size+=256;
		str_buf=realloc(str_buf,str_size);
		if(str_buf==NULL){
			printf("out of memory : add_str str_buf\n");
			exit(1);
		}
	}
	strcpy(str_buf+str_pos,p);
	str_data[str_num].type=C_NOP;
	str_data[str_num].str=str_pos;
	str_data[str_num].next=0;
	str_data[str_num].func=NULL;
	str_data[str_num].backpatch=-1;
	str_data[str_num].label=-1;
	str_pos+=strlen(p)+1;
	return str_num++;
}


/*==========================================
 *
 *------------------------------------------
 */
static void check_script_buf(int size)
{
	if(script_pos+size>=script_size){
		script_size+=SCRIPT_BLOCK_SIZE;
		script_buf=realloc(script_buf,script_size);
		if(script_buf==NULL){
			printf("out of memory : check_script_buf \n");
			exit(1);
		}
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static void add_scriptb(int a)
{
	check_script_buf(1);
	script_buf[script_pos++]=a;
}

/*==========================================
 *
 *------------------------------------------
 */
static void add_scriptc(int a)
{
	while(a>=0x40){
		add_scriptb((a&0x3f)|0x40);
		a=(a-0x40)>>6;
	}
	add_scriptb(a&0x3f);
}

/*==========================================
 *
 *------------------------------------------
 */
static void add_scripti(int a)
{
	while(a>=0x40){
		add_scriptb(a|0xc0);
		a=(a-0x40)>>6;
	}
	add_scriptb(a|0x80);
}

/*==========================================
 *
 *------------------------------------------
 */
// �ő�16M�܂�
static void add_scriptl(int l)
{
	int backpatch = str_data[l].backpatch;

	switch(str_data[l].type){
	case C_POS:
		add_scriptc(C_POS);
		add_scriptb(str_data[l].label);
		add_scriptb(str_data[l].label>>8);
		add_scriptb(str_data[l].label>>16);
		break;
	case C_NOP:
		// ���x���̉\��������̂�backpatch�p�f�[�^���ߍ���
		add_scriptc(C_NAME);
		str_data[l].backpatch=script_pos;
		add_scriptb(backpatch);
		add_scriptb(backpatch>>8);
		add_scriptb(backpatch>>16);
		break;
	case C_INT:
		add_scripti(str_data[l].val);
		break;
	default:
		// �������̗p�r�Ɗm�肵�Ă�̂Ő��������̂܂�
		add_scriptc(C_NAME);
		add_scriptb(l);
		add_scriptb(l>>8);
		add_scriptb(l>>16);
		break;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void set_label(int l,int pos)
{
	int i,next;

	str_data[l].type=C_POS;
	str_data[l].label=pos;
	for(i=str_data[l].backpatch;i>=0 && i!=0x00ffffff;){
		next=(*(int*)(script_buf+i)) & 0x00ffffff;
		script_buf[i-1]=C_POS;
		script_buf[i]=pos;
		script_buf[i+1]=pos>>8;
		script_buf[i+2]=pos>>16;
		i=next;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static unsigned char *skip_space(unsigned char *p)
{
	while(1){
		while(isspace(*p))
			p++;
		if(p[0]=='/' && p[1]=='/'){
			while(*p && *p!='\n')
				p++;
		} else if(p[0]=='/' && p[1]=='*'){
			p++;
			while(*p && (p[-1]!='*' || p[0]!='/'))
				p++;
			if(*p) p++;
		} else
			break;
	}
	return p;
}

/*==========================================
 *
 *------------------------------------------
 */
static unsigned char *skip_word(unsigned char *p)
{
	if(*p=='@') p++;	// like weiss
	if(*p=='$') p++;	// MAP�I�����L�ϐ��p
	while(isalnum(*p)||*p=='_'|| *p>=0x81)
		if(*p>=0x81 && p[1]){
			p+=2;
		} else
			p++;
	return p;
}

static unsigned char *startptr;
static int startline;

/*==========================================
 *
 *------------------------------------------
 */
static void disp_error_message(char *mes,unsigned char *pos)
{
	int line,c=0,i;
	unsigned char *p,*linestart,*lineend;

	for(line=startline,p=startptr;p && *p;line++){
		linestart=p;
		lineend=strchr(p,'\n');
		if(lineend){
			c=*lineend;
			*lineend=0;
		}
		if(lineend==NULL || pos<lineend){
			printf("%s line %d : ",mes,line);
			for(i=0;linestart[i];i++){
				if(linestart+i!=pos)
					printf("%c",linestart[i]);
				else
					printf("\'%c\'",linestart[i]);
			}
			printf("\n");
			if(lineend)
				*lineend=c;
			return;
		}
		*lineend=c;
		p=lineend+1;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned char* parse_simpleexpr(unsigned char *p)
{
	int i;
	p=skip_space(p);

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_simpleexpr %s\n",p);
#endif
	if(*p==';' || *p==','){
		disp_error_message("unexpected expr end",p);
		exit(1);
	}
	if(*p=='('){

		p=parse_subexpr(p+1,-1);
		p=skip_space(p);
		if((*p++)!=')'){
			disp_error_message("unmatch ')'",p);
			exit(1);
		}
	} else if(isdigit(*p) || ((*p=='-' || *p=='+') && isdigit(p[1]))){
		char *np;
		i=strtoul(p,&np,0);
		add_scripti(i);
		p=np;
	} else if(*p=='"'){
		add_scriptc(C_STR);
		p++;
		while(*p && *p!='"'){
			if(p[-1]<=0x7e && *p=='\\')
				p++;
			add_scriptb(*p++);
		}
		if(!*p){
			disp_error_message("unexpected eof @ string",p);
			exit(1);
		}
		add_scriptb(0);
		p++;	//'"'
	} else {
		int c;
		// label , register , function etc
		if(skip_word(p)==p){
			disp_error_message("unexpected charactor",p);
			exit(1);
		}
		c=*skip_word(p);
		*skip_word(p)=0;
		add_scriptl(add_str(p));
		*skip_word(p)=c;
		p=skip_word(p);
	}

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_simpleexpr end %s\n",p);
#endif
	return p;
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned char* parse_subexpr(unsigned char *p,int limit)
{
	int op,opl,len;
	char *tmpp;

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_subexpr %s\n",p);
#endif
	p=skip_space(p);

	if(*p=='-'){
		tmpp=skip_space(p+1);
		if(*tmpp==';' || *tmpp==','){
			add_scriptl(LABEL_NEXTLINE);
			p++;
			return p;
		}
	}
	if((op=C_NEG,*p=='-') || (op=C_LNOT,*p=='!') || (op=C_NOT,*p=='~')){
		p=parse_subexpr(p+1,100);
		add_scriptc(op);
	} else
		p=parse_simpleexpr(p);
	p=skip_space(p);
	while(((op=C_ADD,opl=6,len=1,*p=='+') ||
		   (op=C_SUB,opl=6,len=1,*p=='-') ||
		   (op=C_MUL,opl=7,len=1,*p=='*') ||
		   (op=C_DIV,opl=7,len=1,*p=='/') ||
		   (op=C_MOD,opl=7,len=1,*p=='%') ||
		   (op=C_FUNC,opl=8,len=1,*p=='(') ||
		   (op=C_LAND,opl=1,len=2,*p=='&' && p[1]=='&') ||
		   (op=C_AND,opl=5,len=1,*p=='&') ||
		   (op=C_LOR,opl=0,len=2,*p=='|' && p[1]=='|') ||
		   (op=C_OR,opl=4,len=1,*p=='|') ||
		   (op=C_XOR,opl=3,len=1,*p=='^') ||
		   (op=C_EQ,opl=2,len=2,*p=='=' && p[1]=='=') ||
		   (op=C_NE,opl=2,len=2,*p=='!' && p[1]=='=') ||
		   (op=C_GE,opl=2,len=2,*p=='>' && p[1]=='=') ||
		   (op=C_GT,opl=2,len=1,*p=='>') ||
		   (op=C_LE,opl=2,len=2,*p=='<' && p[1]=='=') ||
		   (op=C_LT,opl=2,len=1,*p=='<')) && opl>limit){
		p+=len;
		if(op==C_FUNC){
			add_scriptc(C_ARG);
			do {
				p=parse_subexpr(p,-1);
				p=skip_space(p);
				if(*p==',') p++;
				p=skip_space(p);
			} while(*p && *p!=')');
			if(*(p++)!=')'){
				disp_error_message("func request '(' ')'",p);
				exit(1);
			}
		} else {
			p=parse_subexpr(p,opl);
		}
		add_scriptc(op);
		p=skip_space(p);
	}
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_subexpr end %s\n",p);
#endif
	return p;  /* return first untreated operator */
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned char* parse_expr(unsigned char *p)
{
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_expr %s\n",p);
#endif
	switch(*p){
	case ')': case ';': case ':': case '[': case ']':
	case '}':
		disp_error_message("unexpected char",p);
		exit(1);
	}
	p=parse_subexpr(p,-1);
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_expr end %s\n",p);
#endif
	return p;
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned char* parse_line(unsigned char *p)
{
	p=skip_space(p);
	if(*p==';')
		return p;

	// �ŏ��͊֐���
	p=parse_simpleexpr(p);
	p=skip_space(p);

	add_scriptc(C_ARG);
	while(p && *p && *p!=';'){
		p=parse_expr(p);
		p=skip_space(p);
		// ������؂��,�͗L���Ă������Ă��ǂ�
		if(*p==',') p++;
		p=skip_space(p);
	}
	if(!p || *(p++)!=';'){
		disp_error_message("need ';'",p);
		exit(1);
	}
	add_scriptc(C_FUNC);

	return p;
}

/*==========================================
 *
 *------------------------------------------
 */
static void add_buildin_func(void)
{
	int i,n;
	for(i=0;buildin_func[i].func;i++){
		n=add_str(buildin_func[i].name);
		str_data[n].type=C_FUNC;
		str_data[n].func=buildin_func[i].func;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static void read_constdb(void)
{
	FILE *fp;
	char line[1024],name[1024];
	int val,n,i,type;

	fp=fopen("db/const.txt","r");
	if(fp==NULL){
		printf("can't read db/const.txt\n");
		return ;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		type=0;
		if(sscanf(line,"%[A-Za-z0-9_],%d,%d",name,&val,&type)>=2 ||
		   sscanf(line,"%[A-Za-z0-9_] %d %d",name,&val,&type)>=2){
			for(i=0;name[i];i++)
				name[i]=tolower(name[i]);
			n=add_str(name);
			if(type==0)
				str_data[n].type=C_INT;
			else
				str_data[n].type=C_PARAM;
			str_data[n].val=val;
		}
	}
	fclose(fp);
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned char* parse_script(unsigned char *src,int line)
{
	unsigned char *p,*tmpp;
	int i;
	static int first=1;

	if(first){
		add_buildin_func();
		read_constdb();
	}
	first=0;
	script_buf=malloc(SCRIPT_BLOCK_SIZE);
	if(script_buf==NULL){
		printf("out of memory : parse_script\n");
		exit(1);
	}
	script_pos=0;
	script_size=SCRIPT_BLOCK_SIZE;
	str_data[LABEL_NEXTLINE].type=C_NOP;
	str_data[LABEL_NEXTLINE].backpatch=-1;
	str_data[LABEL_NEXTLINE].label=-1;
	for(i=LABEL_START;i<str_num;i++){
		if(str_data[i].type==C_POS || str_data[i].type==C_NAME){
			str_data[i].type=C_NOP;
			str_data[i].backpatch=-1;
			str_data[i].label=-1;
		}
	}

	// �O���plabel db�̏�����
	if(scriptlabel_db!=NULL)
		strdb_final(scriptlabel_db,scriptlabel_final);
	scriptlabel_db=strdb_init(50);

	// for error message
	startptr = src;
	startline = line;

	p=src;
	p=skip_space(p);
	if(*p!='{'){
		disp_error_message("not found '{'",p);
		return NULL;
	}
	for(p++;p && *p && *p!='}';){
		p=skip_space(p);
		// label�������ꏈ��
		tmpp=skip_space(skip_word(p));
		if(*tmpp==':'){
			int l,c;

			c=*skip_word(p);
			*skip_word(p)=0;
			l=add_str(p);
			if(str_data[l].label!=-1){
				*skip_word(p)=c;
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);
			strdb_insert(scriptlabel_db,p,script_pos);	// �O���plabel db�o�^
			*skip_word(p)=c;
			p=tmpp+1;
			continue;
		}

		// ���͑S���ꏏ����
		p=parse_line(p);
		p=skip_space(p);
		add_scriptc(C_EOL);

		set_label(LABEL_NEXTLINE,script_pos);
		str_data[LABEL_NEXTLINE].type=C_NOP;
		str_data[LABEL_NEXTLINE].backpatch=-1;
		str_data[LABEL_NEXTLINE].label=-1;
	}

	add_scriptc(C_NOP);

	script_size = script_pos;
	script_buf=realloc(script_buf,script_pos);
	if(script_buf==NULL){
		printf("out of momory : parse_script realloc\n");
		exit(1);
	}

	for(i=LABEL_START;i<str_num;i++){
		if(str_data[i].type==C_NOP){
			int j,next;
			str_data[i].type=C_NAME;
			str_data[i].label=i;
			for(j=str_data[i].backpatch;j>=0 && j!=0x00ffffff;){
				next=(*(int*)(script_buf+j)) & 0x00ffffff;
				script_buf[j]=i;
				script_buf[j+1]=i>>8;
				script_buf[j+2]=i>>16;
				j=next;
			}
		}
	}

#ifdef DEBUG_DISP
	for(i=0;i<script_pos;i++){
		if((i&15)==0) printf("%04x : ",i);
		printf("%02x ",script_buf[i]);
		if((i&15)==15) printf("\n");
	}
	printf("\n");
#endif

	return script_buf;
}

//
// ���s�n
//
enum {STOP=1,END,RERUNLINE,GOTO};

/*==========================================
 *
 *------------------------------------------
 */
// �ϐ��̃f�[�^�E���̂�
int get_val(struct script_state*st,struct script_data* data)
{
	struct map_session_data *sd;
	if(data->type==C_NAME){
		sd=map_id2sd(st->rid);
		data->type=C_INT;
		if(str_data[data->u.num].type==C_INT){
			data->u.num = str_data[data->u.num].val;
		}else if(str_data[data->u.num].type==C_PARAM){
			data->u.num = pc_readparam(sd,str_data[data->u.num].val);
		}else if(str_buf[str_data[data->u.num].str]=='@' || str_buf[str_data[data->u.num].str]=='l'){
			data->u.num = pc_readreg(sd,data->u.num);
		}else if(str_buf[str_data[data->u.num].str]=='$'){
			data->u.num = (int)strdb_search(mapval_db,str_buf+str_data[data->u.num].str);
		}else{
			data->u.num = pc_readglobalreg(sd,str_buf+str_data[data->u.num].str);
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
char* conv_str(struct script_state *st,struct script_data *data)
{
	get_val(st,data);
	if(data->type==C_INT){
		char *buf;
		buf=malloc(16);
		if(buf==NULL){
			printf("out of memory : conv_str\n");
			exit(1);
		}
		sprintf(buf,"%d",data->u.num);
		data->type=C_STR;
		data->u.str=buf;
#if 1
	} else if(data->type==C_NAME){
		// �e���|�����B�{�������͂�
		data->type=C_CONSTSTR;
		data->u.str=str_buf+str_data[data->u.num].str;
#endif
	}
	return data->u.str;
}

/*==========================================
 *
 *------------------------------------------
 */
int conv_num(struct script_state *st,struct script_data *data)
{
	char *p;
	get_val(st,data);
	if(data->type==C_STR || data->type==C_CONSTSTR){
		p=data->u.str;
		data->u.num = atoi(p);
		if(data->type==C_STR)
			free(p);
		data->type=C_INT;
	}
	return data->u.num;
}

/*==========================================
 *
 *------------------------------------------
 */
void push_val(struct script_stack *stack,int type,int val)
{
	if(stack->sp>=stack->sp_max){
		stack->sp_max+=64;
		stack->stack_data=realloc(stack->stack_data,sizeof(stack->stack_data[0])*stack->sp_max);
		if(stack->stack_data==NULL){
			printf("push_val:stack over flow\n");
			exit(1);
		}
	}
//	if(battle_config.etc_log)
//		printf("push (%d,%d)-> %d\n",type,val,stack->sp);
	stack->stack_data[stack->sp].type=type;
	stack->stack_data[stack->sp].u.num=val;
	stack->sp++;
}

/*==========================================
 *
 *------------------------------------------
 */
void push_str(struct script_stack *stack,int type,unsigned char *str)
{
	if(stack->sp>=stack->sp_max){
		stack->sp_max+=64;
		stack->stack_data=realloc(stack->stack_data,sizeof(stack->stack_data[0])*stack->sp_max);
		if(stack->stack_data==NULL){
			printf("push_val:stack over flow\n");
			exit(1);
		}
	}
//	if(battle_config.etc_log)
//		printf("push (%d,%x)-> %d\n",type,str,stack->sp);
	stack->stack_data[stack->sp].type=type;
	stack->stack_data[stack->sp].u.str=str;
	stack->sp++;
}

/*==========================================
 *
 *------------------------------------------
 */
void push_copy(struct script_stack *stack,int pos)
{
	switch(stack->stack_data[pos].type){
	case C_CONSTSTR:
		push_str(stack,C_CONSTSTR,stack->stack_data[pos].u.str);
		break;
	case C_STR:
		push_str(stack,C_STR,strdup(stack->stack_data[pos].u.str));
		break;
	default:
		push_val(stack,stack->stack_data[pos].type,stack->stack_data[pos].u.num);
		break;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void pop_stack(struct script_stack* stack,int start,int end)
{
	int i;
	for(i=start;i<end;i++){
		if(stack->stack_data[i].type==C_STR){
			free(stack->stack_data[i].u.str);
		}
	}
	if(stack->sp>end){
		memmove(&stack->stack_data[start],&stack->stack_data[end],sizeof(stack->stack_data[0])*(stack->sp-end));
	}
	stack->sp-=end-start;
}

//
// ���ߍ��݊֐�
//
/*==========================================
 *
 *------------------------------------------
 */
int buildin_mes(struct script_state *st)
{
	conv_str(st,& (st->stack->stack_data[st->start+2]));
	clif_scriptmes(map_id2sd(st->rid),st->oid,st->stack->stack_data[st->start+2].u.str);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_goto(struct script_state *st)
{
	int pos;

	pos=conv_num(st,& (st->stack->stack_data[st->start+2]));
	st->pos=pos;
	st->state=GOTO;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_next(struct script_state *st)
{
	st->state=STOP;
	clif_scriptnext(map_id2sd(st->rid),st->oid);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_close(struct script_state *st)
{
	st->state=END;
	clif_scriptclose(map_id2sd(st->rid),st->oid);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_menu(struct script_state *st)
{
	char *buf;
	int len,i;
	struct map_session_data *sd;

	sd=map_id2sd(st->rid);

	if(sd->state.menu_or_input==0){
		st->state=RERUNLINE;
		sd->state.menu_or_input=1;
		for(i=st->start+2,len=16;i<st->end;i+=2){
			conv_str(st,& (st->stack->stack_data[i]));
			len+=strlen(st->stack->stack_data[i].u.str)+1;
		}
		buf=malloc(len);
		if(buf==NULL){
			printf("out of memory : buildin_menu\n");
			exit(1);
		}
		buf[0]=0;
		for(i=st->start+2,len=0;i<st->end;i+=2){
			strcat(buf,st->stack->stack_data[i].u.str);
			strcat(buf,":");
		}
		clif_scriptmenu(map_id2sd(st->rid),st->oid,buf);
		free(buf);
	} else if(sd->npc_menu==0xff){	// cansel
		sd->state.menu_or_input=0;
		st->state=END;
	} else {	// goto����
		// ragemu�݊��̂���
		pc_setreg(sd,add_str("l15"),sd->npc_menu);
		sd->state.menu_or_input=0;
		if(sd->npc_menu>0 && sd->npc_menu<(st->end-st->start)/2){
			int pos;
			pos=conv_num(st,& (st->stack->stack_data[st->start+sd->npc_menu*2+1]));
			st->pos=pos;
			st->state=GOTO;
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_rand(struct script_state *st)
{
	int range,min,max;

	if(st->end>st->start+3){
		min=conv_num(st,& (st->stack->stack_data[st->start+2]));
		max=conv_num(st,& (st->stack->stack_data[st->start+3]));
		if(max<min){
			int tmp;
			tmp=min;
			min=max;
			max=tmp;
		}
		range=max-min+1;
		push_val(st->stack,C_INT,rand()%range+min);
	} else {
		range=conv_num(st,& (st->stack->stack_data[st->start+2]));
		push_val(st->stack,C_INT,rand()%range);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_warp(struct script_state *st)
{
	int x,y;
	char *str;
	struct map_session_data *sd=map_id2sd(st->rid);

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));
	if(strcmp(str,"Random")==0)
		pc_randomwarp(sd,3);
	else if(strcmp(str,"SavePoint")==0){
		if(map[sd->bl.m].flag.noteleport)	// �e���|�֎~
			return 0;

		pc_setpos(sd,sd->status.save_point.map,
			sd->status.save_point.x,sd->status.save_point.y,3);
	}else
		pc_setpos(sd,str,x,y,0);
	return 0;
}
/*==========================================
 * �G���A�w�胏�[�v
 *------------------------------------------
 */
int buildin_areawarp_sub(struct block_list *bl,va_list ap)
{
	int x,y;
	char *map;
	map=va_arg(ap, char *);
	x=va_arg(ap,int);
	y=va_arg(ap,int);
	pc_setpos((struct map_session_data *)bl,map,x,y,0);
	return 0;
}
int buildin_areawarp(struct script_state *st)
{
	int x,y,m;
	char *str;
	char *mapname;
	int x0,y0,x1,y1;

	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	str=conv_str(st,& (st->stack->stack_data[st->start+7]));
	x=conv_num(st,& (st->stack->stack_data[st->start+8]));
	y=conv_num(st,& (st->stack->stack_data[st->start+9]));
	
	if( (m=map_mapname2mapid(mapname))< 0)
		return 0;
	
	map_foreachinarea(buildin_areawarp_sub,
		m,x0,y0,x1,y1,BL_PC,	str,x,y );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_heal(struct script_state *st)
{
	int hp,sp;

	hp=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sp=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_heal(map_id2sd(st->rid),hp,sp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_itemheal(struct script_state *st)
{
	int hp,sp;

	hp=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sp=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_itemheal(map_id2sd(st->rid),hp,sp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_percentheal(struct script_state *st)
{
	int hp,sp;

	hp=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sp=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_percentheal(map_id2sd(st->rid),hp,sp);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_jobchange(struct script_state *st)
{
	int job;

	job=conv_num(st,& (st->stack->stack_data[st->start+2]));
	pc_jobchange(map_id2sd(st->rid),job);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_input(struct script_state *st)
{
	struct map_session_data *sd;

	sd=map_id2sd(st->rid);
	if(sd->state.menu_or_input){
		sd->state.menu_or_input=0;
		if(st->end>st->start+2){ // ����1��
			int num=st->stack->stack_data[st->start+2].u.num;
			if (sd->npc_amount<0) sd->npc_amount=0;	// Added by RoVeRT
			if(str_buf[str_data[num].str]=='@' || str_buf[str_data[num].str]=='l')
				pc_setreg(sd,num,sd->npc_amount);
			else if(str_buf[str_data[num].str]=='$')
				strdb_insert(mapval_db,str_buf+str_data[num].str,sd->npc_amount);
			else
				pc_setglobalreg(sd,str_buf+str_data[num].str,sd->npc_amount);
		} else {
			// ragemu�݊��̂���
			pc_setreg(sd,add_str("l14"),sd->npc_amount);
		}
	} else {
		st->state=RERUNLINE;
		clif_scriptinput(map_id2sd(st->rid),st->oid);
		sd->state.menu_or_input=1;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_if(struct script_state *st)
{
	int sel,i;

	sel=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(!sel)
		return 0;

	// �֐������R�s�[
	push_copy(st->stack,st->start+3);
	// �ԂɈ����}�[�J������
	push_val(st->stack,C_ARG,0);
	// �c��̈������R�s�[
	for(i=st->start+4;i<st->end;i++){
		push_copy(st->stack,i);
	}
	run_func(st);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_set(struct script_state *st)
{
	int val;
	struct map_session_data *sd;
	int num=st->stack->stack_data[st->start+2].u.num;

	val=conv_num(st,& (st->stack->stack_data[st->start+3]));
	sd=map_id2sd(st->rid);
	if(str_data[num].type==C_PARAM){
		pc_setparam(sd,str_data[num].val,val);
	}else if(str_buf[str_data[num].str]=='@'
	       ||str_buf[str_data[num].str]=='l') {
		pc_setreg(sd,num,val);
	}else if(str_buf[str_data[num].str]=='$') {
		strdb_insert(mapval_db,str_buf+str_data[num].str,val);
	}else{
		pc_setglobalreg(sd,str_buf+str_data[num].str,val);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_setlook(struct script_state *st)
{
	int type,val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val=conv_num(st,& (st->stack->stack_data[st->start+3]));

	pc_changelook(map_id2sd(st->rid),type,val);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_cutin(struct script_state *st)
{
	int type;

	conv_str(st,& (st->stack->stack_data[st->start+2]));
	type=conv_num(st,& (st->stack->stack_data[st->start+3]));

	clif_cutin(map_id2sd(st->rid),st->stack->stack_data[st->start+2].u.str,type);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_viewpoint(struct script_state *st)
{
	int type,x,y,id,color;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));
	id=conv_num(st,& (st->stack->stack_data[st->start+5]));
	color=conv_num(st,& (st->stack->stack_data[st->start+6]));

	clif_viewpoint(map_id2sd(st->rid),st->oid,type,x,y,id,color);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_countitem(struct script_state *st)		// Modified by RoVeRT
{
	int count,i,nameid;
	char *id;
	struct map_session_data *sd;
	struct item_data *item_data;

	id=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if( (nameid=atoi(id))>0 )
		nameid=((item_data=itemdb_exists(nameid))?nameid:0);
	else if( (item_data=itemdb_searchname(id))!=NULL )
		nameid=item_data->nameid;

	sd=map_id2sd(st->rid);
	for(i=0,count=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid)
			count+=sd->status.inventory[i].amount;
	}

	push_val(st->stack,C_INT,count);

	return 0;
}

/*==========================================
 * �d�ʃ`�F�b�N
 *------------------------------------------
 */
int buildin_checkweight(struct script_state *st)	// Modified by RoVeRT
{
	int nameid,amount;
	struct map_session_data *sd;
	char *id;
	struct item_data *item_data;

	id=conv_str(st,& (st->stack->stack_data[st->start+2]));
	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));

	if( (nameid=atoi(id))>0 )
		nameid=((item_data=itemdb_exists(nameid))?nameid:0);
	else if( (item_data=itemdb_searchname(id))!=NULL )
		nameid=item_data->nameid;

	sd=map_id2sd(st->rid);
	if(itemdb_weight(nameid)*amount + sd->weight > sd->max_weight){
		push_val(st->stack,C_INT,0);
	} else {
		push_val(st->stack,C_INT,1);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_getitem(struct script_state *st)	// Modified by RoVeRT
{
	int nameid,amount,flag = 0;
	struct item item_tmp;
	struct map_session_data *sd;
	char *id;
	struct item_data *item_data;

	sd = map_id2sd(st->rid);
	id=conv_str(st,& (st->stack->stack_data[st->start+2]));
	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));

	if( (nameid=atoi(id))>0 )
		nameid=((item_data=itemdb_exists(nameid))?nameid:0);
	else if( (item_data=itemdb_searchname(id))!=NULL )
		nameid=item_data->nameid;

	if(nameid<0) { // �����_��
		nameid=itemdb_searchrandomid(-nameid);
		flag = 1;
	}

	if(nameid > 0) {
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=1;
		else
			item_tmp.identify=!itemdb_isequip(nameid);
		if((flag = pc_additem(sd,&item_tmp,amount))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&item_tmp,amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_delitem(struct script_state *st)	// Modified by RoVeRT
{
	int nameid,amount,i;
	struct map_session_data *sd;
	char *id;
	struct item_data *item_data;

	id=conv_str(st,& (st->stack->stack_data[st->start+2]));
	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));

	if( (nameid=atoi(id))>0 )
		nameid=((item_data=itemdb_exists(nameid))?nameid:0);
	else if( (item_data=itemdb_searchname(id))!=NULL )
		nameid=item_data->nameid;

	sd=map_id2sd(st->rid);
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid){
			if(sd->status.inventory[i].amount>amount){
				pc_delitem(sd,i,amount,0);
				break;
			} else {
				amount-=sd->status.inventory[i].amount;
				pc_delitem(sd,i,sd->status.inventory[i].amount,0);
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_readparam(struct script_state *st)
{
	int type;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	push_val(st->stack,C_INT,pc_readparam(sd,type));

	return 0;
}
/*==========================================
 *�L�����֌W��ID�擾
 *------------------------------------------
 */
int buildin_getcharid(struct script_state *st)
{
	int num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	if(num==1)
		push_val(st->stack,C_INT,sd->status.party_id);
	if(num==2)
		push_val(st->stack,C_INT,sd->status.guild_id);
	return 0;
}
/*==========================================
 *�w��ID��PT���擾
 *------------------------------------------
 */
char *buildin_getpartyname_sub(int party_id)
{
	struct party *p;
	
	p=NULL;		
	p=party_search(party_id);
	
	if(p!=NULL){
		char *buf;
		buf=malloc(24);
		if(buf==NULL){
			if(battle_config.error_log)
				printf("out of memory : buildin_getguildname_sub\n");
			exit(1);
		}
		strcpy(buf,p->name);
		return buf;
	}

	return 0;
}
int buildin_getpartyname(struct script_state *st)
{
	char *name;
	int party_id;
	
	party_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	name=buildin_getpartyname_sub(party_id);
	if(name!=0)
		push_str(st->stack,C_STR,name);
	else
		push_str(st->stack,C_STR,"null");

	return 0;
}
/*==========================================
 *�w��ID�̃M���h���擾
 *------------------------------------------
 */
char *buildin_getguildname_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		char *buf;
		buf=malloc(24);
		if(buf==NULL){
			if(battle_config.error_log)
				printf("out of memory : buildin_getguildname_sub\n");
			exit(1);
		}
		strcpy(buf,g->name);
		return buf;
	}

	return 0;
}
int buildin_getguildname(struct script_state *st)
{
	char *name;
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	name=buildin_getguildname_sub(guild_id);
	if(name!=0)
		push_str(st->stack,C_STR,name);
	else
		push_str(st->stack,C_STR,"null");
	return 0;
}

/*==========================================
 *�w��ID��GuildMaster���擾
 *------------------------------------------
 */
char *buildin_getguildmaster_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		char *buf;
		buf=malloc(24);
		if(buf==NULL){
			if(battle_config.error_log)
				printf("out of memory : buildin_getguildmaster_sub\n");
			exit(1);
		}
		strcpy(buf,g->master);
		return buf;
	}

	return 0;
}
int buildin_getguildmaster(struct script_state *st)
{
	char *master;
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0)
		push_str(st->stack,C_STR,master);
	else
		push_str(st->stack,C_STR,"null");
	return 0;
}

/*==========================================
 * �L�����N�^�̖��O
 *------------------------------------------
 */
int buildin_strcharinfo(struct script_state *st)
{
	struct map_session_data *sd;
	int num;

	sd=map_id2sd(st->rid);
	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(num==0){
		char *buf;
		buf=malloc(24);
		if(buf==NULL){
			printf("out of memory : buildin_strcharinfo\n");
			exit(1);
		}
		strcpy(buf,sd->status.name);
		push_str(st->stack,C_STR,buf);
	}
	if(num==1){
		char *buf;
		buf=buildin_getpartyname_sub(sd->status.party_id);
		if(buf!=0)
			push_str(st->stack,C_STR,buf);
	}
	if(num==2){
		char *buf;
		buf=buildin_getguildname_sub(sd->status.guild_id);
		if(buf!=0)
			push_str(st->stack,C_STR,buf);
	}

	return 0;
}

unsigned int equip[10]={0x0100,0x0010,0x0020,0x0002,0x0004,0x0040,0x0008,0x0080,0x0200,0x0001};

/*==========================================
 * ������������i���B���j���[�p�j
 *------------------------------------------
 */
int buildin_getequipname(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;
	struct item_data* item;
	char *buf;

	buf=malloc(64);
	if(buf==NULL){
		printf("out of memory : buildin_getequipname\n");
		exit(1);
	}
	sd=map_id2sd(st->rid);
	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0){
		item=sd->inventory_data[i];
		if(item)
			sprintf(buf,"%s-[%s]",pos[num-1],item->jname);
		else
			sprintf(buf,"%s-[%s]",pos[num-1],pos[10]);
	}else{
		sprintf(buf,"%s-[%s]",pos[num-1],pos[10]);
	}
	push_str(st->stack,C_STR,buf);

	return 0;
}

/*==========================================
 * �����`�F�b�N
 *------------------------------------------
 */
int buildin_getequipisequiped(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0){
		push_val(st->stack,C_INT,1);
	}else{
		push_val(st->stack,C_INT,0);
	}

	return 0;
}

/*==========================================
 * �����i���B�\�`�F�b�N
 *------------------------------------------
 */
int buildin_getequipisenableref(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0 && num<7 && sd->inventory_data[i] && (num!=1 || sd->inventory_data[i]->def > 1
	             || (sd->inventory_data[i]->def==1 && sd->inventory_data[i]->equip_script==NULL)
	             || (sd->inventory_data[i]->def<=0 && sd->inventory_data[i]->equip_script!=NULL))
	   ){
		push_val(st->stack,C_INT,1);
	}else{
		push_val(st->stack,C_INT,0);
	}

	return 0;
}

/*==========================================
 * �����i�Ӓ�`�F�b�N
 *------------------------------------------
 */
int buildin_getequipisidentify(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		push_val(st->stack,C_INT,sd->status.inventory[i].identify);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * �����i���B�x
 *------------------------------------------
 */
int buildin_getequiprefinerycnt(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		push_val(st->stack,C_INT,sd->status.inventory[i].refine);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * �����i����LV
 *------------------------------------------
 */
int buildin_getequipweaponlv(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0 && sd->inventory_data[i])
		push_val(st->stack,C_INT,sd->inventory_data[i]->wlv);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * �����i���B������
 *------------------------------------------
 */
int buildin_getequippercentrefinery(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		push_val(st->stack,C_INT,pc_percentrefinery(sd,&sd->status.inventory[i]));
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * ���B����
 *------------------------------------------
 */
int buildin_successrefitem(struct script_state *st)
{
	int i,num,ep;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0) {
		ep=sd->status.inventory[i].equip;

		sd->status.inventory[i].refine++;
		pc_unequipitem(sd,i,0);
		clif_refine(sd->fd,sd,0,i,sd->status.inventory[i].refine);
		clif_delitem(sd,i,1);
		clif_additem(sd,i,1,0);
		pc_equipitem(sd,i,ep);
		clif_misceffect(&sd->bl,3);
	}

	return 0;
}

/*==========================================
 * ���B���s
 *------------------------------------------
 */
int buildin_failedrefitem(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0) {
		pc_unequipitem(sd,i,0);
		// ���B���s�G�t�F�N�g�̃p�P�b�g
		clif_refine(sd->fd,sd,1,i,sd->status.inventory[i].refine);
		pc_delitem(sd,i,1,0);
		// ���̐l�ɂ����s��ʒm
		clif_misceffect(&sd->bl,2);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_statusup(struct script_state *st)
{
	int type;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	pc_statusup(sd,type);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_statusup2(struct script_state *st)
{
	int type,val;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val=conv_num(st,& (st->stack->stack_data[st->start+3]));
	sd=map_id2sd(st->rid);
	pc_statusup2(sd,type,val);

	return 0;
}
/*==========================================
 * �����i�ɂ��\�͒l�{�[�i�X
 *------------------------------------------
 */
int buildin_bonus(struct script_state *st)
{
	int type,val;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val=conv_num(st,& (st->stack->stack_data[st->start+3]));
	sd=map_id2sd(st->rid);
	pc_bonus(sd,type,val);

	return 0;
}
/*==========================================
 * �����i�ɂ��\�͒l�{�[�i�X
 *------------------------------------------
 */
int buildin_bonus2(struct script_state *st)
{
	int type,type2,val;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	type2=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val=conv_num(st,& (st->stack->stack_data[st->start+4]));
	sd=map_id2sd(st->rid);
	pc_bonus2(sd,type,type2,val);

	return 0;
}
/*==========================================
 * �����i�ɂ��\�͒l�{�[�i�X
 *------------------------------------------
 */
int buildin_bonus3(struct script_state *st)
{
	int type,type2,type3,val;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	type2=conv_num(st,& (st->stack->stack_data[st->start+3]));
	type3=conv_num(st,& (st->stack->stack_data[st->start+4]));
	val=conv_num(st,& (st->stack->stack_data[st->start+5]));
	sd=map_id2sd(st->rid);
	pc_bonus3(sd,type,type2,type3,val);

	return 0;
}
/*==========================================
 * �X�L������
 *------------------------------------------
 */
int buildin_skill(struct script_state *st)
{
	int id,level,flag=1;
	struct map_session_data *sd;

	id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	level=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if( st->end>st->start+4 )
		flag=conv_num(st,&(st->stack->stack_data[st->start+4]) );
	sd=map_id2sd(st->rid);
	pc_skill(sd,id,level,flag);

	return 0;
}
/*==========================================
 * �X�L�����x������
 *------------------------------------------
 */
int buildin_getskilllv(struct script_state *st)
{
	int id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	push_val(st->stack,C_INT, pc_checkskill( map_id2sd(st->rid) ,id) );
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_basicskillcheck(struct script_state *st)
{
	push_val(st->stack,C_INT, battle_config.basic_skill_check);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_getgmlevel(struct script_state *st)
{
	push_val(st->stack,C_INT, pc_isGM(map_id2sd(st->rid)));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_end(struct script_state *st)
{
	st->state = END;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_checkoption(struct script_state *st)
{
	int type;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);

	if(sd->status.option & type){
		push_val(st->stack,C_INT,1);
	} else {
		push_val(st->stack,C_INT,0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_setoption(struct script_state *st)
{
	int type;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	pc_setoption(sd,type);

	return 0;
}

/*==========================================
 * �J�[�g��t����
 *------------------------------------------
 */
int buildin_setcart(struct script_state *st)
{
	struct map_session_data *sd;

	sd=map_id2sd(st->rid);
	pc_setcart(sd,1);

	return 0;
}

/*==========================================
 * ���t����
 *------------------------------------------
 */
int buildin_setfalcon(struct script_state *st)
{
	struct map_session_data *sd;

	sd=map_id2sd(st->rid);
	pc_setfalcon(sd);

	return 0;
}

/*==========================================
 * �y�R�y�R���
 *------------------------------------------
 */
int buildin_setriding(struct script_state *st)
{
	struct map_session_data *sd;

	sd=map_id2sd(st->rid);
	pc_setriding(sd);

	return 0;
}

/*==========================================
 *	�Z�[�u�|�C���g�̕ۑ�
 *------------------------------------------
 */
int buildin_savepoint(struct script_state *st)
{
	int x,y;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));
	pc_setsavepoint(map_id2sd(st->rid),str,x,y);
	return 0;
}

/*==========================================
 * �J�v���q�ɂ��J��
 *------------------------------------------
 */
int buildin_openstorage(struct script_state *st)
{
	storage_storageopen(map_id2sd(st->rid));
	return 0;
}
/*==========================================
 * �A�C�e���ɂ��X�L������
 *------------------------------------------
 */
int buildin_itemskill(struct script_state *st)
{
	int id,lv;
	char *str;
	struct map_session_data *sd=map_id2sd(st->rid);

	id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	lv=conv_num(st,& (st->stack->stack_data[st->start+3]));
	str=conv_str(st,& (st->stack->stack_data[st->start+4]));
	
	// �r�����ɃX�L���A�C�e���͎g�p�ł��Ȃ�
	if(sd->skilltimer != -1)
		return 0;

	sd->skillitem=id;
	sd->skillitemlv=lv;
	clif_item_skill(sd,id,lv,str);
	return 0;
}
/*==========================================
 * �A�C�e���쐬
 *------------------------------------------
 */
int buildin_produce(struct script_state *st)
{
	int trigger;
	struct map_session_data *sd=map_id2sd(st->rid);
	trigger=conv_num(st,& (st->stack->stack_data[st->start+2]));
	clif_skill_produce_mix_list(sd,trigger);
	return 0;
}

/*==========================================
 * �����X�^�[����
 *------------------------------------------
 */
int buildin_monster(struct script_state *st)
{
	int class,amount,x,y;
	char *str,*map,*event="",*type;
	struct map_session_data *sd=map_id2sd(st->rid);

	map	=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x	=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y	=conv_num(st,& (st->stack->stack_data[st->start+4]));
	str	=conv_str(st,& (st->stack->stack_data[st->start+5]));
	type=conv_str(st,& (st->stack->stack_data[st->start+6]));
	amount=conv_num(st,& (st->stack->stack_data[st->start+7]));

	if( (class=atoi((char*)type))==0 )
		class=mobdb_searchname(type);

	if( st->end>st->start+8 )
		event=conv_str(st,& (st->stack->stack_data[st->start+8]));

	mob_once_spawn(sd,map,x,y,str,class,amount,event);
	return 0;
}
/*==========================================
 * �����X�^�[����
 *------------------------------------------
 */
int buildin_areamonster(struct script_state *st)
{
	int class,amount,x0,y0,x1,y1;
	char *str,*map,*event="";
	struct map_session_data *sd=map_id2sd(st->rid);

	map	=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0	=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0	=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1	=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1	=conv_num(st,& (st->stack->stack_data[st->start+6]));
	str	=conv_str(st,& (st->stack->stack_data[st->start+7]));
	class=conv_num(st,& (st->stack->stack_data[st->start+8]));
	amount=conv_num(st,& (st->stack->stack_data[st->start+9]));
	if( st->end>st->start+10 )
		event=conv_str(st,& (st->stack->stack_data[st->start+10]));

	mob_once_spawn_area(sd,map,x0,y0,x1,y1,str,class,amount,event);
	return 0;
}
/*==========================================
 * �����X�^�[�폜
 *------------------------------------------
 */
int buildin_killmonster_sub(struct block_list *bl,va_list ap)
{
	char *event=va_arg(ap,char *);
	if(strcmp(event,((struct mob_data *)bl)->npc_event)==0)
		mob_delete((struct mob_data *)bl);
	return 0;
}
int buildin_killmonster(struct script_state *st)
{
	char *mapname,*event;
	int m;
	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	event=conv_str(st,& (st->stack->stack_data[st->start+3]));

	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
	map_foreachinarea(buildin_killmonster_sub,
		m,0,0,map[m].xs,map[m].ys,BL_MOB, event );
	return 0;
}

/*==========================================
 * �C�x���g���s
 *------------------------------------------
 */
int buildin_doevent(struct script_state *st)
{
	char *event;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	npc_event(map_id2sd(st->rid),event,0);
	return 0;
}
/*==========================================
 * �C�x���g�^�C�}�[�ǉ�
 *------------------------------------------
 */
int buildin_addtimer(struct script_state *st)
{
	char *event;
	int tick;
	tick=conv_num(st,& (st->stack->stack_data[st->start+2]));
	event=conv_str(st,& (st->stack->stack_data[st->start+3]));
	pc_addeventtimer(map_id2sd(st->rid),tick,event);
	return 0;
}
/*==========================================
 * �C�x���g�^�C�}�[�폜
 *------------------------------------------
 */
int buildin_deltimer(struct script_state *st)
{
	char *event;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	pc_deleventtimer(map_id2sd(st->rid),event);
	return 0;
}
/*==========================================
 * �C�x���g�^�C�}�[�̃J�E���g�l�ǉ�
 *------------------------------------------
 */
int buildin_addtimercount(struct script_state *st)
{
	char *event;
	int tick;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_addeventtimercount(map_id2sd(st->rid),event,tick);
	return 0;
}

/*==========================================
 * �V�̐��A�i�E���X
 *------------------------------------------
 */
int buildin_announce(struct script_state *st)
{
	char *str;
	int flag;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+3]));

	if(flag&0x0f){
		struct block_list *bl=map_id2bl((flag&0x08)?st->oid:st->rid);
		clif_GMmessage(bl,str,strlen(str)+1,flag);
	}else
		intif_GMmessage(str,strlen(str)+1,flag);
	return 0;
}
/*==========================================
 * �V�̐��A�i�E���X�i����}�b�v�j
 *------------------------------------------
 */
int buildin_mapannounce_sub(struct block_list *bl,va_list ap)
{
	char *str;
	int len,flag;
	str=va_arg(ap,char *);
	len=va_arg(ap,int);
	flag=va_arg(ap,int);
	clif_GMmessage(bl,str,len,flag|3);
	return 0;
}
int buildin_mapannounce(struct script_state *st)
{
	char *mapname,*str;
	int flag,m;
	
	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	str=conv_str(st,& (st->stack->stack_data[st->start+3]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+4]));

	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
	map_foreachinarea(buildin_mapannounce_sub,
		m,0,0,map[m].xs,map[m].ys,BL_PC, str,strlen(str)+1,flag&0x10);
	return 0;
}
/*==========================================
 * �V�̐��A�i�E���X�i����G���A�j
 *------------------------------------------
 */
int buildin_areaannounce(struct script_state *st)
{
	char *map,*str;
	int flag,m;
	int x0,y0,x1,y1;
	
	map=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	str=conv_str(st,& (st->stack->stack_data[st->start+7]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+8]));

	if( (m=map_mapname2mapid(map))<0 )
		return 0;
	
	map_foreachinarea(buildin_mapannounce_sub,
		m,x0,y0,x1,y1,BL_PC, str,strlen(str)+1,flag&0x10 );
	return 0;
}
/*==========================================
 * ���[�U�[������
 *------------------------------------------
 */
int buildin_getusers(struct script_state *st)
{
	int flag=conv_num(st,& (st->stack->stack_data[st->start+2]));
	struct block_list *bl=map_id2bl((flag&0x08)?st->oid:st->rid);
	int val=0;
	switch(flag&0x07){
	case 0:	val=map[bl->m].users; break;
	case 1: val=map_getusers(); break;
	}
	push_val(st->stack,C_INT,val);
	return 0;
}
/*==========================================
 * �}�b�v�w�胆�[�U�[������
 *------------------------------------------
 */
int buildin_getmapusers(struct script_state *st)
{
	char *str;
	int m;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	if( (m=map_mapname2mapid(str))< 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	push_val(st->stack,C_INT,map[m].users);
	return 0;
}
/*==========================================
 * �G���A�w�胆�[�U�[������
 *------------------------------------------
 */
int buildin_getareausers_sub(struct block_list *bl,va_list ap)
{
	int *users=va_arg(ap,int *);
	(*users)++;
	return 0;
}
int buildin_getareausers(struct script_state *st)
{
	char *str;
	int m,x0,y0,x1,y1,users=0;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	if( (m=map_mapname2mapid(str))< 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_getareausers_sub,
		m,x0,y0,x1,y1,BL_PC,&users);
	push_val(st->stack,C_INT,users);
	return 0;
}

/*==========================================
 * NPC�̗L����
 *------------------------------------------
 */
int buildin_enablenpc(struct script_state *st)
{
	char *str;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	npc_enable(str,1);
	return 0;
}
/*==========================================
 * NPC�̖�����
 *------------------------------------------
 */
int buildin_disablenpc(struct script_state *st)
{
	char *str;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	npc_enable(str,0);
	return 0;
}

int buildin_enablearena(struct script_state *st)	// Added by RoVeRT
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
	struct chat_data *cd;


	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL)
		return 0;

	npc_enable(nd->name,1);
	nd->arenaflag=1;

	if(cd->users>=cd->trigger && cd->npc_event[0])
		npc_timer_event(cd->npc_event);

	return 0;
}
int buildin_disablearena(struct script_state *st)	// Added by RoVeRT
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
	nd->arenaflag=0;

	return 0;
}

/*==========================================
 * ��Ԉُ�ɂ�����
 *------------------------------------------
 */
int buildin_sc_start(struct script_state *st)
{
	int type,tick,val1;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val1=conv_num(st,& (st->stack->stack_data[st->start+4]));
	skill_status_change_start(map_id2bl(st->rid),type,val1,0,tick,0);
	return 0;
}

/*==========================================
 * ��Ԉُ킪����
 *------------------------------------------
 */
int buildin_sc_end(struct script_state *st)
{
	int type;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	skill_status_change_end(map_id2bl(st->rid),type,-1);
//	if(battle_config.etc_log)
//		printf("sc_end : %d %d\n",st->rid,type);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_debugmes(struct script_state *st)
{
	conv_str(st,& (st->stack->stack_data[st->start+2]));
	printf("script debug : %d %d : %s\n",st->rid,st->oid,st->stack->stack_data[st->start+2].u.str);
	return 0;
}

/*==========================================
 *�ߊl�A�C�e���g�p
 *------------------------------------------
 */
int buildin_catchpet(struct script_state *st)
{
	int pet_id;
	struct map_session_data *sd;
	pet_id= conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	pet_catch_process1(sd,pet_id);
	return 0;
}

/*==========================================
 *�g�ї��z���@�g�p
 *------------------------------------------
 */
int buildin_birthpet(struct script_state *st)
{
	struct map_session_data *sd;
	sd=map_id2sd(st->rid);
	clif_sendegg(sd);
	return 0;
}

/*==========================================
 * �X�e�[�^�X���Z�b�g
 *------------------------------------------
 */
int buildin_resetstatus(struct script_state *st)
{
	struct map_session_data *sd;
	sd=map_id2sd(st->rid);
	pc_resetstate(sd);
	return 0;
}

/*==========================================
 * �X�L�����Z�b�g
 *------------------------------------------
 */
int buildin_resetskill(struct script_state *st)
{
	struct map_session_data *sd;
	sd=map_id2sd(st->rid);
	pc_resetskill(sd);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_changebase(struct script_state *st)
{
	struct map_session_data *sd;
	int vclass;
	sd=map_id2sd(st->rid);
	vclass = conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(vclass == 22 && !battle_config.wedding_modifydisplay)
		return 0;
	
	if(vclass==22)
				pc_unequipitem(sd,sd->equip_index[9],1);	// �����O��
		
	sd->view_class = vclass;
	
	return 0;
}

/*==========================================
 * ���ʕϊ�
 *------------------------------------------
 */
int buildin_changesex(struct script_state *st)
{
	struct map_session_data *sd=NULL;
	sd=map_id2sd(st->rid);

	if(sd->status.sex==0){
		sd->status.sex=1;
		sd->sex=1;
		if(sd->status.class == 20)
			sd->status.class = 19;
		chrif_changesex(sd->status.account_id,1);
	}else if(sd->status.sex==1){
		sd->status.sex=0;
		sd->sex=0;
		if(sd->status.class == 19)
			sd->status.class = 20;
		chrif_changesex(sd->status.account_id,0);

	}
	chrif_save(sd);
	return 0;
}


/*==========================================
 * npc�`���b�g�쐬
 *------------------------------------------
 */
int buildin_waitingroom(struct script_state *st)	// Modified by RoVeRT
{
	char *name,*ev="";
	int limit,trigger;
	name=conv_str(st,& (st->stack->stack_data[st->start+2]));
	limit= trigger= conv_num(st,& (st->stack->stack_data[st->start+3]));
	if( st->end>st->start+4 )
		trigger=conv_num(st,& (st->stack->stack_data[st->start+4]));
	if( st->end>st->start+5 )
		ev=conv_str(st,& (st->stack->stack_data[st->start+5]));
	chat_createcnpchat( (struct npc_data *)map_id2bl(st->oid),
		limit,trigger,name,strlen(name)+1,ev);
	return 0;
}
/*==========================================
 * �`���b�g�����o�[�S�����[�v
 *------------------------------------------
 */
int buildin_warpwaitingpc(struct script_state *st)	// Modified by RoVeRT
{
	int x,y,i;
	char *str;
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
	struct chat_data *cd;

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL || !nd->arenaflag )
		return 0;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));

	for(i=0;i<cd->trigger;i++){
		struct map_session_data *sd=cd->usersd[i];
	
		if(strcmp(str,"Random")==0)
			pc_randomwarp(sd,3);
		else if(strcmp(str,"SavePoint")==0){
			if(map[sd->bl.m].flag.noteleport)	// �e���|�֎~
				return 0;
	
			pc_setpos(sd,sd->status.save_point.map,
				sd->status.save_point.x,sd->status.save_point.y,3);
		}else
			pc_setpos(sd,str,x,y,0);
	}
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
enum { MF_NOMEMO,MF_NOTELEPORT,MF_NOSAVE,MF_NOBRANCH,MF_NOPENALTY,MF_PVP,MF_PVP_NOPARTY,MF_PVP_NOGUILD,MF_GVG,MF_GVG_NOPARTY };

int buildin_setmapflagnosave(struct script_state *st)
{
	int m,x,y;
	char *str,*str2;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	str2=conv_str(st,& (st->stack->stack_data[st->start+3]));
	x=conv_num(st,& (st->stack->stack_data[st->start+4]));
	y=conv_num(st,& (st->stack->stack_data[st->start+5]));
	m = map_mapname2mapid(str);
	if(m >= 0) {
		map[m].flag.nosave=1;
		memcpy(map[m].save.map,str2,16);
		map[m].save.x=x;
		map[m].save.y=y;
	}

	return 0;
}

int buildin_setmapflag(struct script_state *st)
{
	int m,i;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	i=conv_num(st,& (st->stack->stack_data[st->start+3]));
	m = map_mapname2mapid(str);
	if(m >= 0) {
		switch(i) {
			case MF_NOMEMO:
				map[m].flag.nomemo=1;
				break;
			case MF_NOTELEPORT:
				map[m].flag.noteleport=1;
				break;
			case MF_NOBRANCH:
				map[m].flag.nobranch=1;
				break;
			case MF_NOPENALTY:
				map[m].flag.nopenalty=1;
				break;
			case MF_PVP_NOPARTY:
				map[m].flag.pvp_noparty=1;
				break;
			case MF_PVP_NOGUILD:
				map[m].flag.pvp_noguild=1;
				break;
			case MF_GVG_NOPARTY:
				map[m].flag.gvg_noparty=1;
				break;
		}
	}

	return 0;
}

int buildin_removemapflag(struct script_state *st)
{
	int m,i;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	i=conv_num(st,& (st->stack->stack_data[st->start+3]));
	m = map_mapname2mapid(str);
	if(m >= 0) {
		switch(i) {
			case MF_NOMEMO:
				map[m].flag.nomemo=0;
				break;
			case MF_NOTELEPORT:
				map[m].flag.noteleport=0;
				break;
			case MF_NOSAVE:
				map[m].flag.nosave=0;
				break;
			case MF_NOBRANCH:
				map[m].flag.nobranch=0;
				break;
			case MF_NOPENALTY:
				map[m].flag.nopenalty=0;
				break;
			case MF_PVP_NOPARTY:
				map[m].flag.pvp_noparty=0;
				break;
			case MF_PVP_NOGUILD:
				map[m].flag.pvp_noguild=0;
				break;
			case MF_GVG_NOPARTY:
				map[m].flag.gvg_noparty=0;
				break;
		}
	}

	return 0;
}

int buildin_pvpon(struct script_state *st)
{
	int m,i;
	char *str;
	struct map_session_data *pl_sd=NULL;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && !map[m].flag.pvp) {
		map[m].flag.pvp = 1;
		clif_send0199(m,1);
		for(i=0;i<fd_max;i++){	//�l�������[�v
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(m == pl_sd->bl.m && pl_sd->pvp_timer == -1) {
					pl_sd->pvp_timer=add_timer(gettick()+200,pc_calc_pvprank_timer,pl_sd->bl.id,0);
					pl_sd->pvp_rank=0;
					pl_sd->pvp_lastusers=0;
					pl_sd->pvp_point=5;
				}
			}
		}
	}

	return 0;
}

int buildin_pvpoff(struct script_state *st)
{
	int m,i;
	char *str;
	struct map_session_data *pl_sd=NULL;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && map[m].flag.pvp) {
		map[m].flag.pvp = 0;
		clif_send0199(m,0);
		for(i=0;i<fd_max;i++){	//�l�������[�v
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(m == pl_sd->bl.m) {
					clif_pvpset(pl_sd,0,0,2);
					if(pl_sd->pvp_timer != -1) {
						delete_timer(pl_sd->pvp_timer,pc_calc_pvprank_timer);
						pl_sd->pvp_timer = -1;
					}
				}
			}
		}
	}

	return 0;
}

int buildin_gvgon(struct script_state *st)
{
	int m;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && !map[m].flag.gvg) {
		map[m].flag.gvg = 1;
		clif_send0199(m,3);
	}

	return 0;
}
int buildin_gvgoff(struct script_state *st)
{
	int m;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && map[m].flag.gvg) {
		map[m].flag.gvg = 0;
		clif_send0199(m,0);
	}

	return 0;
}
/*==========================================
 *	NPC�G���[�V����
 *------------------------------------------
 */

int buildin_emotion(struct script_state *st)
{
	int type;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(type < 0 || type > 100)
		return 0;
	clif_emotion(map_id2bl(st->oid),type);
	return 0;
}

int buildin_maprespawnguildid_sub(struct block_list *bl,va_list ap)
{
	int g_id=va_arg(ap,int);
	int flag=va_arg(ap,int);
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;

	if(bl->type == BL_PC)
		sd=(struct map_session_data*)bl;
	if(bl->type == BL_MOB)
		md=(struct mob_data *)bl;

	if(sd){
		if((sd->status.guild_id == g_id) && (flag&1))
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
		else if((sd->status.guild_id != g_id) && (flag&2))
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	}
	if(md && flag&4){
		if(md->class < 1285 || md->class > 1288)
			mob_delete(md);
	}
	return 0;
}
int buildin_maprespawnguildid(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	int g_id=conv_num(st,& (st->stack->stack_data[st->start+3]));
	int flag=conv_num(st,& (st->stack->stack_data[st->start+4]));

	int m=map_mapname2mapid(mapname);

	if(m) map_foreachinarea(buildin_maprespawnguildid_sub,m,0,0,map[m].xs-1,map[m].ys-1,BL_NUL,g_id,flag);
	return 0;
}

int buildin_agitstart(struct script_state *st)
{
	if(agit_flag==1) return 1;      // Agit already Start.
	agit_flag=1;
	guild_agit_start();
	return 0;
}

int buildin_agitend(struct script_state *st)
{
	if(agit_flag==0) return 1;      // Agit already End.
	agit_flag=0;
	guild_agit_end();
	return 0;
}

int buildin_getcastlename(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	struct guild_castle *gc;
	int i;
	char *buf=malloc(24);
	if(buf==NULL){
		if(battle_config.error_log)
			printf("out of memory : buildin_getcastlename\n");
		exit(1);
	}
	strcpy(buf,"");
	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				strcpy(buf,gc->castle_name);
				break;
			}
		}
	}
	push_str(st->stack,C_STR,buf);
	return 0;
}

int buildin_getcastledata(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	int index=conv_num(st,& (st->stack->stack_data[st->start+3]));
	struct guild_castle *gc;
	int i,j;

	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				switch(index){
				case 0: for(j=1;j<18;j++) intif_guild_castle_dataload(gc->castle_id,j); break;  // Initialize[AgitInit]
				case 1: push_val(st->stack,C_INT,gc->guild_id); break;
				case 2: push_val(st->stack,C_INT,gc->economy); break;
				case 3: push_val(st->stack,C_INT,gc->defense); break;
				case 4: push_val(st->stack,C_INT,gc->triggerE); break;
				case 5: push_val(st->stack,C_INT,gc->triggerD); break;
				case 6: push_val(st->stack,C_INT,gc->nextTime); break;
				case 7: push_val(st->stack,C_INT,gc->payTime); break;
				case 8: push_val(st->stack,C_INT,gc->createTime); break;
				case 9: push_val(st->stack,C_INT,gc->visibleC); break;
				case 10: push_val(st->stack,C_INT,gc->visibleG0); break;
				case 11: push_val(st->stack,C_INT,gc->visibleG1); break;
				case 12: push_val(st->stack,C_INT,gc->visibleG2); break;
				case 13: push_val(st->stack,C_INT,gc->visibleG3); break;
				case 14: push_val(st->stack,C_INT,gc->visibleG4); break;
				case 15: push_val(st->stack,C_INT,gc->visibleG5); break;
				case 16: push_val(st->stack,C_INT,gc->visibleG6); break;
				case 17: push_val(st->stack,C_INT,gc->visibleG7); break;
				default:
					push_val(st->stack,C_INT,0); break;
				}
				return 0;
			}
		}
	}
	push_val(st->stack,C_INT,0);
	return 0;
}

int buildin_setcastledata(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	int index=conv_num(st,& (st->stack->stack_data[st->start+3]));
	int value=conv_num(st,& (st->stack->stack_data[st->start+4]));
	struct guild_castle *gc;
	int i;

	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				// Save Data byself First
				switch(index){
				case 1: gc->guild_id = value; break;
				case 2: gc->economy = value; break;
				case 3: gc->defense = value; break;
				case 4: gc->triggerE = value; break;
				case 5: gc->triggerD = value; break;
				case 6: gc->nextTime = value; break;
				case 7: gc->payTime = value; break;
				case 8: gc->createTime = value; break;
				case 9: gc->visibleC = value; break;
				case 10: gc->visibleG0 = value; break;
				case 11: gc->visibleG1 = value; break;
				case 12: gc->visibleG2 = value; break;
				case 13: gc->visibleG3 = value; break;
				case 14: gc->visibleG4 = value; break;
				case 15: gc->visibleG5 = value; break;
				case 16: gc->visibleG6 = value; break;
				case 17: gc->visibleG7 = value; break;
				default: return 0;
				}
				intif_guild_castle_datasave(gc->castle_id,index,value);
				return 0;
			}
		}
	}
	return 0;
}

int buildin_mapwarp(struct script_state *st)	// Added by RoVeRT
{
	int x,y,m;
	char *str;
	char *mapname;
	int x0,y0,x1,y1;

	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=0;
	y0=0;
	x1=map[map_mapname2mapid(mapname)].xs;
	y1=map[map_mapname2mapid(mapname)].ys;
	str=conv_str(st,& (st->stack->stack_data[st->start+3]));
	x=conv_num(st,& (st->stack->stack_data[st->start+4]));
	y=conv_num(st,& (st->stack->stack_data[st->start+5]));
	
	if( (m=map_mapname2mapid(mapname))< 0)
		return 0;
	
	map_foreachinarea(buildin_areawarp_sub,
		m,x0,y0,x1,y1,BL_PC,	str,x,y );
	return 0;
}


/*int buildin_npctimer(struct script_state *st)	// Added by RoVeRT
{
	int option;
	option=conv_num(st,& (st->stack->stack_data[st->start+2]));
	npc_do_ontimer(st->oid, map_id2sd(st->rid), option);
	return 0;
}*/


int buildin_inittimer(struct script_state *st)	// Added by RoVeRT
{
//	struct npc_data *nd=(struct npc_data*)map_id2bl(st->oid);

//	nd->lastaction=nd->timer=gettick();
	npc_do_ontimer(st->oid, map_id2sd(st->rid), 1);

	return 0;
}

int buildin_stoptimer(struct script_state *st)	// Added by RoVeRT
{
//	struct npc_data *nd=(struct npc_data*)map_id2bl(st->oid);

//	nd->lastaction=nd->timer=-1;
	npc_do_ontimer(st->oid, map_id2sd(st->rid), 0);

	return 0;
}

int buildin_cmdothernpc(struct script_state *st)	// Added by RoVeRT
{
	char *npc,*command;

	npc=conv_str(st,& (st->stack->stack_data[st->start+2]));
	command=conv_str(st,& (st->stack->stack_data[st->start+3]));

	npc_command(map_id2sd(st->rid),npc,command);
	return 0;
}

int buildin_mobcount_sub(struct block_list *bl,va_list ap)	// Added by RoVeRT
{
	char *event=va_arg(ap,char *);
	int *c=va_arg(ap,int *);

	if(strcmp(event,((struct mob_data *)bl)->npc_event)==0)
		(*c)++;
	return 0;
}

int buildin_mobcount(struct script_state *st)	// Added by RoVeRT
{
	char *mapname,*event;
	int m,c=0;
	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	event=conv_str(st,& (st->stack->stack_data[st->start+3]));

	if( (m=map_mapname2mapid(mapname))<0 ) {
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_mobcount_sub,
		m,0,0,map[m].xs,map[m].ys,BL_MOB, event,&c );

	push_val(st->stack,C_INT, (c - 1));

	return 0;
}

/* =====================================================================
 * NPC command checks number of cards compounded on equipped items
 * Usage: getequipcardcnt(x)  returns number of cards on equipped item x
 * Returns 0 if item has no cards or is a blacksmith-made item.
 * ---------------------------------------------------------------------
 */

int buildin_getequipcardcnt(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(sd->status.inventory[i].card[0] == 0x00ff){		/* If item was made by a Blacksmith, exit */
		push_val(st->stack,C_INT,0);
		return 0;
	}
	int c=4;
	do{
		if((sd->status.inventory[i].card[c-1] > 4000) && (sd->status.inventory[i].card[c-1] < 5000)){
			push_val(st->stack,C_INT,(c));
			return 0;
		}
	}while(c--);
	push_val(st->stack,C_INT,0);
	return 0;
}

/* ================================================================
 * Card removal success command. Cards are removed and given to PC.
 * ----------------------------------------------------------------
 */

// Added by TyrNemesis^
int buildin_successremovecards(struct script_state *st)
{
	int i,num,cardflag=0,flag;
	struct map_session_data *sd;
	struct item item_tmp;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(sd->status.inventory[i].card[0]==0x00ff){	/* If item was made by a Blacksmith, exit */
		return 0;
	}
	int c=4;
	do{	/* Check if the item HAS any cards */
		if((sd->status.inventory[i].card[c-1] > 4000) && (sd->status.inventory[i].card[c-1] < 5000)){ 

			cardflag = 1;	/* This tells the code that at least one card was found and awarded */
				item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c-1];
				item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
				item_tmp.attribute=0;
				item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;

			if((flag=pc_additem(sd,&item_tmp,1))){	/* If there is a card in the slot, award it. */
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
	}while(c--);

	if(cardflag == 1){	/* If any cards were awarded, clear the item and return it. */
		flag=0;
		item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
		item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
		item_tmp.attribute=sd->status.inventory[i].attribute;
		item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
		pc_delitem(sd,i,1,0);
		if((flag=pc_additem(sd,&item_tmp,1))){
			clif_additem(sd,0,0,flag);
			map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
		clif_misceffect(&sd->bl,3);
		return 0;
	}
	return 0;
}

/* ================================================================
 * Card removal failure command. Parm1 = equip slot, Parm2 = Type
 * 0: Lose item and cards. 1: Lose cards. 2: Lose item. 3: No loss.
 * ----------------------------------------------------------------
 */

// Added by TyrNemesis^
int buildin_failedremovecards(struct script_state *st)
{
	int i,num,cardflag=0,flag,typefail;
	struct map_session_data *sd;
	struct item item_tmp;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	typefail=conv_num(st,& (st->stack->stack_data[st->start+3]));
	sd=map_id2sd(st->rid);
	i=pc_checkequip(sd,equip[num-1]);
	if(sd->status.inventory[i].card[0]==0x00ff){	/* If item was made by a Blacksmith, exit */
		return 0;
	}
	int c=4;
	do{	/* Check if the item HAS any cards */
		if((sd->status.inventory[i].card[c-1] > 4000) && (sd->status.inventory[i].card[c-1] < 5000)){
			cardflag = 1; /* This tells the code that at least one card was found */

			if(typefail == 2){	/* If destroying item only, award cards before item is destroyed */
				item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c-1];
				item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
				item_tmp.attribute=0;
				item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
				if((flag=pc_additem(sd,&item_tmp,1))){
					clif_additem(sd,0,0,flag);
					map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
				}
			}
		}
	}while(c--);

	if(cardflag == 1){	/* If item had any cards, perform required failure actions */

		if(typefail == 0 || typefail == 2){	/* Delete the item */
			pc_delitem(sd,i,1,0);
			clif_misceffect(&sd->bl,2);
			return 0;
		}
		if(typefail == 1){	/* Strip the cards and return the item */
			flag=0;
			item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
			item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
			item_tmp.attribute=sd->status.inventory[i].attribute;
			item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
			pc_delitem(sd,i,1,0);
			if((flag=pc_additem(sd,&item_tmp,1))){
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		clif_misceffect(&sd->bl,2);
		return 0;
	}
	return 0;
}

//
// ���s��main
//
/*==========================================
 *
 *------------------------------------------
 */
static int unget_com_data=-1;
int get_com(unsigned char *script,int *pos)
{
	int i,j;
	if(unget_com_data>=0){
		i=unget_com_data;
		unget_com_data=-1;
		return i;
	}
	if(script[*pos]>=0x80){
		return C_INT;
	}
	i=0; j=0;
	while(script[*pos]>=0x40){
		i=script[(*pos)++]<<j;
		j+=6;
	}
	return i+(script[(*pos)++]<<j);
}

/*==========================================
 *
 *------------------------------------------
 */
void unget_com(int c)
{
	if(unget_com_data!=-1){
		if(battle_config.error_log)
			printf("unget_com can back only 1 data\n");
	}
	unget_com_data=c;
}

/*==========================================
 *
 *------------------------------------------
 */
int get_num(unsigned char *script,int *pos)
{
	int i,j;
	i=0; j=0;
	while(script[*pos]>=0xc0){
		i+=(script[(*pos)++]&0x7f)<<j;
		j+=6;
	}
	return i+((script[(*pos)++]&0x7f)<<j);
}

/*==========================================
 *
 *------------------------------------------
 */
int pop_val(struct script_state* st)
{
	if(st->stack->sp<=0)
		return 0;
	st->stack->sp--;
	get_val(st,&(st->stack->stack_data[st->stack->sp]));
	if(st->stack->stack_data[st->stack->sp].type==C_INT)
		return st->stack->stack_data[st->stack->sp].u.num;
	return 0;
}

#define isstr(c) ((c).type==C_STR || (c).type==C_CONSTSTR)

/*==========================================
 *
 *------------------------------------------
 */
void op_add(struct script_state* st)
{
	st->stack->sp--;
	get_val(st,&(st->stack->stack_data[st->stack->sp]));
	get_val(st,&(st->stack->stack_data[st->stack->sp-1]));

	if(isstr(st->stack->stack_data[st->stack->sp]) || isstr(st->stack->stack_data[st->stack->sp-1])){
		conv_str(st,&(st->stack->stack_data[st->stack->sp]));
		conv_str(st,&(st->stack->stack_data[st->stack->sp-1]));
	}
	if(st->stack->stack_data[st->stack->sp].type==C_INT){ // ii
		st->stack->stack_data[st->stack->sp-1].u.num += st->stack->stack_data[st->stack->sp].u.num;
	} else { // ss�̗\��
		char *buf;
		buf=malloc(strlen(st->stack->stack_data[st->stack->sp-1].u.str)+
				strlen(st->stack->stack_data[st->stack->sp].u.str)+1);
		if(buf==NULL){
			printf("out of memory : op_add\n");
			exit(1);
		}
		strcpy(buf,st->stack->stack_data[st->stack->sp-1].u.str);
		strcat(buf,st->stack->stack_data[st->stack->sp].u.str);
		if(st->stack->stack_data[st->stack->sp-1].type==C_STR)
			free(st->stack->stack_data[st->stack->sp-1].u.str);
		if(st->stack->stack_data[st->stack->sp].type==C_STR)
			free(st->stack->stack_data[st->stack->sp].u.str);
		st->stack->stack_data[st->stack->sp-1].type=C_STR;
		st->stack->stack_data[st->stack->sp-1].u.str=buf;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void op_2num(struct script_state *st,int op)
{
	int i1,i2;
	i2=pop_val(st);
	i1=pop_val(st);
	switch(op){
	case C_SUB:
		i1-=i2;
		break;
	case C_MUL:
		i1*=i2;
		break;
	case C_DIV:
		i1/=i2;
		break;
	case C_MOD:
		i1%=i2;
		break;
	case C_AND:
		i1&=i2;
		break;
	case C_OR:
		i1|=i2;
		break;
	case C_XOR:
		i1^=i2;
		break;
	case C_LAND:
		i1=i1&&i2;
		break;
	case C_LOR:
		i1=i1||i2;
		break;
	case C_EQ:
		i1=i1==i2;
		break;
	case C_NE:
		i1=i1!=i2;
		break;
	case C_GT:
		i1=i1>i2;
		break;
	case C_GE:
		i1=i1>=i2;
		break;
	case C_LT:
		i1=i1<i2;
		break;
	case C_LE:
		i1=i1<=i2;
		break;
	}
	push_val(st->stack,C_INT,i1);
}

/*==========================================
 *
 *------------------------------------------
 */
void op_1num(struct script_state *st,int op)
{
	int i1;
	i1=pop_val(st);
	switch(op){
	case C_NEG:
		i1=-i1;
		break;
	case C_NOT:
		i1=~i1;
		break;
	case C_LNOT:
		i1=!i1;
		break;
	}
	push_val(st->stack,C_INT,i1);
}

/*==========================================
 *
 *------------------------------------------
 */
int run_func(struct script_state *st)
{
	int i,start_sp,end_sp,func;

	end_sp=st->stack->sp;
	for(i=end_sp-1;i>=0 && st->stack->stack_data[i].type!=C_ARG;i--);
	if(i==0){
		if(battle_config.error_log)
			printf("function not found\n");
		st->stack->sp=0;
		st->state=END;
		return 0;
	}
	start_sp=i-1;
	st->start=i-1;
	st->end=end_sp;

	func=st->stack->stack_data[st->start].u.num;
#ifdef DEBUG_RUN
	if(battle_config.etc_log) {
		printf("run_func : %s? (%d(%d))\n",str_buf+str_data[func].str,func,str_data[func].type);
		printf("stack dump :");
		for(i=0;i<end_sp;i++){
			switch(st->stack->stack_data[i].type){
			case C_INT:
				printf(" int(%d)",st->stack->stack_data[i].u.num);
				break;
			case C_NAME:
				printf(" name(%s)",str_buf+str_data[st->stack->stack_data[i].u.num].str);
				break;
			case C_ARG:
				printf(" arg");
				break;
			case C_POS:
				printf(" pos(%d)",st->stack->stack_data[i].u.num);
				break;
			default:
				printf(" %d,%d",st->stack->stack_data[i].type,st->stack->stack_data[i].u.num);
			}
		}
		printf("\n");
	}
#endif
	if(str_data[func].func){
		str_data[func].func(st);
	} else {
		if(battle_config.error_log)
			printf("run_func : %s? (%d(%d))\n",str_buf+str_data[func].str,func,str_data[func].type);
		push_val(st->stack,C_INT,0);
	}

	pop_stack(st->stack,start_sp,end_sp);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int run_script(unsigned char *script,int pos,int rid,int oid)
{
	struct script_stack stack;
	int c;
	struct script_state st;
	int rerun_pos;

	if(script==NULL || pos<0)
		return -1;

	stack.sp=0;
	stack.sp_max=64;
	stack.stack_data=malloc(sizeof(stack.stack_data[0])*stack.sp_max);
	if(stack.stack_data==NULL){
		printf("out of memory : run_script\n");
		exit(1);
	}

	st.stack=&stack;
	st.pos=pos;
	st.rid=rid;
	st.oid=oid;
	rerun_pos=st.pos;
	for(st.state=0;st.state==0;){
		switch(c=get_com(script,&st.pos)){
		case C_EOL:
			if(stack.sp){
				if(battle_config.error_log)
					printf("stack.sp !=0\n");
				stack.sp=0;
			}
			rerun_pos=st.pos;
			break;
		case C_INT:
			push_val(&stack,C_INT,get_num(script,&st.pos));
			break;
		case C_POS:
		case C_NAME:
			push_val(&stack,c,(*(int*)(script+st.pos))&0xffffff);
			st.pos+=3;
			break;
		case C_ARG:
			push_val(&stack,c,0);
			break;
		case C_STR:
			push_str(&stack,C_CONSTSTR,script+st.pos);
			while(script[st.pos++]);
			break;
		case C_FUNC:
			run_func(&st);
			if(st.state==GOTO){
				rerun_pos=st.pos;
				st.state=0;
			}
			break;

		case C_ADD:
			op_add(&st);
			break;

		case C_SUB:
		case C_MUL:
		case C_DIV:
		case C_MOD:
		case C_EQ:
		case C_NE:
		case C_GT:
		case C_GE:
		case C_LT:
		case C_LE:
		case C_AND:
		case C_OR:
		case C_XOR:
		case C_LAND:
		case C_LOR:
			op_2num(&st,c);
			break;

		case C_NEG:
		case C_NOT:
		case C_LNOT:
			op_1num(&st,c);
			break;

		case C_NOP:
			st.state=END;
			break;

		default:
			if(battle_config.error_log)
				printf("unknown command : %d @ %d\n",c,pos);
			st.state=END;
			break;
		}
	}
	switch(st.state){
	case STOP:
		break;
	case END:
		{
			struct map_session_data *sd=map_id2sd(st.rid);
			st.pos=-1;
			if(sd && sd->npc_id==st.oid)
				npc_event_dequeue(sd);
		}
		break;
	case RERUNLINE:
		st.pos=rerun_pos;
		break;
	}

	free(stack.stack_data);
	stack.stack_data=NULL;
	return st.pos;
}

/*==========================================
 * 
 *------------------------------------------
 */
static int set_posword(char *p)
{
	char* np,* str[15];
	int i=0;
	for(i=0;i<11;i++) {
		if((np=strchr(p,','))!=NULL) {
			str[i]=p;
			*np=0;
			p=np+1;
		} else {
			str[i]=p;
			p+=strlen(p);
		}
		if(str[i])
			strcpy(pos[i],str[i]);
	}
	return 0;
}

int script_config_read(char *cfgName)
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
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if(i!=2)
			continue;
		if(strcmpi(w1,"refine_posword")==0) {
			set_posword(w2);
		}
	}
	fclose(fp);

	return 0;
}

/*==========================================
 * ������
 *------------------------------------------
 */
int do_init_script()
{
	mapval_db=strdb_init(32);
	scriptlabel_db=strdb_init(50);
	return 0;
}
