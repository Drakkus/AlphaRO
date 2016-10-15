#ifndef _MAP_H_
#define _MAP_H_

#include <stdarg.h>
#include "mmo.h"

#define MAX_PC_CLASS (1+6+6+1+6+1+1+1+1)
#define MAX_NPC_PER_MAP 512
#define BLOCK_SIZE 8
#define AREA_SIZE 20
#define LOCAL_REG_NUM 16
#define LIFETIME_FLOORITEM 60
#define DAMAGELOG_SIZE 30
#define LOOTITEM_SIZE 10
#define MAX_SKILL_LEVEL 100
#define MAX_STATUSCHANGE 192
#define MAX_SKILLUNITGROUP	32
#define MAX_MOBSKILLUNITGROUP	8
#define MAX_SKILLUNITGROUPTICKSET	128
#define MAX_SKILLTIMERSKILL 32
#define MAX_MOBSKILLTIMERSKILL 10
#define MAX_MOBSKILL	24
#define MAX_EVENTQUEUE	2
#define MAX_EVENTTIMER	32
#define NATURAL_HEAL_INTERVAL 500
#define MAX_FLOORITEM 262144
#define MAX_LEVEL 255

#define DEFAULT_AUTOSAVE_INTERVAL 60*1000

#define MAP_CONF_NAME	"conf/map_alpha.conf"

enum { BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL , BL_PET };
enum { WARP, SHOP, SCRIPT, MONS };
struct block_list {
	struct block_list *next,*prev;
	int id;
	short m,x,y;
	unsigned char type;
	unsigned char subtype;
};

struct walkpath_data {
	unsigned char path_len,path_pos,path_half;
	unsigned char path[32];
};
struct script_reg {
	int index;
	int data;
};
struct status_change {
	int timer;
	int val1,val2,val3,val4;
};
struct vending {
	short index;
	short amount;
	int value;
};

struct skill_unit_group;
struct skill_unit {
	struct block_list bl;

	struct skill_unit_group *group;

	int limit;
	int val1,val2;
	short alive,range;
};
struct skill_unit_group {
	int src_id;
	int party_id;
	int guild_id;
	int map,range;
	int target_flag;
	unsigned int tick;
	int limit,interval;

	int skill_id,skill_lv;
	int val1,val2;
	char *valstr;
	int unit_id;
	int group_id;
	int unit_count,alive_count;
	struct skill_unit *unit;
};
struct skill_unit_group_tickset {
	unsigned int tick;
	int group_id;
};
struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x,y;
	short skill_id,skill_lv;
	int type;
	int flag;
};


struct npc_data;
struct pet_db;
struct item_data;
struct square;

struct map_session_data {
	struct block_list bl;
	struct {
		unsigned auth : 1 ;
		unsigned change_walk_target : 1 ;
		unsigned attack_continue : 1 ;
		unsigned menu_or_input : 1;
		unsigned dead_sit : 2;
		unsigned skillcastcancel : 1;
		unsigned waitingdisconnect : 1;
		unsigned lr_flag : 2;
		unsigned connect_new : 1;
		unsigned arrow_atk : 1;
		unsigned attack_type : 3;
		unsigned skill_flag : 1;
		unsigned gangsterparadise : 1;
	} state;
	struct {
		unsigned restart_full_recover : 1;
		unsigned no_castcancel : 1;
		unsigned no_castcancel2 : 1;
		unsigned no_sizefix : 1;
		unsigned no_magic_damage : 1;
		unsigned no_weapon_damage : 1;
		unsigned no_gemstone : 1;
		unsigned infinite_endure : 1;
	} special_state;
	int char_id,login_id1,login_id2,sex;
	struct mmo_charstatus status;
	struct item_data *inventory_data[MAX_INVENTORY];
	short equip_index[11];
	int weight,max_weight;
	int cart_weight,cart_max_weight,cart_num,cart_max_num;
	char mapname[24];
	int fd,new_fd;
	short to_x,to_y;
	short speed,prev_speed;
	short opt1,opt2;
	char dir,head_dir;
	unsigned int client_tick,server_tick;
	struct walkpath_data walkpath;
	int walktimer;
	int npc_id,npc_shopid;
	int npc_pos;
	int npc_menu;
	int npc_amount;
	unsigned int chatID;

	int attacktimer;
	int attacktarget;
	unsigned int attackabletime;

	short attackrange,attackrange_;
	int skilltimer;
	int skilltarget;
	short skillx,skilly;
	short skillid,skilllv;
	short skillitem,skillitemlv;
	short skillid_old,skilllv_old;
	struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
	short sg_count,fw_count;

	int invincible_timer;
	unsigned int canact_tick;
	unsigned int canmove_tick;
	unsigned int canlog_tick;
	int hp_sub,sp_sub;
	int inchealhptick,inchealsptick,inchealspirittick;

	short view_class;
	short weapontype1,weapontype2;
	int paramb[6],paramc[6],parame[6],paramcard[6];
	int hit,flee,flee2,aspd,amotion,dmotion;
	int watk,watk2,atkmods[3];
	int def,def2,mdef,mdef2,critical,matk1,matk2;
	int atk_ele,def_ele,star,overrefine;
	int castrate,hprate,sprate,dsprate;
	int addele[10],addrace[12],addsize[3],subele[10],subrace[12];
	int addeff[10],reseff[10];
	int watk_,watk_2,atkmods_[3],addele_[10],addrace_[12],addsize_[3];	//�񓁗��̂��߂ɒǉ�
	int atk_ele_,star_,overrefine_;				//�񓁗��̂��߂ɒǉ�
	int base_atk,atk_rate;
	int arrow_atk,arrow_ele,arrow_cri,arrow_hit,arrow_range;
	int arrow_addele[10],arrow_addrace[12],arrow_addsize[3],arrow_addeff[10];
	int nhealhp,nhealsp,nshealhp,nshealsp,nsshealhp,nsshealsp;
	int aspd_rate,speed_rate,hprecov_rate,sprecov_rate,critical_def,double_rate;
	int near_attack_def_rate,long_attack_def_rate,magic_def_rate,misc_def_rate;
	int matk_rate,ignore_def_ele,ignore_def_race,ignore_def_ele_,ignore_def_race_;
	int ignore_mdef_ele,ignore_mdef_race;
	int magic_addele[10],magic_addrace[12],magic_subrace[12];
	int perfect_hit,get_zeny_num;
	int critical_rate,hit_rate,flee_rate,flee2_rate,def_rate,def2_rate,mdef_rate,mdef2_rate;
	int def_ratio_atk_ele,def_ratio_atk_ele_,def_ratio_atk_race,def_ratio_atk_race_;
	int add_damage_class_count,add_damage_class_count_,add_magic_damage_class_count;
	short add_damage_classid[10],add_damage_classid_[10],add_magic_damage_classid[10];
	int add_damage_classrate[10],add_damage_classrate_[10],add_magic_damage_classrate[10];
	int add_def_class_count,add_mdef_class_count;
	int add_def_classid[10],add_mdef_classid[10];
	int add_def_classrate[10],add_mdef_classrate[10];
	int monster_drop_item_count;
	short monster_drop_itemid[10];
	int monster_drop_race[10],monster_drop_itemrate[10];
	int double_add_rate,speed_add_rate,aspd_add_rate,perfect_hit_add,get_zeny_add_num;
	short splash_range,splash_add_range;
	short spiritball, spiritball_old;
	int spirit_timer[MAX_SKILL_LEVEL];

	int reg_num;
	struct script_reg *reg;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	struct square dev;

	int trade_partner;
	int deal_item_index[10];
	int deal_item_amount[10];
	int deal_zeny;
	short deal_locked;

	int party_sended,party_invite,party_invite_account;
	int party_hp,party_x,party_y;

	int guild_sended,guild_invite,guild_invite_account;
	int guild_emblem_id,guild_alliance,guild_alliance_account;

	int vender_id;
	int vend_num;
	char message[80];
	struct vending vending[12];

	int catch_target_class;
	struct s_pet pet;
	struct pet_db *petDB;
	struct pet_data *pd;
	int pet_hungry_timer;

	int pvp_point,pvp_rank,pvp_timer,pvp_lastusers;

	char eventqueue[MAX_EVENTQUEUE][50];
	int eventtimer[MAX_EVENTTIMER];

	int last_skillid,last_skilllv;		// Added by RoVeRT
	struct{
		char name[24];
		int state;
	} ignore[50];
	int ignoreAll;				// End Addition
};

struct npc_item_list {
	int nameid,value;
};
struct npc_data {
	struct block_list bl;
	short n;
	short class,dir;
	short speed;
	char name[16];
	char exname[16];
	int chat_id;
	short flag;
	union {
		struct {
			char *script;
			short xs,ys;
		} scr;
		struct npc_item_list shop_item[1];
		struct {
			short xs,ys;
			short x,y;
			char name[16];
		} warp;
	} u;
	// �����Ƀ����o��ǉ����Ă͂Ȃ�Ȃ�(shop_item���ϒ��̈�)

	char eventqueue[MAX_EVENTQUEUE][50];
	int eventtimer[MAX_EVENTTIMER];
	short arenaflag;
};
struct mob_data {
	struct block_list bl;
	short n;
	short base_class,class,dir;
	short x0,y0,xs,ys;
	char name[16];
	int spawndelay1,spawndelay2;
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned targettype : 1 ;
		unsigned steal_flag : 1 ;
		unsigned steal_coin_flag : 1 ;
		unsigned skillcastcancel : 1 ;
		unsigned master_check : 1 ;
		unsigned change_walk_target : 1 ;
		unsigned walk_easy : 1 ;
	} state;
	int timer;
	short to_x,to_y;
	short speed;
	int hp;
	int target_id,attacked_id;
	struct walkpath_data walkpath;
	unsigned int next_walktime;
	unsigned int attackabletime;
	unsigned int last_deadtime,last_spawntime,last_thinktime;
	unsigned int canmove_tick;
	short move_fail_count;
	struct {
		int id;
		int dmg;
	} dmglog[DAMAGELOG_SIZE];
	struct item *lootitem;
	short lootitem_count;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	short opt1,opt2,option;
	short min_chase;
	short sg_count;

	int skilltimer;
	int skilltarget;
	short skillx,skilly;
	short skillid,skilllv,skillidx;
	unsigned int skilldelay[MAX_MOBSKILL];
	int def_ele;
	int master_id,master_dist;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL];
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	char npc_event[50];
};
struct pet_data {
	struct block_list bl;
	short n;
	short class,dir;
	short speed;
	char name[16];
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned change_walk_target : 1 ;
	} state;
	int timer;
	short to_x,to_y;
	short equip;
	struct walkpath_data walkpath;
	int target_id;
	int move_fail_count;
	unsigned int attackabletime,next_walktime,last_thinktime;
	struct map_session_data *msd;
};

enum { MS_IDLE,MS_WALK,MS_ATTACK,MS_DEAD,MS_DELAY };

enum { NONE_ATTACKABLE,ATTACKABLE };

struct map_data {
	char name[16];
	unsigned char *gat;	// NULL�Ȃ牺��map_data_other_server�Ƃ��Ĉ���
	struct block_list **block;
	struct block_list **block_mob;
	int *block_count,*block_mob_count;
	int m;
	short xs,ys;
	short bxs,bys;
	int npc_num;
	int users;
	struct {
		unsigned nomemo : 1;
		unsigned noteleport : 1;
		unsigned noreturn : 1;
		unsigned monster_noteleport : 1;
		unsigned nosave : 1;
		unsigned nobranch : 1;
		unsigned nopenalty : 1;
		unsigned pvp : 1;
		unsigned pvp_noparty : 1;
		unsigned pvp_noguild : 1;
		unsigned gvg : 1;
		unsigned gvg_noparty : 1;
	} flag;
	struct point save;
	struct npc_data *npc[MAX_NPC_PER_MAP];
};
struct map_data_other_server {
	char name[16];
	unsigned char *gat;	// NULL�Œ�ɂ��Ĕ��f
	unsigned long ip;
	unsigned int port;
};
#define read_gat(m,x,y) (map[m].gat[(x)+(y)*map[m].xs])
#define read_gatp(m,x,y) (m->gat[(x)+(y)*m->xs])

struct flooritem_data {
	struct block_list bl;
	short subx,suby;
	int cleartimer;
	int first_get_id,second_get_id,third_get_id;
	unsigned int first_get_tick,second_get_tick,third_get_tick;
	struct item item_data;
};

// taulin: revised for fixadies
enum {
	SP_SPEED,SP_BASEEXP,SP_JOBEXP,SP_KARMA,SP_MANNER,SP_HP,SP_MAXHP,SP_SP,	// 0-7
	SP_MAXSP,SP_STATUSPOINT,SP_0a,SP_BASELEVEL,SP_SKILLPOINT,SP_STR,SP_AGI,SP_VIT,	// 8-15
	SP_INT,SP_DEX,SP_LUK,SP_CLASS,SP_ZENY,SP_SEX,SP_NEXTBASEEXP,SP_NEXTJOBEXP,	// 16-23
	SP_WEIGHT,SP_MAXWEIGHT,SP_1a,SP_1b,SP_1c,SP_1d,SP_1e,SP_1f,	// 24-31
	SP_ATK1,SP_ATK2,SP_DEF1,SP_MATK1,SP_USTR,SP_UAGI,SP_UVIT,SP_UINT,	// 32-39
	SP_UDEX,SP_ULUK,SP_2a,SP_2b,SP_MATK2,SP_2d,SP_DEF2,SP_MDEF1,	// 40-47
	SP_MDEF2,SP_HIT,SP_FLEE1,SP_FLEE2,SP_CRITICAL,SP_ASPD,SP_36,SP_JOBLEVEL,	// 48-55
	SP_CARTINFO=99,	// 99

	// original 1000-
	SP_ATTACKRANGE=1000,	SP_ATKELE,SP_DEFELE,	// 1000-1002
	SP_CASTRATE, SP_MAXHPRATE, SP_MAXSPRATE, SP_SPRATE, // 1003-1006
	SP_ADDELE, SP_ADDRACE, SP_ADDSIZE, SP_SUBELE, SP_SUBRACE, // 1007-1011
	SP_ADDEFF, SP_RESEFF,	// 1012-1013
	SP_BASE_ATK,SP_ASPD_RATE,SP_HP_RECOV_RATE,SP_SP_RECOV_RATE,SP_SPEED_RATE, // 1014-1018
	SP_CRITICAL_DEF,SP_NEAR_ATK_DEF,SP_LONG_ATK_DEF, // 1019-1021
	SP_DOUBLE_RATE, SP_DOUBLE_ADD_RATE, SP_MATK, SP_MATK_RATE, // 1022-1025
	SP_IGNORE_DEF_ELE,SP_IGNORE_DEF_RACE, // 1026-1027
	SP_ATK_RATE,SP_SPEED_ADDRATE,SP_ASPD_ADDRATE, // 1028-1030
	SP_MAGIC_ATK_DEF,SP_MISC_ATK_DEF, // 1031-1032
	SP_IGNORE_MDEF_ELE,SP_IGNORE_MDEF_RACE, // 1033-1034
	SP_MAGIC_ADDELE,SP_MAGIC_ADDRACE,SP_MAGIC_SUBRACE, // 1035-1037
	SP_PERFECT_HIT_RATE,SP_PERFECT_HIT_ADD_RATE,SP_CRITICAL_RATE,SP_GET_ZENY_NUM,SP_ADD_GET_ZENY_NUM, // 1038-1042
	SP_ADD_DAMAGE_CLASS,SP_ADD_MAGIC_DAMAGE_CLASS,SP_ADD_DEF_CLASS,SP_ADD_MDEF_CLASS, // 1043-1046
	SP_ADD_MONSTER_DROP_ITEM,SP_DEF_RATIO_ATK_ELE,SP_DEF_RATIO_ATK_RACE,SP_ADD_SPEED, // 1047-1050
	SP_HIT_RATE,SP_FLEE_RATE,SP_FLEE2_RATE,SP_DEF_RATE,SP_DEF2_RATE,SP_MDEF_RATE,SP_MDEF2_RATE, // 1051-1057
	SP_SPLASH_RANGE,SP_SPLASH_ADD_RANGE, // 1058-

	SP_RESTART_FULL_RECORVER=2000,SP_NO_CASTCANCEL,SP_NO_SIZEFIX,SP_NO_MAGIC_DAMAGE,SP_NO_WEAPON_DAMAGE,SP_NO_GEMSTONE, // 2000-2005
	SP_NO_CASTCANCEL2,SP_INFINITE_ENDURE, // 2006-2007
};

enum {
	LOOK_BASE,LOOK_HAIR,LOOK_WEAPON,LOOK_HEAD_BOTTOM,LOOK_HEAD_TOP,LOOK_HEAD_MID,LOOK_HAIR_COLOR,LOOK_CLOTHES_COLOR,LOOK_SHIELD,LOOK_SHOES
};

struct chat_data {
	struct block_list bl;

	unsigned char pass[8];   /* password */
	unsigned char title[61]; /* room title MAX 60 */
	unsigned char limit;     /* join limit */
	unsigned char users;     /* current users */
	unsigned char pub;       /* room attribute */
	struct map_session_data *usersd[20];
	struct block_list *owner_;
	struct block_list **owner;
	char npc_event[50];
	int trigger;		// Added by RoVeRT
};

/*struct mons_data {
	int type;
	int max_hp;
	int npc_num;
	int job_exp;
	int base_exp;
	int atk;
	int hit;
	int flee;
	int def;
	struct {
		int nameid,p;
	} dropitem[16];
};*/


extern struct map_data map[];
extern int map_num;
extern int autosave_interval;
extern int agit_flag;

extern char motd_txt[];
extern char help_txt[];


// �I�S�̏��
void map_setusers(int);
int map_getusers(void);
// block�폜�֘A
int map_freeblock( void *bl );
int map_freeblock_lock(void);
int map_freeblock_unlock(void);
// block�֘A
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,...);
void map_foreachinmovearea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,int,int,...);
int map_countnearpc(int,int,int);
// �ꎞ�Iobject�֘A
int map_addobject(struct block_list *);
int map_delobject(int);
int map_delobjectnofree(int id);
void map_foreachobject(int (*)(struct block_list*,va_list),int,...);
//
int map_quit(struct map_session_data *);
// npc
int map_addnpc(int,struct npc_data *);


// ���A�C�e���֘A
int map_clearflooritem_timer(int,unsigned int,int,int);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem(struct item *,int,int,int,int,struct map_session_data *,struct map_session_data *,struct map_session_data *,int);
int map_searchrandfreecell(int,int,int,int);

// �L����id�����L������ �ϊ��֘A
void map_addchariddb(int charid,char *name);
int map_reqchariddb(struct map_session_data * sd,int charid);
char * map_charid2nick(int);

struct map_session_data * map_id2sd(int);
struct block_list * map_id2bl(int);
int map_mapname2mapid(char*);
int map_mapname2ipport(char*,int*,int*);
int map_setipport(char *name,unsigned long ip,int port);
void map_addiddb(struct block_list *);
void map_deliddb(struct block_list *bl);
int map_foreachiddb(int (*)(void*,void*,va_list),...);
void map_addnickdb(struct map_session_data *);
struct map_session_data * map_nick2sd(char*);

// gat�֘A
int map_getcell(int,int,int);
int map_setcell(int,int,int,int);

// ���̑�
int map_check_dir(int s_dir,int t_dir);
int map_calc_dir( struct block_list *src,int x,int y);

// path.c���
int path_search(struct walkpath_data*,int,int,int,int,int,int);
int path_blownpos(int m,int x0,int y0,int dx,int dy,int count);

#endif
