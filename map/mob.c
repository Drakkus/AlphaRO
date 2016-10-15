// $Id: mob.c,v 1.46 2004/02/18 18:10:58 rovert Exp $
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "timer.h"
#include "socket.h"
#include "db.h"
#include "map.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "mob.h"
#include "guild.h"
#include "itemdb.h"
#include "skill.h"
#include "battle.h"
#include "party.h"
#include "npc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MIN_MOBTHINKTIME 100

#define MOB_LAZYMOVEPERC 50		// �蔲�����[�hMOB�̈ړ��m���i�番���j
#define MOB_LAZYWARPPERC 20		// �蔲�����[�hMOB�̃��[�v�m���i�番���j

struct mob_db mob_db[2001];

/*==========================================
 * ���[�J���v���g�^�C�v�錾 (�K�v�ȕ��̂�)
 *------------------------------------------
 */
static int distance(int,int,int,int);
static int mob_makedummymobdb(int);
static int mob_timer(int,unsigned int,int,int);
int mobskill_use(struct mob_data *md,unsigned int tick,int event);
int mobskill_deltimer(struct mob_data *md );

/*==========================================
 * mob�𖼑O�Ō���
 *------------------------------------------
 */
int mobdb_searchname(const char *str)
{
	int i;
	for(i=0;i<sizeof(mob_db)/sizeof(mob_db[0]);i++){
		if( strcmpi(mob_db[i].name,str)==0 || strcmp(mob_db[i].jname,str)==0 ||
			memcmp(mob_db[i].name,str,16)==0 || memcmp(mob_db[i].jname,str,16)==0)
			return i;
	}
	return 0;
}
/*==========================================
 * MOB�o���p�̍Œ���̃f�[�^�Z�b�g
 *------------------------------------------
 */
int mob_spawn_dataset(struct mob_data *md,const char *mobname,int class)
{
	md->bl.prev=NULL;
	md->bl.next=NULL;
	if(strcmp(mobname,"--en--")==0)
		memcpy(md->name,mob_db[class].name,16);
	else if(strcmp(mobname,"--ja--")==0)
		memcpy(md->name,mob_db[class].jname,16);
	else
		memcpy(md->name,mobname,16);

	md->n = 0;
	md->base_class = md->class = class;
	md->bl.id= npc_get_new_npc_id();

	memset(&md->state,0,sizeof(md->state));
	md->timer = -1;
	md->target_id=0;
	md->attacked_id=0;
	md->speed=mob_db[class].speed;

	return 0;
}


/*==========================================
 * �ꔭMOB�o��(�X�N���v�g�p)
 *------------------------------------------
 */
int mob_once_spawn(struct map_session_data *sd,char *mapname,
	int x,int y,const char *mobname,int class,int amount,const char *event)
{
	struct mob_data *md=NULL;
	int m,count;

	if(strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	if(m<0 || amount<=0 || class>2000)	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	if(class<0){	// �����_���ɏ���
		int i=0;
		int j=-class-1;
		int k;
		if(j>=0 && j<MAX_RANDOMMONSTER){
			do{
				class=rand()%1000+1001;
				k=rand()%1000000;
			}while((mob_db[class].max_hp <= 0 || mob_db[class].summonper[j] <= k || (sd->status.base_level<mob_db[class].lv && battle_config.random_monster_checklv)) && (i++) < 2000);
			if(i>=2000){
				class=mob_db[0].summonper[j];
			}
		}else{
			return 0;
		}
//		if(battle_config.etc_log)
//			printf("mobclass=%d try=%d\n",class,i);
	}
	if(x<=0) x=sd->bl.x;
	if(y<=0) y=sd->bl.y;

	for(count=0;count<amount;count++){
		md=malloc(sizeof(struct mob_data));
		if(md==NULL){
			printf("mob_once_spawn: out of memory !\n");
			exit(1);
		}
		if(mob_db[class].mode&0x02) {
			md->lootitem=malloc(sizeof(struct item)*LOOTITEM_SIZE);
			if(md->lootitem==NULL){
				printf("mob_once_spawn: out of memory !\n");
				exit(1);
			}
		}
		else
			md->lootitem=NULL;

		mob_spawn_dataset(md,mobname,class);
		md->bl.m=m;
		md->bl.x=x;
		md->bl.y=y;

		md->x0=x;
		md->y0=y;
		md->xs=0;
		md->ys=0;
		md->spawndelay1=-1;	// ��x�̂݃t���O
		md->spawndelay2=-1;	// ��x�̂݃t���O

		memcpy(md->npc_event,event,sizeof(md->npc_event));

		md->bl.type=BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

	}
	return (amount>0)?md->bl.id:0;
}
/*==========================================
 * �ꔭMOB�o��(�X�N���v�g�p���G���A�w��)
 *------------------------------------------
 */
int mob_once_spawn_area(struct map_session_data *sd,char *mapname,
	int x0,int y0,int x1,int y1,
	const char *mobname,int class,int amount,const char *event)
{
	int x,y,i,c,max,lx=-1,ly=-1,id=0,m;

	if(strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	max=(y1-y0+1)*(x1-x0+1)*3;
	if(max>1000)max=1000;

	if(m<0 || amount<=0 || class>2000)	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	for(i=0;i<amount;i++){
		int j=0;
		do{
			x=rand()%(x1-x0+1)+x0;
			y=rand()%(y1-y0+1)+y0;
		}while( ( (c=map_getcell(m,x,y))==1)&& (++j)<max );
		if(j>=max){
			if(lx>=0){	// �����Ɏ��s�����̂ňȑO�ɕ������ꏊ���g��
				x=lx;
				y=ly;
			}else
				return 0;	// �ŏ��ɕ����ꏊ�̌��������s�����̂ł�߂�
		}
		id=mob_once_spawn(sd,mapname,x,y,mobname,class,1,event);
		lx=x;
		ly=y;
	}
	return id;
}

/*==========================================
 * mob�̌���������
 *------------------------------------------
 */
int mob_get_viewclass(int class)
{
	return mob_db[class].view_class;
}

/*==========================================
 * MOB�����݈ړ��\�ȏ�Ԃɂ��邩�ǂ���
 *------------------------------------------
 */
int mob_can_move(struct mob_data *md)
{
	if(md->canmove_tick > gettick() || (md->opt1 > 0 && md->opt1 != 6))
		return 0;
	if( md->sc_data[SC_ANKLE].timer != -1 || md->sc_data[SC_AUTOCOUNTER].timer != -1 )	// �A���N�����œ����Ȃ�
		return 0;

	return 1;
}

/*==========================================
 * mob�̎���1���ɂ����鎞�Ԍv�Z
 *------------------------------------------
 */
static int calc_next_walk_step(struct mob_data *md)
{
	if(md->walkpath.path_pos>=md->walkpath.path_len)
		return -1;
	if(md->walkpath.path[md->walkpath.path_pos]&1)
		return battle_get_speed(&md->bl)*14/10;
	return battle_get_speed(&md->bl);
}

static int mob_walktoxy_sub(struct mob_data *md);

/*==========================================
 * mob���s����
 *------------------------------------------
 */
static int mob_walk(struct mob_data *md,unsigned int tick,int data)
{
	int moveblock;
	int i,ctype;
	static int dirx[8]={0,-1,-1,-1,0,1,1,1};
	static int diry[8]={1,1,0,-1,-1,-1,0,1};
	int x,y,dx,dy,flag = 0;

	md->state.state=MS_IDLE;
	if(md->walkpath.path_pos>=md->walkpath.path_len || md->walkpath.path_pos!=data)
		return 0;

	md->walkpath.path_half ^= 1;
	if(md->walkpath.path_half==0){
		md->walkpath.path_pos++;
		if(md->state.change_walk_target){
			mob_walktoxy_sub(md);
			return 0;
		}
	}
	else {
		if(md->walkpath.path[md->walkpath.path_pos]>=8)
			return 1;

		x = md->bl.x;
		y = md->bl.y;
		ctype = map_getcell(md->bl.m,x,y);
		if(ctype == 1) {
			mob_stop_walking(md,1);
			return 0;
		}
		md->dir=md->walkpath.path[md->walkpath.path_pos];
		dx = dirx[md->dir];
		dy = diry[md->dir];

		ctype = map_getcell(md->bl.m,x+dx,y+dy);
		if(ctype == 1) {
			mob_walktoxy_sub(md);
			return 0;
		}
//		printf("mob map: %d x: %d y: %d\n",md->bl.m,md->bl.x,md->bl.y);
		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		md->state.state=MS_WALK;
		map_foreachinmovearea(clif_moboutsight,md->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,md);

		x += dx;
		y += dy;
		if(md->min_chase>13)
			md->min_chase--;

		if(moveblock) map_delblock(&md->bl);
		md->bl.x = x;
		md->bl.y = y;
		if(moveblock) map_addblock(&md->bl);

		map_foreachinmovearea(clif_mobinsight,md->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,md);
		md->state.state=MS_IDLE;

		if(md->option&4)
			skill_check_cloaking(&md->bl);

		skill_unit_move(&md->bl,tick,1);	// �X�L�����j�b�g�̌���

		if(md->sc_data[SC_ANKLE].timer != -1 || md->canmove_tick > tick)
			flag = 1;
	}
	if((i=calc_next_walk_step(md))>0 && !flag){
		i = i>>1;
		if(i < 1 && md->walkpath.path_half == 0)
			i = 1;
		md->timer=add_timer(tick+i,mob_timer,md->bl.id,md->walkpath.path_pos);
		md->state.state=MS_WALK;

		if(md->walkpath.path_pos>=md->walkpath.path_len)
			clif_fixmobpos(md);	// �Ƃ܂����Ƃ��Ɉʒu�̍đ��M
	}
	return 0;
}

/*==========================================
 * mob�̍U������
 *------------------------------------------
 */
static int mob_attack(struct mob_data *md,unsigned int tick,int data)
{
	struct map_session_data *sd;
	int mode,race,range;

	md->min_chase=13;
	md->state.state=MS_IDLE;
	md->state.skillstate=MSS_ATTACK;

	if( md->skilltimer!=-1 )	// �X�L���g�p��
		return 0;

	if(md->opt1>0 || md->option&6)
		return 0;

	if(md->sc_data[SC_AUTOCOUNTER].timer != -1)
		return 0;

	sd=map_id2sd(md->target_id);
	if(sd==NULL || pc_isdead(sd) || md->bl.m != sd->bl.m || sd->bl.prev == NULL || sd->invincible_timer != -1 ||
		distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y)>=13 || pc_isinvisible(sd)){
		md->target_id=0;
		md->state.targettype = NONE_ATTACKABLE;
		return 0;
	}

	mode=mob_db[md->class].mode;
	race=mob_db[md->class].race;
	if(!(mode&0x20) && (sd->sc_data[SC_TRICKDEAD].timer != -1 ||
		 ((pc_ishiding(sd) || sd->state.gangsterparadise) && race!=4 && race!=6) ) ) {
		md->target_id=0;
		md->state.targettype = NONE_ATTACKABLE;
		return 0;
	}

	range = mob_db[md->class].range;
	if(mode&1)
		range++;
	if(distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y) > range){
		md->state.state=MS_IDLE;
		return 0;
	}
	md->dir=map_calc_dir(&md->bl, sd->bl.x,sd->bl.y );	// �����ݒ�

	md->to_x = md->bl.x;
	md->to_y = md->bl.y;
	clif_fixmobpos(md);

	if( mobskill_use(md,tick,-2) )	// �X�L���g�p
		return 0;

	battle_weapon_attack(&md->bl,&sd->bl,tick,0);

/*	if(sd->sc_data[SC_AUTOCOUNTER].timer != -1){		// AppleGirl
		pc_attack(sd,md->bl.id,0);
		skill_status_change_end(&sd->bl,SC_AUTOCOUNTER,-1);
		return 0;
	}*/
/*	if(sd->sc_data[SC_AUTOGUARD].timer != -1){
		pc_attack(sd,md->bl.id,0);
		skill_status_change_end(&sd->bl,SC_AUTOGUARD,-1);
		return 0;
	}*/
	if(sd->sc_data[SC_REFLECTSHIELD].timer != -1){
		pc_attack(sd,md->bl.id,0);
		skill_status_change_end(&sd->bl,SC_AUTOCOUNTER,-1);
		return 0;
	}

	md->attackabletime = tick + battle_get_adelay(&md->bl);

	md->timer=add_timer(md->attackabletime,mob_timer,md->bl.id,0);
	md->state.state=MS_ATTACK;

	return 0;
}


/*==========================================
 * id���U�����Ă���PC�̍U�����~
 * clif_foreachclient��callback�֐�
 *------------------------------------------
 */
int mob_stopattacked(struct map_session_data *sd,va_list ap)
{
	int id;

	id=va_arg(ap,int);
	if(sd->attacktarget==id)
		pc_stopattack(sd);
	return 0;
}
/*==========================================
 * ���ݓ����Ă���^�C�}���~�߂ď�Ԃ�ύX
 *------------------------------------------
 */
int mob_changestate(struct mob_data *md,int state,int type)
{
	unsigned int tick;
	int i;

	if(md->timer != -1)
		delete_timer(md->timer,mob_timer);
	md->timer=-1;
	md->state.state=state;

	switch(state){
	case MS_WALK:
		if((i=calc_next_walk_step(md))>0){
			i = i>>2;
			md->timer=add_timer(gettick()+i,mob_timer,md->bl.id,0);
		}
		else
			md->state.state=MS_IDLE;
		break;
	case MS_ATTACK:
		tick = gettick();
		i=DIFF_TICK(md->attackabletime,tick);
		if(i>0 && i<2000)
			md->timer=add_timer(md->attackabletime,mob_timer,md->bl.id,0);
		else if(type) {
			md->attackabletime = tick + battle_get_amotion(&md->bl);
			md->timer=add_timer(md->attackabletime,mob_timer,md->bl.id,0);
		}
		else {
			md->attackabletime = tick + 1;
			md->timer=add_timer(md->attackabletime,mob_timer,md->bl.id,0);
		}
		break;
	case MS_DELAY:
		md->timer=add_timer(gettick()+type,mob_timer,md->bl.id,0);
		break;
	case MS_DEAD:
		skill_castcancel(&md->bl,0);
//		mobskill_deltimer(md);
		md->state.skillstate=MSS_DEAD;
		md->last_deadtime=gettick();
		// ���񂾂̂ł���mob�ւ̍U���ґS���̍U�����~�߂�
		clif_foreachclient(mob_stopattacked,md->bl.id);
		skill_unit_out_all(&md->bl,gettick(),1);
		skill_status_change_clear(&md->bl);	// �X�e�[�^�X�ُ����������
		skill_clear_unitgroup(&md->bl);	// �S�ẴX�L�����j�b�g�O���[�v���폜����
		skill_cleartimerskill(&md->bl);
		md->hp=md->target_id=md->attacked_id=0;
		md->state.targettype = NONE_ATTACKABLE;
		break;
	}

	return 0;
}

/*==========================================
 * mob��timer���� (timer�֐�)
 * ���s�ƍU���ɕ���
 *------------------------------------------
 */
static int mob_timer(int tid,unsigned int tick,int id,int data)
{
	struct mob_data *md;

	md=(struct mob_data*)map_id2bl(id);
	if(md==NULL || md->bl.type!=BL_MOB)
		return 1;

	if(md->timer != tid){
		if(battle_config.error_log)
			printf("mob_timer %d != %d\n",md->timer,tid);
		return 0;
	}
	md->timer=-1;
	if(md->bl.prev == NULL || md->state.state == MS_DEAD)
		return 1;

	map_freeblock_lock();
	switch(md->state.state){
	case MS_WALK:
		mob_walk(md,tick,data);
		break;
	case MS_ATTACK:
		mob_attack(md,tick,data);
		break;
	case MS_DELAY:
		mob_changestate(md,MS_IDLE,0);
		break;
	default:
		if(battle_config.error_log)
			printf("mob_timer : %d ?\n",md->state.state);
		break;
	}
	map_freeblock_unlock();
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_walktoxy_sub(struct mob_data *md)
{
	struct walkpath_data wpd;

	if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,md->to_x,md->to_y,md->state.walk_easy))
		return 1;
	memcpy(&md->walkpath,&wpd,sizeof(wpd));

	md->state.change_walk_target=0;
	mob_changestate(md,MS_WALK,0);
	clif_movemob(md);

	return 0;
}

/*==========================================
 * mob�ړ��J�n
 *------------------------------------------
 */
int mob_walktoxy(struct mob_data *md,int x,int y,int easy)
{
	struct walkpath_data wpd;

	if(md->state.state == MS_WALK && path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,x,y,easy) )
		return 1;

	md->state.walk_easy = easy;
	md->to_x=x;
	md->to_y=y;
	if(md->state.state == MS_WALK) {
		md->state.change_walk_target=1;
	} else {
		return mob_walktoxy_sub(md);
	}

	return 0;
}

/*==========================================
 * delay�t��mob spawn (timer�֐�)
 *------------------------------------------
 */
static int mob_delayspawn(int tid,unsigned int tick,int m,int n)
{
	mob_spawn(m);
	return 0;
}

/*==========================================
 * spawn�^�C�~���O�v�Z
 *------------------------------------------
 */
int mob_setdelayspawn(int id)
{
	unsigned int spawntime,spawntime1,spawntime2,spawntime3;
	struct mob_data *md;

	md=(struct mob_data*)map_id2bl(id);
	if(md==NULL || md->bl.type!=BL_MOB)
		return -1;

	// �������Ȃ�MOB�̏���
	if(md->spawndelay1==-1 && md->spawndelay2==-1 && md->n==0){
		map_deliddb(&md->bl);
		if(md->lootitem) {
			map_freeblock(md->lootitem);
			md->lootitem=NULL;
		}
		map_freeblock(md);	// free�̂����
		return 0;
	}

	spawntime1=md->last_spawntime+md->spawndelay1;
	spawntime2=md->last_deadtime+md->spawndelay2;
	spawntime3=gettick()+5000;
	// spawntime = max(spawntime1,spawntime2,spawntime3);
	if(DIFF_TICK(spawntime1,spawntime2)>0){
		spawntime=spawntime1;
	} else {
		spawntime=spawntime2;
	}
	if(DIFF_TICK(spawntime3,spawntime)>0){
		spawntime=spawntime3;
	}

	add_timer(spawntime,mob_delayspawn,id,0);
	return 0;
}

/*==========================================
 * mob�o���B�F�X��������������
 *------------------------------------------
 */
int mob_spawn(int id)
{
	int x=0,y=0,i=0,c;
	unsigned int tick = gettick();
	struct mob_data *md;

	md=(struct mob_data*)map_id2bl(id);
	if(md==NULL || md->bl.type!=BL_MOB)
		return -1;

	md->last_spawntime=tick;
	if( md->bl.prev!=NULL ){
//		clif_clearchar_area(&md->bl,3);
		skill_unit_out_all(&md->bl,gettick(),1);
		map_delblock(&md->bl);
	}
	else
		md->class = md->base_class;

	do {
		if(md->x0==0 && md->y0==0){
			x=rand()%(map[md->bl.m].xs-2)+1;
			y=rand()%(map[md->bl.m].ys-2)+1;
		} else {
			x=md->x0+rand()%(md->xs+1)-md->xs/2;
			y=md->y0+rand()%(md->ys+1)-md->ys/2;
		}
		i++;
	} while(((c=map_getcell(md->bl.m,x,y))==1) && i<50);

	if(i>=50){
//		if(battle_config.error_log)
//			printf("MOB spawn error %d @ %s\n",id,map[md->bl.m].name);
		add_timer(tick+5000,mob_delayspawn,id,0);
		return 1;
	}

	md->to_x=md->bl.x=x;
	md->to_y=md->bl.y=y;
	md->dir=0;

	map_addblock(&md->bl);

	memset(&md->state,0,sizeof(md->state));
	md->attacked_id = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;
	md->master_id=0;
	md->master_dist=0;

	md->state.state = MS_IDLE;
	md->state.skillstate = MSS_IDLE;
	md->timer = -1;
	md->last_thinktime = tick;
	md->next_walktime = tick+rand()%50+5000;
	md->attackabletime = tick;
	md->canmove_tick = tick;
	md->sg_count=0;

	md->skilltimer=-1;
	for(i=0,c=tick-1000*3600*10;i<MAX_MOBSKILL;i++)
		md->skilldelay[i] = c;
	md->skillid=0;
	md->skilllv=0;

	memset(md->dmglog,0,sizeof(md->dmglog));
	if(md->lootitem)
		memset(md->lootitem,0,sizeof(md->lootitem));
	md->lootitem_count = 0;

	for(i=0;i<MAX_SKILLTIMERSKILL/2;i++)
		md->skilltimerskill[i].timer = -1;

	for(i=0;i<MAX_STATUSCHANGE;i++)
		md->sc_data[i].timer=-1;
	md->sc_count=0;
	md->opt1=md->opt2=md->option=0;

	memset(md->skillunit,0,sizeof(md->skillunit));
	memset(md->skillunittick,0,sizeof(md->skillunittick));

	md->hp = battle_get_max_hp(&md->bl);
	if(md->hp<=0){
		mob_makedummymobdb(md->class);
		md->hp = battle_get_max_hp(&md->bl);
	}

	clif_spawnmob(md);

	return 0;
}

/*==========================================
 * 2�_�ԋ����v�Z
 *------------------------------------------
 */
static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

/*==========================================
 * MOB�̍U����~
 *------------------------------------------
 */
int mob_stopattack(struct mob_data *md)
{
	md->target_id=0;
	md->state.targettype = NONE_ATTACKABLE;
	md->attacked_id=0;
	return 0;
}
/*==========================================
 * MOB�̈ړ����~
 *------------------------------------------
 */
int mob_stop_walking(struct mob_data *md,int type)
{
	if(md->state.state == MS_WALK) {
		md->walkpath.path_len=0;
		md->to_x=md->bl.x;
		md->to_y=md->bl.y;
		mob_changestate(md,MS_IDLE,0);
	}
	if(type&0x01)
		clif_fixmobpos(md);
	if(type&0x02) {
		int delay=mob_db[md->class].dmotion;
		unsigned int tick = gettick();
		if(battle_config.monster_damage_delay_rate != 100)
			delay = delay*battle_config.monster_damage_delay_rate/100;
		if(md->canmove_tick < tick)
			md->canmove_tick = tick + delay;
	}

	return 0;
}

/*==========================================
 * �w��ID�̑��ݏꏊ�ւ̓��B�\��
 *------------------------------------------
 */
int mob_can_reach(struct mob_data *md,struct block_list *bl,int range)
{
	int dx=abs(bl->x - md->bl.x),dy=abs(bl->y - md->bl.y);
	struct walkpath_data wpd;
	int i;

	if( md->bl.m != bl-> m)	// �Ⴄ�}�b�v
		return 0;
	
	if( range>0 && range < ((dx>dy)?dx:dy) )	// ��������
		return 0;

	if( md->bl.x==bl->x && md->bl.y==bl->y )	// �����}�X
		return 1;

	// ��Q������
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x,bl->y,0)!=-1)
		return 1;
	if(bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;

	// �אډ\���ǂ�������
	dx=(dx>0)?1:((dx<0)?-1:0);
	dy=(dy>0)?1:((dy<0)?-1:0);
	if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x-dx,bl->y-dy,0)!=-1)
		return 1;
	for(i=0;i<9;i++){
		if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x-1+i/3,bl->y-1+i%3,0)!=-1)
			return 1;
	}
	return 0;
}

/*==========================================
 * �����X�^�[�̍U���Ώی���
 *------------------------------------------
 */
int mob_target(struct mob_data *md,struct block_list *bl,int dist)
{
	struct map_session_data *sd;
	struct status_change *sc_data = battle_get_sc_data(bl);
	short *option = battle_get_option(bl);
	int mode=mob_db[md->class].mode,race=mob_db[md->class].race;

	if(!mode) {
		md->target_id = 0;
		return 0;
	}
	// �^�Q�ς݂Ń^�Q��ς���C���Ȃ��Ȃ牽�����Ȃ�
	if( (md->target_id > 0 && md->state.targettype == ATTACKABLE) && ( !(mode&0x04) || rand()%100>25) )
		return 0;

	if(mode&0x20 ||	// MVPMOB�Ȃ狭��
		(sc_data && sc_data[SC_TRICKDEAD].timer == -1 &&
		 ( (option && !(*option&0x06) ) || race==4 || race==6) ) ){
		if(bl->type == BL_PC) {
			sd = (struct map_session_data *)bl;
			if(sd->invincible_timer != -1 || pc_isinvisible(sd))
				return 0;
			if(!(mode&0x20) && race!=4 && race!=6 && sd->state.gangsterparadise)
				return 0;
		}

		md->target_id=bl->id;	// �W�Q���Ȃ������̂Ń��b�N
		if(bl->type == BL_PC || bl->type == BL_MOB)
			md->state.targettype = ATTACKABLE;
		else
			md->state.targettype = NONE_ATTACKABLE;
		md->min_chase=dist+13;
		if(md->min_chase>26)
			md->min_chase=26;
	}
	return 0;
}

/*==========================================
 * �A�N�e�B�u�����X�^�[�̍��G���[�e�B��
 *------------------------------------------
 */
static int mob_ai_sub_hard_activesearch(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd=(struct map_session_data *)bl;
	struct mob_data* md;
	int mode,race,dist,*pcc;

	md=va_arg(ap,struct mob_data *);
	pcc=va_arg(ap,int *);
	mode=mob_db[md->class].mode;

	// �A�N�e�B�u�Ń^�[�Q�b�g�˒����ɂ���Ȃ�A���b�N����
	if( mode&0x04 ){
		if( !pc_isdead(sd) && sd->bl.m == md->bl.m && sd->invincible_timer == -1 && !pc_isinvisible(sd) &&
			(dist=distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y))<9){

			race=mob_db[md->class].race;
			if(mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				((!pc_ishiding(sd) && !sd->state.gangsterparadise) || race==4 || race==6) )  ){	// �W�Q���Ȃ�������

				if( mob_can_reach(md,bl,12) && 		// ���B�\������
					rand()%1000<1000/(++(*pcc)) ){	// �͈͓�PC�œ��m���ɂ���
					md->target_id=sd->bl.id;
					md->state.targettype = ATTACKABLE;
					md->min_chase=13;
				}
			}
		}
	}
	return 0;
}

/*==========================================
 * loot monster item search
 *------------------------------------------
 */
static int mob_ai_sub_hard_lootsearch(struct block_list *bl,va_list ap)
{
	struct mob_data* md;
	int mode,dist,*itc;

	md=va_arg(ap,struct mob_data *);
	itc=va_arg(ap,int *);
	mode=mob_db[md->class].mode;

	if( !md->target_id && mode&0x02){
		if(!md->lootitem || (battle_config.monster_loot_type == 1 && md->lootitem_count >= LOOTITEM_SIZE) )
			return 0;
		if(bl->m == md->bl.m && (dist=distance(md->bl.x,md->bl.y,bl->x,bl->y))<9){
			if( mob_can_reach(md,bl,12) && 		// ���B�\������
				rand()%1000<1000/(++(*itc)) ){	// �͈͓�PC�œ��m���ɂ���
				md->target_id=bl->id;
				md->state.targettype = NONE_ATTACKABLE;
				md->min_chase=13;
			}
		}
	}
	return 0;
}

/*==========================================
 * �����N�����X�^�[�̍��G���[�e�B��
 *------------------------------------------
 */
static int mob_ai_sub_hard_linksearch(struct block_list *bl,va_list ap)
{
	struct mob_data *tmd=(struct mob_data *)bl;
	struct mob_data* md;
	struct block_list *target;

	md=va_arg(ap,struct mob_data *);
	target=va_arg(ap,struct block_list *);

	// �����N�����X�^�[�Ŏ˒����ɉɂȓ���MOB������Ȃ�A���b�N������
/*	if( (md->target_id > 0 && md->state.targettype == ATTACKABLE) && mob_db[md->class].mode&0x08){
		if( tmd->class==md->class && (!tmd->target_id || md->state.targettype == NONE_ATTACKABLE) && tmd->bl.m == md->bl.m){
			if( mob_can_reach(tmd,target,12) ){	// ���B�\������
				tmd->target_id=md->target_id;
				tmd->state.targettype = ATTACKABLE;
				tmd->min_chase=13;
			}
		}
	}*/
	if( md->attacked_id > 0 && mob_db[md->class].mode&0x08){
		if( tmd->class==md->class && tmd->bl.m == md->bl.m && (!tmd->target_id || md->state.targettype == NONE_ATTACKABLE)){
			if( mob_can_reach(tmd,target,12) ){	// ���B�\������
				tmd->target_id=md->attacked_id;
				tmd->state.targettype = ATTACKABLE;
				tmd->min_chase=13;
			}
		}
	}

	return 0;
}
/*==========================================
 * ��芪�������X�^�[�̎匟��
 *------------------------------------------
 */
static int mob_ai_sub_hard_mastersearch(struct block_list *bl,va_list ap)
{
	struct mob_data *mmd=(struct mob_data *)bl;
	struct mob_data *md;
	int mode,race,old_dist;
	unsigned int tick;

	md=va_arg(ap,struct mob_data *);
	tick=va_arg(ap,unsigned int);
	mode=mob_db[md->class].mode;

	// ��ł͂Ȃ�
	if(mmd->bl.type!=BL_MOB || mmd->bl.id!=md->master_id)
		return 0;

	// ��Ƃ̋����𑪂�
	old_dist=md->master_dist;
	md->master_dist=distance(md->bl.x,md->bl.y,mmd->bl.x,mmd->bl.y);

	// ���O�܂Ŏ傪�߂��ɂ����̂Ńe���|�[�g���Ēǂ�������
	if( old_dist<10 && md->master_dist>18){
		mob_warp(md,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	// �傪���邪�A���������̂ŋߊ��
	if((!md->target_id || md->state.targettype == NONE_ATTACKABLE) && mob_can_move(md) && 
		(md->walkpath.path_pos>=md->walkpath.path_len || md->walkpath.path_len==0) && md->master_dist<15){
		int i=0,dx,dy,ret;
		if(md->master_dist>4) {
			do {
				if(i<=5){
					dx=mmd->bl.x - md->bl.x;
					dy=mmd->bl.y - md->bl.y;
					if(dx<0) dx+=(rand()%( (dx<-3)?3:-dx )+1);
					else if(dx>0) dx-=(rand()%( (dx>3)?3:dx )+1);
					if(dy<0) dy+=(rand()%( (dy<-3)?3:-dy )+1);
					else if(dy>0) dy-=(rand()%( (dy>3)?3:dy )+1);
				}else{
					dx=mmd->bl.x - md->bl.x + rand()%7 - 3;
					dy=mmd->bl.y - md->bl.y + rand()%7 - 3;
				}

				ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
				i++;
			} while(ret && i<10);
		}
		else {
			do {
				dx = rand()%9 - 5;
				dy = rand()%9 - 5;
				if( dx == 0 && dy == 0) {
					dx = (rand()%1)? 1:-1;
					dy = (rand()%1)? 1:-1;
				}
				dx += mmd->bl.x;
				dy += mmd->bl.y;

				ret=mob_walktoxy(md,mmd->bl.x+dx,mmd->bl.y+dy,0);
				i++;
			} while(ret && i<10);
		}

		md->next_walktime=tick+500;
		md->state.master_check = 1;
	}

	// �傪���āA�傪���b�N���Ă��Ď����̓��b�N���Ă��Ȃ�
	if( (mmd->target_id>0 && mmd->state.targettype == ATTACKABLE) && (!md->target_id || md->state.targettype == NONE_ATTACKABLE) ){
		struct map_session_data *sd=map_id2sd(mmd->target_id);
		if(sd!=NULL && !pc_isdead(sd) && sd->invincible_timer == -1 && !pc_isinvisible(sd)){

			race=mob_db[md->class].race;
			if(mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				( (!pc_ishiding(sd) && !sd->state.gangsterparadise) || race==4 || race==6) ) ){	// �W�Q���Ȃ�������

				md->target_id=sd->bl.id;
				md->state.targettype = ATTACKABLE;
				md->min_chase=5+distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y);
				md->state.master_check = 1;
			}
		}
	}

	// �傪���āA�傪���b�N���ĂȂ��Ď����̓��b�N���Ă���
/*	if( (md->target_id>0 && mmd->state.targettype == ATTACKABLE) && (!mmd->target_id || mmd->state.targettype == NONE_ATTACKABLE) ){
		struct map_session_data *sd=map_id2sd(md->target_id);
		if(sd!=NULL && !pc_isdead(sd) && sd->invincible_timer == -1 && !pc_isinvisible(sd)){

			race=mob_db[mmd->class].race;
			if(mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				(!(sd->status.option&0x06) || race==4 || race==6)
				) ){	// �W�Q���Ȃ�������
			
				mmd->target_id=sd->bl.id;
				mmd->state.targettype = ATTACKABLE;
				mmd->min_chase=5+distance(mmd->bl.x,mmd->bl.y,sd->bl.x,sd->bl.y);
			}
		}
	}*/
	
	return 0;
}

/*==========================================
 * ���b�N���~�߂đҋ@��ԂɈڂ�B
 *------------------------------------------
 */
static int mob_unlocktarget(struct mob_data *md,int tick)
{
	md->target_id=0;
	md->state.targettype = NONE_ATTACKABLE;
	md->state.skillstate=MSS_IDLE;
	md->next_walktime=tick+rand()%3000+3000;
	return 0;
}
/*==========================================
 * �����_�����s
 *------------------------------------------
 */
static int mob_randomwalk(struct mob_data *md,int tick)
{
	const int retrycount=20;
	int speed=battle_get_speed(&md->bl);
	if(DIFF_TICK(md->next_walktime,tick)<0){
		int i,x,y,c,d=12-md->move_fail_count;
		if(d<5) d=5;
		for(i=0;i<retrycount;i++){	// �ړ��ł���ꏊ�̒T��
			int r=rand();
			x=md->bl.x+r%(d*2+1)-d;
			y=md->bl.y+r/(d*2+1)%(d*2+1)-d;
			if((c=map_getcell(md->bl.m,x,y))!=1 && mob_walktoxy(md,x,y,1)==0){
				md->move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				md->move_fail_count++;
				if(md->move_fail_count>1000){
					if(battle_config.error_log)
						printf("MOB cant move. random spawn %d, class = %d\n",md->bl.id,md->class);
					md->move_fail_count=0;
					mob_spawn(md->bl.id);
				}
			}
		}
		for(i=c=0;i<md->walkpath.path_len;i++){	// ���̕��s�J�n�������v�Z
			if(md->walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		md->next_walktime = tick+rand()%3000+3000+c;
		md->state.skillstate=MSS_WALK;
		return 1;
	}
	return 0;
}

/*==========================================
 * PC���߂��ɂ���MOB��AI
 *------------------------------------------
 */
static int mob_ai_sub_hard(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	struct map_session_data *sd;
	struct block_list *bl_item;
	struct flooritem_data *fitem;
	unsigned int tick;
	int i,dx,dy,ret,dist;
	int attack_type=0;
	int mode,race;

	md=(struct mob_data*)bl;
	tick=va_arg(ap,unsigned int);


	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME)
		return 0;
	md->last_thinktime=tick;

	if( md->skilltimer!=-1 || md->bl.prev==NULL ){	// �X�L���r���������S��
		if(DIFF_TICK(tick,md->next_walktime)>MIN_MOBTHINKTIME)
			md->next_walktime=tick;
		return 0;
	}

	mode=mob_db[md->class].mode;
	race=mob_db[md->class].race;

	// �ُ�
	if((md->opt1 > 0 && md->opt1 != 6) || md->state.state==MS_DELAY){
		return 0;
	}

	if(!mode && md->target_id > 0)
		md->target_id = 0;

	if(md->attacked_id > 0 && mode&0x08){	// �����N�����X�^�[
		sd=map_id2sd(md->attacked_id);
		if(sd) {
			if(sd->invincible_timer == -1 && !pc_isinvisible(sd)) {
				map_foreachinarea(mob_ai_sub_hard_linksearch,md->bl.m,
					md->bl.x-13,md->bl.y-13,
					md->bl.x+13,md->bl.y+13,
					BL_MOB,md,&sd->bl);
			}
		}
	}

	// �܂��U�����ꂽ���m�F�i�A�N�e�B�u�Ȃ�25%�̊m���Ń^�[�Q�b�g�ύX�j
	if( mode>0 && md->attacked_id>0 && (!md->target_id || md->state.targettype == NONE_ATTACKABLE
		|| (mob_db[md->class].mode&0x04 && rand()%100<25 )  )){
		sd=map_id2sd(md->attacked_id);
		if(sd==NULL || md->bl.m != sd->bl.m || sd->bl.prev == NULL || sd->invincible_timer != -1 || pc_isinvisible(sd) ||
			(dist=distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y))>=32){
			md->attacked_id=0;
		}
		else {
			md->target_id=md->attacked_id; // set target
			md->state.targettype = ATTACKABLE;
			attack_type = 1;
			md->attacked_id=0;
			md->min_chase=dist+13;
			if(md->min_chase>26)
				md->min_chase=26;
		}
	}

	md->state.master_check = 0;
	// ��芪�������X�^�[�̎�̌���
	if( md->master_id > 0 )
		map_foreachinarea(mob_ai_sub_hard_mastersearch,md->bl.m,
						  md->bl.x-AREA_SIZE*2,md->bl.y-AREA_SIZE*2,
						  md->bl.x+AREA_SIZE*2,md->bl.y+AREA_SIZE*2,
						  BL_MOB,md,tick);

	// �A�N�e�B�������X�^�[�̍��G
	if( (!md->target_id || md->state.targettype == NONE_ATTACKABLE) && mode&0x04 && !md->state.master_check &&
		battle_config.monster_active_enable){
		i=0;
		map_foreachinarea(mob_ai_sub_hard_activesearch,md->bl.m,
						  md->bl.x-AREA_SIZE*2,md->bl.y-AREA_SIZE*2,
						  md->bl.x+AREA_SIZE*2,md->bl.y+AREA_SIZE*2,
						  BL_PC,md,&i);
	}

	// ���[�g�����X�^�[�̃A�C�e���T�[�`
	if( !md->target_id && mode&0x02 && !md->state.master_check){
		i=0;
		map_foreachinarea(mob_ai_sub_hard_lootsearch,md->bl.m,
						  md->bl.x-AREA_SIZE*2,md->bl.y-AREA_SIZE*2,
						  md->bl.x+AREA_SIZE*2,md->bl.y+AREA_SIZE*2,
						  BL_ITEM,md,&i);
	}

	// �U���Ώۂ�����Ȃ�U��
	if(md->target_id > 0){
		sd=map_id2sd(md->target_id);
		if(sd) {
			if(sd->bl.m != md->bl.m || sd->bl.prev == NULL ||
				 (dist=distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y))>=md->min_chase){
			// �ʃ}�b�v���A���E�O
				mob_unlocktarget(md,tick);

			} else if( !(mode&0x20) &&
					(sd->sc_data[SC_TRICKDEAD].timer != -1 ||
					 ((pc_ishiding(sd) || sd->state.gangsterparadise) && race!=4 && race!=6)
					) ){
			// �X�L���Ȃǂɂ����G�W�Q
				mob_unlocktarget(md,tick);

			} else if(!battle_check_range(&md->bl,&sd->bl,mob_db[md->class].range)){

				// �U���͈͊O�Ȃ̂ňړ�
				if(!(mode&1)){	// �ړ����Ȃ����[�h
					mob_unlocktarget(md,tick);
					return 0;
				}
							
				if( !mob_can_move(md) )	// �����Ȃ���Ԃɂ���
					return 0;

				md->state.skillstate=MSS_CHASE;	// �ˌ����X�L��
				mobskill_use(md,tick,-1);
					
//				if(md->timer != -1 && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,sd->bl.x,sd->bl.y)<2) )
				if(md->timer != -1 && md->state.state!=MS_ATTACK && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,sd->bl.x,sd->bl.y)<2) )
					return 0; // ���Ɉړ���

				if( !mob_can_reach(md,&sd->bl,(md->min_chase>13)?md->min_chase:13) ){
					// �ړ��ł��Ȃ��̂Ń^�Q�����iIW�Ƃ��H�j
					mob_unlocktarget(md,tick);

				}else{
					// �ǐ�
					md->next_walktime=tick+500;
					i=0;
					do {
						if(i==0){	// �ŏ���AEGIS�Ɠ������@�Ō���
							dx=sd->bl.x - md->bl.x;
							dy=sd->bl.y - md->bl.y;
							if(dx<0) dx++;
							else if(dx>0) dx--;
							if(dy<0) dy++;
							else if(dy>0) dy--;
						}else{	// ���߂Ȃ�Athena��(�����_��)
							dx=sd->bl.x - md->bl.x + rand()%3 - 1;
							dy=sd->bl.y - md->bl.y + rand()%3 - 1;
						}
/*						if(path_search(&md->walkpath,md->bl.m,md->bl.x,md->bl.y,md->bl.x+dx,md->bl.y+dy,0)){
							dx=sd->bl.x - md->bl.x;
							dy=sd->bl.y - md->bl.y;
							if(dx<0) dx--;
							else if(dx>0) dx++;
							if(dy<0) dy--;
							else if(dy>0) dy++;
						}*/
						ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
						i++;
					} while(ret && i<5);

					if(ret){ // �ړ��s�\�ȏ�����̍U���Ȃ�2������
						if(dx<0) dx=2;
						else if(dx>0) dx=-2;
						if(dy<0) dy=2;
						else if(dy>0) dy=-2;
						mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
					}
				}

			} else { // �U���˒��͈͓�
				md->state.skillstate=MSS_ATTACK;

				if(md->state.state==MS_WALK){	// ���s���Ȃ��~
					mob_stop_walking(md,1);
				}
				if(md->state.state==MS_ATTACK)
					return 0; // ���ɍU����
				mob_changestate(md,MS_ATTACK,attack_type);

/*				if(mode&0x08){	// �����N�����X�^�[
					map_foreachinarea(mob_ai_sub_hard_linksearch,md->bl.m,
						md->bl.x-13,md->bl.y-13,
						md->bl.x+13,md->bl.y+13,
						BL_MOB,md,&sd->bl);
				}*/
			}
			return 0;
		}
		else {	// ���[�g�����X�^�[����
			bl_item = map_id2bl(md->target_id);
		
			if(bl_item == NULL || bl_item->type != BL_ITEM ||bl_item->m != md->bl.m ||
				 (dist=distance(md->bl.x,md->bl.y,bl_item->x,bl_item->y))>=md->min_chase || !md->lootitem){
				 // �������邩�A�C�e�����Ȃ��Ȃ���
				mob_unlocktarget(md,tick);

			}
			else if(dist){
				if(!(mode&1)){	// �ړ����Ȃ����[�h
					mob_unlocktarget(md,tick);
					return 0;
				}
				if( !mob_can_move(md) )	// �����Ȃ���Ԃɂ���
					return 0;

				md->state.skillstate=MSS_LOOT;	// ���[�g���X�L���g�p
				mobskill_use(md,tick,-1);

//				if(md->timer != -1 && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,bl_item->x,bl_item->y)<2) )
				if(md->timer != -1 && md->state.state!=MS_ATTACK && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,bl_item->x,bl_item->y) <= 0))
					return 0; // ���Ɉړ���


				md->next_walktime=tick+500;
				dx=bl_item->x - md->bl.x;
				dy=bl_item->y - md->bl.y;
/*				if(path_search(&md->walkpath,md->bl.m,md->bl.x,md->bl.y,md->bl.x+dx,md->bl.y+dy,0)){
					dx=bl_item->x - md->bl.x;
					dy=bl_item->y - md->bl.y;
				}*/
				ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);

				if(ret){
					// �ړ��ł��Ȃ��̂Ń^�Q�����iIW�Ƃ��H�j
					mob_unlocktarget(md,tick);
				}

			} else {	// �A�C�e���܂ł��ǂ蒅����
				if(md->state.state==MS_ATTACK)
					return 0; // �U����
				if(md->state.state==MS_WALK){	// ���s���Ȃ��~
					mob_stop_walking(md,1);
				}
			
				fitem = (struct flooritem_data *)bl_item;
				if(md->lootitem_count < LOOTITEM_SIZE)
					memcpy(&md->lootitem[md->lootitem_count++],&fitem->item_data,sizeof(md->lootitem[0]));
				else if(battle_config.monster_loot_type == 1 && md->lootitem_count >= LOOTITEM_SIZE) {
					mob_unlocktarget(md,tick);
					return 0;
				}
				else {
					if(md->lootitem[0].card[0] == (short)0xff00)
						intif_delete_petdata(*((long *)(&md->lootitem[0].card[1])));
					for(i=0;i<LOOTITEM_SIZE-1;i++)
						memcpy(&md->lootitem[i],&md->lootitem[i+1],sizeof(md->lootitem[0]));
					memcpy(&md->lootitem[LOOTITEM_SIZE-1],&fitem->item_data,sizeof(md->lootitem[0]));
				}
				map_clearflooritem(bl_item->id);
				mob_unlocktarget(md,tick);
			
			}
			return 0;
		}
	}

	// ���s��/�ҋ@���X�L���g�p
	if( mobskill_use(md,tick,-1) )
		return 0;

	// ���s����
	if( mode&1 && mob_can_move(md) &&	// �ړ��\MOB&�������Ԃɂ���
		(md->master_id==0 || md->master_dist>10) ){	//��芪��MOB����Ȃ�

		// �����_���ړ�
		if( mob_randomwalk(md,tick) )
			return 0;
	}

	// �����I����Ă�̂őҋ@
	if( md->walkpath.path_len==0 || md->walkpath.path_pos>=md->walkpath.path_len )
		md->state.skillstate=MSS_IDLE;
	return 0;
}

/*==========================================
 * PC���E����mob�p�܂��ߏ���(foreachclient)
 *------------------------------------------
 */
static int mob_ai_sub_foreachclient(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;

	tick=va_arg(ap,unsigned int);
	map_foreachinarea(mob_ai_sub_hard,sd->bl.m,
					  sd->bl.x-AREA_SIZE*2,sd->bl.y-AREA_SIZE*2,
					  sd->bl.x+AREA_SIZE*2,sd->bl.y+AREA_SIZE*2,
					  BL_MOB,tick);

	return 0;
}

/*==========================================
 * PC���E����mob�p�܂��ߏ��� (interval timer�֐�)
 *------------------------------------------
 */
static int mob_ai_hard(int tid,unsigned int tick,int id,int data)
{
	clif_foreachclient(mob_ai_sub_foreachclient,tick);

	return 0;
}

/*==========================================
 * �蔲�����[�hMOB AI�i�߂���PC�����Ȃ��j
 *------------------------------------------
 */
static int mob_ai_sub_lazy(void * key,void * data,va_list app)
{
	struct mob_data *md=data;
	unsigned int tick;
	va_list ap;

	if(md==NULL || md->bl.type!=BL_MOB)
		return 0;
	ap=va_arg(app,va_list);
	tick=va_arg(ap,unsigned int);

	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME*10)
		return 0;
	md->last_thinktime=tick;

	if(md->bl.prev==NULL || md->skilltimer!=-1){
		if(DIFF_TICK(tick,md->next_walktime)>MIN_MOBTHINKTIME*10)
			md->next_walktime=tick;
		return 0;
	}

	if(DIFF_TICK(md->next_walktime,tick)<0 &&
		(mob_db[md->class].mode&1) && mob_can_move(md) ){

		if( map[md->bl.m].users>0 ){
			// �����}�b�v��PC������̂ŁA�����܂��Ȏ蔲������������
	
			// ���X�ړ�����
			if(rand()%1000<MOB_LAZYMOVEPERC)
				mob_randomwalk(md,tick);
		
			// ����MOB�łȂ��ABOSS�ł��Ȃ�MOB�͎��X�A�����Ȃ���
			else if( rand()%1000<MOB_LAZYWARPPERC && md->x0<=0 &&
				mob_db[md->class].mexp <= 0 && !(mob_db[md->class].mode & 0x20))
				mob_spawn(md->bl.id);
			
		}else{
			// �����}�b�v�ɂ���PC�����Ȃ��̂ŁA�Ƃ��Ă��K���ȏ���������
		
			// ����MOB�łȂ��ꍇ�A���X�ړ�����i���s�ł͂Ȃ����[�v�ŏ����y���j
			if( md->x0<=0 && rand()%1000<MOB_LAZYWARPPERC )
				mob_warp(md,-1,-1,-1);
		}
	
		md->next_walktime = tick+rand()%10000+5000;
	}
	return 0;
}

/*==========================================
 * PC���E�O��mob�p�蔲������ (interval timer�֐�)
 *------------------------------------------
 */
static int mob_ai_lazy(int tid,unsigned int tick,int id,int data)
{
	map_foreachiddb(mob_ai_sub_lazy,tick);

	return 0;
}


/*==========================================
 * delay�t��item drop�p�\����
 * timer�֐��ɓn�����int 2�����Ȃ̂�
 * ���̍\���̂Ƀf�[�^�����ēn��
 *------------------------------------------
 */
struct delay_item_drop {
	int m,x,y;
	int nameid,amount;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

struct delay_item_drop2 {
	int m,x,y;
	struct item item_data;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

/*==========================================
 * delay�t��item drop (timer�֐�)
 *------------------------------------------
 */
static int mob_delay_item_drop(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop *ditem;
	struct item temp_item;

	ditem=(struct delay_item_drop *)id;

	memset(&temp_item,0,sizeof(temp_item));
	temp_item.nameid = ditem->nameid;
	temp_item.amount = ditem->amount;
	temp_item.identify = !itemdb_isequip(temp_item.nameid);
	map_addflooritem(&temp_item,1,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);

	free(ditem);
	return 0;
}

/*==========================================
 * delay�t��item drop (timer�֐�) - lootitem
 *------------------------------------------
 */
static int mob_delay_item_drop2(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop2 *ditem;

	ditem=(struct delay_item_drop2 *)id;

	map_addflooritem(&ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);

	free(ditem);
	return 0;
}

/*==========================================
 * md������
 *------------------------------------------
 */
int mob_delete(struct mob_data *md)
{
	mob_changestate(md,MS_DEAD,0);
	clif_clearchar_area(&md->bl,1);
	map_delblock(&md->bl);
	mob_setdelayspawn(md->bl.id);
	return 0;
}

int mob_catch_delete(struct mob_data *md)
{
	mob_changestate(md,MS_DEAD,0);
	clif_clearchar_area(&md->bl,0);
	map_delblock(&md->bl);
	mob_setdelayspawn(md->bl.id);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md = (struct mob_data *)bl;
	int id;
	id=va_arg(ap,int);
	if(md->master_id > 0 && md->master_id == id )
		mob_damage(NULL,md,md->hp,1);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave(struct mob_data *md)
{
	map_foreachinarea(mob_deleteslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs-1,map[md->bl.m].ys-1,
		BL_MOB,md->bl.id);
	return 0;
}

/*==========================================
 * md��sd����damage�̃_���[�W
 *------------------------------------------
 */
int mob_damage(struct block_list *src,struct mob_data *md,int damage,int type)
{
	int i,count,minpos,mindmg;
	struct map_session_data *sd = NULL,*tmpsd[DAMAGELOG_SIZE];
	struct {
		struct party *p;
		int id,base_exp,job_exp;
	} pt[DAMAGELOG_SIZE];
	int pnum=0;
	int mvp_damage,max_hp = battle_get_max_hp(&md->bl);
	struct map_session_data *mvp_sd=sd ,*second_sd = NULL,*third_sd = NULL;

	if(src && src->type == BL_PC) {
		sd = (struct map_session_data *)src;
		mvp_sd = sd;
	}

//	if(battle_config.battle_log)
//		printf("mob_damage %d %d %d\n",md->hp,max_hp,damage);
	if(md->bl.prev==NULL){
		if(battle_config.error_log)
			printf("mob_damage : BlockError!!\n");
		return 0;
	}

	if(md->state.state==MS_DEAD || md->hp<=0) {
		if(md->bl.prev != NULL) {
			mob_changestate(md,MS_DEAD,0);
			mobskill_use(md,gettick(),-1);	// ���S���X�L��
			clif_clearchar_area(&md->bl,1);
			map_delblock(&md->bl);
			mob_setdelayspawn(md->bl.id);
		}
		return 0;
	}

	if(md->sc_data[SC_ENDURE].timer == -1)
		mob_stop_walking(md,3);

	if(md->hp > max_hp)
		md->hp = max_hp;

	// over kill���͊ۂ߂�
	if(damage>md->hp)
		damage=md->hp;

	if(!(type&2)) {
		if(sd!=NULL){
			for(i=0,minpos=0,mindmg=0x7fffffff;i<DAMAGELOG_SIZE;i++){
				if(md->dmglog[i].id==sd->bl.id)
					break;
				if(md->dmglog[i].id==0){
					minpos=i;
					mindmg=0;
				}
				else if(md->dmglog[i].dmg<mindmg){
					minpos=i;
					mindmg=md->dmglog[i].dmg;
				}
			}
			if(i<DAMAGELOG_SIZE)
				md->dmglog[i].dmg+=damage;
			else {
				md->dmglog[minpos].id=sd->bl.id;
				md->dmglog[minpos].dmg=damage;
			}

			if(md->attacked_id <= 0)
				md->attacked_id = sd->bl.id;
		}
		if(src && src->type == BL_PET && battle_config.pet_attack_exp_to_master) {
			struct pet_data *pd = (struct pet_data *)src;
			for(i=0,minpos=0,mindmg=0x7fffffff;i<DAMAGELOG_SIZE;i++){
				if(md->dmglog[i].id==pd->msd->bl.id)
					break;
				if(md->dmglog[i].id==0){
					minpos=i;
					mindmg=0;
				}
				else if(md->dmglog[i].dmg<mindmg){
					minpos=i;
					mindmg=md->dmglog[i].dmg;
				}
			}
			if(i<DAMAGELOG_SIZE)
				md->dmglog[i].dmg+=(damage*battle_config.pet_attack_exp_rate)/100;
			else {
				md->dmglog[minpos].id=pd->msd->bl.id;
				md->dmglog[minpos].dmg=(damage*battle_config.pet_attack_exp_rate)/100;
			}
		}
	}

	md->hp-=damage;

	if(md->hp>0){
		return 0;
	}

	// ----- �������玀�S���� -----

//	if(md->class == 1288 && map[md->bl.m].flag.gvg)
//		guild_gvg_break_empelium(md);

	map_freeblock_lock();
	mob_changestate(md,MS_DEAD,0);
	mobskill_use(md,gettick(),-1);	// ���S���X�L��

	memset(tmpsd,0,sizeof(tmpsd));
	memset(pt,0,sizeof(pt));

	max_hp = battle_get_max_hp(&md->bl);

	// map�O�ɏ������l�͌v�Z���珜���̂�
	// overkill���͖�������sum��max_hp�Ƃ͈Ⴄ


	for(i=0,count=0,mvp_damage=0;i<DAMAGELOG_SIZE;i++){
		if(md->dmglog[i].id==0)
			continue;
		tmpsd[i] = map_id2sd(md->dmglog[i].id);
		if(tmpsd[i] == NULL)
			continue;
		count++;
		if(tmpsd[i]->bl.m != md->bl.m || pc_isdead(tmpsd[i]))
			continue;

		if(mvp_damage<md->dmglog[i].dmg){
			third_sd = second_sd;
			second_sd = mvp_sd;
			mvp_sd=tmpsd[i];
			mvp_damage=md->dmglog[i].dmg;
		}
	}

	// �o���l�̕��z
	for(i=0;i<DAMAGELOG_SIZE;i++){
		int per,pid,base_exp,job_exp,flag=1;
		struct party *p;
		if(tmpsd[i]==NULL || tmpsd[i]->bl.m != md->bl.m)
			continue;

		per=md->dmglog[i].dmg*256*(9+((count > 6)? 6:count))/10/max_hp;
		if(per>512) per=512;
		if(per<1) per=1;
		base_exp=mob_db[md->class].base_exp*per/256;
		if(base_exp < 1) base_exp = 1;
		job_exp=mob_db[md->class].job_exp*per/256;
		if(job_exp < 1) job_exp = 1;

		if((pid=tmpsd[i]->status.party_id)>0){	// �p�[�e�B�ɓ����Ă���
			int j=0;
			for(j=0;j<pnum;j++)	// �����p�[�e�B���X�g�ɂ��邩�ǂ���
				if(pt[j].id==pid)
					break;
			if(j==pnum){	// ���Ȃ��Ƃ��͌������ǂ����m�F
				if((p=party_search(pid))!=NULL && p->exp!=0){
					pt[pnum].id=pid;
					pt[pnum].p=p;
					pt[pnum].base_exp=base_exp;
					pt[pnum].job_exp=job_exp;
					pnum++;
					flag=0;
				}
			}else{	// ����Ƃ��͌���
				pt[j].base_exp+=base_exp;
				pt[j].job_exp+=job_exp;
				flag=0;
			}
		}
		if(flag)	// �e������
			pc_gainexp(tmpsd[i],base_exp,job_exp);
	}
	// �������z
	for(i=0;i<pnum;i++)
		party_exp_share(pt[i].p,md->bl.m,pt[i].base_exp,pt[i].job_exp);

	// item drop
	if(!(type&1)) {
		for(i=0;i<8;i++){
			struct delay_item_drop *ditem;
			int drop_rate;

			if(mob_db[md->class].dropitem[i].nameid <= 0)
				continue;
			drop_rate = mob_db[md->class].dropitem[i].p;
			if(drop_rate <= 0 && battle_config.drop_rate0item)
				drop_rate = 1;
			if(drop_rate <= rand()%10000)
				continue;

			ditem=malloc(sizeof(*ditem));
			if(ditem==NULL){
				printf("out of memory : mob_damage\n");
				exit(1);
			}

			ditem->nameid = mob_db[md->class].dropitem[i].nameid;
			ditem->amount = 1;
			ditem->m = md->bl.m;
			ditem->x = md->bl.x;
			ditem->y = md->bl.y;
			ditem->first_sd = mvp_sd;
			ditem->second_sd = second_sd;
			ditem->third_sd = third_sd;
			add_timer(gettick()+500+i,mob_delay_item_drop,(int)ditem,0);
		}
		if(sd && sd->state.attack_type == BF_WEAPON) {
			for(i=0;i<sd->monster_drop_item_count;i++) {
				struct delay_item_drop *ditem;
				int race = battle_get_race(&md->bl);
				if(sd->monster_drop_itemid[i] <= 0)
					continue;
				if(sd->monster_drop_race[i] & (1<<race) || 
					(mob_db[md->class].mode & 0x20 && sd->monster_drop_race[i] & 1<<10) ||
					(!(mob_db[md->class].mode & 0x20) && sd->monster_drop_race[i] & 1<<11) ) {
					if(sd->monster_drop_itemrate[i] <= rand()%10000)
						continue;

					ditem=malloc(sizeof(*ditem));
					if(ditem==NULL){
						printf("out of memory : mob_damage\n");
						exit(1);
					}

					ditem->nameid = sd->monster_drop_itemid[i];
					ditem->amount = 1;
					ditem->m = md->bl.m;
					ditem->x = md->bl.x;
					ditem->y = md->bl.y;
					ditem->first_sd = mvp_sd;
					ditem->second_sd = second_sd;
					ditem->third_sd = third_sd;
					add_timer(gettick()+520+i,mob_delay_item_drop,(int)ditem,0);
				}
			}
			if(sd->get_zeny_num > 0)
				pc_getzeny(sd,mob_db[md->class].lv*10 + rand()%(sd->get_zeny_num+1));
		}
		if(md->lootitem) {
			for(i=0;i<md->lootitem_count;i++) {
				struct delay_item_drop2 *ditem;

				ditem=malloc(sizeof(*ditem));
				if(ditem==NULL){
					printf("out of memory : mob_damage\n");
					exit(1);
				}
				memcpy(&ditem->item_data,&md->lootitem[i],sizeof(md->lootitem[0]));
				ditem->m = md->bl.m;
				ditem->x = md->bl.x;
				ditem->y = md->bl.y;
				ditem->first_sd = mvp_sd;
				ditem->second_sd = second_sd;
				ditem->third_sd = third_sd;
				add_timer(gettick()+540+i,mob_delay_item_drop2,(int)ditem,0);
			}
		}
	}

	// mvp����
	if(mvp_sd && mob_db[md->class].mexp > 0 ){
		int j;
	//	int mexp = mob_db[md->class].mexp*(9+count)/10;
	// taulin: removing mvp effect and exp packets for now
	//	clif_mvp_effect(mvp_sd);					// �G�t�F�N�g
	//	clif_mvp_exp(mvp_sd,mexp);
	//	pc_gainexp(mvp_sd,mexp,0);
		for(j=0;j<3;j++){
			i = rand() % 3;
			struct item item;
	//		int ret;
			int drop_rate;
			if(mob_db[md->class].mvpitem[i].nameid <= 0)
				continue;
			drop_rate = mob_db[md->class].mvpitem[i].p;
			if(drop_rate <= 0 && battle_config.drop_rate0item)
				drop_rate = 1;
			if(drop_rate < battle_config.item_drop_mvp_min)
				drop_rate = battle_config.item_drop_mvp_min;
			if(drop_rate > battle_config.item_drop_mvp_max)
				drop_rate = battle_config.item_drop_mvp_max;
			if(drop_rate <= rand()%10000)
				continue;
			memset(&item,0,sizeof(item));
			item.nameid=mob_db[md->class].mvpitem[i].nameid;
			item.identify=!itemdb_isequip(item.nameid);
		// taulin: and have mvp items drop on ground
		//	clif_mvp_item(mvp_sd,item.nameid);
		//	if(mvp_sd->weight*2 > mvp_sd->max_weight)
				map_addflooritem(&item,1,mvp_sd->bl.m,mvp_sd->bl.x,mvp_sd->bl.y,mvp_sd,second_sd,third_sd,1);
		//	else if((ret = pc_additem(mvp_sd,&item,1))) {
		//		clif_additem(sd,0,0,ret);
		//		map_addflooritem(&item,1,mvp_sd->bl.m,mvp_sd->bl.x,mvp_sd->bl.y,mvp_sd,second_sd,third_sd,1);
		//	}
			break;
		}
	}

	// <Agit> NPC Event [OnAgitBreak]
	if(md->npc_event[0] && strcmp(((md->npc_event)+strlen(md->npc_event)-13),"::OnAgitBreak") == 0) {
		printf("MOB.C: Run NPC_Event[OnAgitBreak].\n");
		if (agit_flag == 1) //Call to Run NPC_Event[OnAgitBreak]
			guild_agit_break(md);
	}

	if(md->npc_event[0]){	// SCRIPT���s
//		if(battle_config.battle_log)
//			printf("mob_damage : run event : %s\n",md->npc_event);
		if(src && src->type == BL_PET)
			sd = ((struct pet_data *)src)->msd;
		if(sd == NULL) {
			if(mvp_sd != NULL)
				sd = mvp_sd;
			else {
				struct map_session_data *tmpsd;
				int i;
				for(i=0;i<fd_max;i++){
					if(session[i] && (tmpsd=session[i]->session_data) && tmpsd->state.auth) {
						if(md->bl.m == tmpsd->bl.m) {
							sd = tmpsd;
							break;
						}
					}
				}
			}
		}
		if(sd)
			npc_event(sd,md->npc_event,1);
	}

	clif_clearchar_area(&md->bl,1);
	map_delblock(&md->bl);
	mob_deleteslave(md);
	mob_setdelayspawn(md->bl.id);
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_class_change(struct mob_data *md,int class)
{
	unsigned int tick;
	int i,c,hp_rate,max_hp;
	if(md->bl.prev == NULL) return 0;

	max_hp = battle_get_max_hp(&md->bl);
	hp_rate = md->hp*100/max_hp;
	clif_mob_class_change(md,class);
	md->class = class;
	max_hp = battle_get_max_hp(&md->bl);
	md->hp = max_hp*hp_rate/100;
	if(md->hp > max_hp) md->hp = max_hp;
	else if(md->hp < 1) md->hp = 1;

	tick  = gettick();

	memcpy(md->name,mob_db[class].jname,16);
	memset(&md->state,0,sizeof(md->state));
	md->attacked_id = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;

	mob_changestate(md,MS_IDLE,0);
	skill_castcancel(&md->bl,0);
	md->state.skillstate = MSS_IDLE;
	md->last_thinktime = tick;
	md->next_walktime = tick+rand()%50+5000;
	md->attackabletime = tick;
	md->canmove_tick = tick;
	md->sg_count=0;

	for(i=0,c=tick-1000*3600*10;i<MAX_MOBSKILL;i++)
		md->skilldelay[i] = c;
	md->skillid=0;
	md->skilllv=0;

	if(md->lootitem == NULL && mob_db[class].mode&0x02) {
		md->lootitem=malloc(sizeof(struct item)*LOOTITEM_SIZE);
		if(md->lootitem==NULL){
			printf("mob_class_change: out of memory !\n");
			exit(1);
		}
	}

	skill_clear_unitgroup(&md->bl);
	skill_cleartimerskill(&md->bl);

	clif_clearchar_area(&md->bl,0);
	clif_spawnmob(md);

	return 0;
}

/*==========================================
 * mob��
 *------------------------------------------
 */
int mob_heal(struct mob_data *md,int heal)
{
	int max_hp = battle_get_max_hp(&md->bl);
	md->hp += heal;
	if( max_hp < md->hp )
		md->hp = max_hp;
	return 0;
}


/*==========================================
 * Added by RoVeRT
 *------------------------------------------
 */
int mob_warpslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md=(struct mob_data *)bl;
	int id,x,y;
	id=va_arg(ap,int);
	x=va_arg(ap,int);
	y=va_arg(ap,int);
	if( md->master_id==id ) {
		mob_warp(md,x,y,2);
	}
	return 0;
}

/*==========================================
 * Added by RoVeRT
 *------------------------------------------
 */
int mob_warpslave(struct mob_data *md,int x, int y)
{
printf("warp slave\n");
	map_foreachinarea(mob_warpslave_sub, md->bl.m,
		x-AREA_SIZE,y-AREA_SIZE,
		x+AREA_SIZE,y+AREA_SIZE,BL_MOB,
		md->bl.id, md->bl.x, md->bl.y );
	return 0;
}

/*==========================================
 * mob���[�v
 *------------------------------------------
 */
int mob_warp(struct mob_data *md,int x,int y,int type)
{
	int m,i=0,c,xs=0,ys=0,bx=x,by=y,lx=x,ly=y;
	if( md==NULL || md->bl.prev==NULL )
		return 0;

	if(type >= 0) {
		if(map[md->bl.m].flag.monster_noteleport)
			return 0;
		clif_clearchar_area(&md->bl,type);
	}
	skill_unit_out_all(&md->bl,gettick(),1);
	map_delblock(&md->bl);
	m=md->bl.m;

	if(bx>0 && by>0){	// �ʒu�w��̏ꍇ���͂X�Z����T��
		xs=ys=9;
	}

	while( ( x<0 || y<0 || (c=read_gat(m,x,y))==1) && (i++)<1000 ){
		if( xs>0 && ys>0 && i<250 ){	// �w��ʒu�t�߂̒T��
			x=bx+rand()%xs-xs/2;
			y=by+rand()%ys-ys/2;
		}else{			// ���S�����_���T��
			x=rand()%(map[m].xs-2)+1;
			y=rand()%(map[m].ys-2)+1;
		}
	}
	md->dir=0;
	if(i<1000){
		lx=md->bl.x;
		ly=md->bl.y;
		md->bl.x=x;
		md->bl.y=y;
	}else {
		if(battle_config.error_log)
			printf("MOB %d warp failed, class = %d\n",md->bl.id,md->class);
	}

	md->target_id=0;	// �^�Q����������
	md->state.targettype=NONE_ATTACKABLE;
	md->attacked_id=0;
	md->state.state=MS_IDLE;
	md->state.skillstate=MSS_IDLE;

	if(type>0 && i==1000) {
		if(battle_config.battle_log)
			printf("MOB %d warp to (%d,%d), class = %d\n",md->bl.id,x,y,md->class);
	}

	map_addblock(&md->bl);
	if(type>0) {
		clif_spawnmob(md);
		mob_warpslave(md,lx,ly);
	}

	return 0;
}

/*==========================================
 * ��ʓ��̎�芪���̐��v�Z�p(foreachinarea)
 *------------------------------------------
 */
int mob_countslave_sub(struct block_list *bl,va_list ap)
{
	int id,*c;
	id=va_arg(ap,int);
	c=va_arg(ap,int *);
	if( ((struct mob_data *)bl)->master_id==id )
		(*c)++;
	return 0;
}
/*==========================================
 * ��ʓ��̎�芪���̐��v�Z
 *------------------------------------------
 */
int mob_countslave(struct mob_data *md)
{
	int c=0;
	map_foreachinarea(mob_countslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs-1,map[md->bl.m].ys-1,
		BL_MOB,md->bl.id,&c);
	return c;
}
/*==========================================
 * �艺MOB����
 *------------------------------------------
 */
int mob_summonslave(struct mob_data *md2,int class,int amount,int flag)
{
	struct mob_data *md;
	int bx=md2->bl.x,by=md2->bl.y,m=md2->bl.m;

	if(class>2000)	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	for(;amount>0;amount--){
		int x=0,y=0,c=0,i=0;
		md=malloc(sizeof(struct mob_data));
		if(md==NULL){
			printf("mob_once_spawn: out of memory !\n");
			exit(1);
		}
		if(mob_db[class].mode&0x02) {
			md->lootitem=malloc(sizeof(struct item)*LOOTITEM_SIZE);
			if(md->lootitem==NULL){
				printf("mob_once_spawn: out of memory !\n");
				exit(1);
			}
		}
		else
			md->lootitem=NULL;

		while((x<=0 || y<=0 || (c=map_getcell(m,x,y))==1) && (i++)<50){
			x=rand()%9-4+bx;
			y=rand()%9-4+by;
		}
		if(i>=50){
			x=bx;
			y=by;
		}
	
		mob_spawn_dataset(md,"--ja--",class);
		md->bl.m=m;
		md->bl.x=x;
		md->bl.y=y;

		md->x0=x;
		md->y0=y;
		md->xs=0;
		md->ys=0;
		md->spawndelay1=-1;	// ��x�̂݃t���O
		md->spawndelay2=-1;	// ��x�̂݃t���O

		memset(md->npc_event,0,sizeof(md->npc_event));
		md->bl.type=BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

		if(flag)
			md->master_id=md2->bl.id;
	}
	return 0;
}

/*==========================================
 * ���������b�N���Ă���PC�̐��𐔂���(foreachclient)
 *------------------------------------------
 */
static int mob_counttargeted_sub(struct block_list *bl,va_list ap)
{
	int id,*c;
	struct block_list *src;
	id=va_arg(ap,int);
	c=va_arg(ap,int *);
	src=va_arg(ap,struct block_list *);
	if(id == bl->id || (src && id == src->id)) return 0;
	if(bl->type == BL_PC) {
		if(((struct map_session_data *)bl)->attacktarget == id && ((struct map_session_data *)bl)->attacktimer != -1)
			(*c)++;
	}
	else if(bl->type == BL_MOB) {
		if(((struct mob_data *)bl)->target_id == id && ((struct mob_data *)bl)->timer != -1 && ((struct mob_data *)bl)->state.state == MS_ATTACK)
			(*c)++;
	}
	else if(bl->type == BL_PET) {
		if(((struct pet_data *)bl)->target_id == id && ((struct pet_data *)bl)->timer != -1 && ((struct pet_data *)bl)->state.state == MS_ATTACK)
			(*c)++;
	}
	return 0;
}
/*==========================================
 * ���������b�N���Ă���PC�̐��𐔂���
 *------------------------------------------
 */
int mob_counttargeted(struct mob_data *md,struct block_list *src)
{
	int c=0;
	map_foreachinarea(mob_counttargeted_sub, md->bl.m,
		md->bl.x-AREA_SIZE,md->bl.y-AREA_SIZE,
		md->bl.x+AREA_SIZE,md->bl.y+AREA_SIZE,0,md->bl.id,&c,src);
	return c;
}

/*==========================================
 * friendhpltmaxrate sub
 *------------------------------------------
 */
static int mob_checkslavehp_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md=((struct mob_data *)bl);
	int id,*c,pcnt;
	id=va_arg(ap,int);
	c=va_arg(ap,int*);
	pcnt=va_arg(ap,int);
	
	if( md->master_id==id )
		if (md->hp < battle_get_max_hp(&md->bl)*pcnt/100)
			(*c)++;
	return 0;
}
/*==========================================
 * friendhpltmaxrate
 *------------------------------------------
 */
int mob_checkslavehp(struct mob_data *md,int pcnt)
{
	int c=0;
	map_foreachinarea(mob_checkslavehp_sub, md->bl.m,
		md->bl.x-AREA_SIZE,md->bl.y-AREA_SIZE,
		md->bl.x+AREA_SIZE,md->bl.y+AREA_SIZE,BL_MOB,md->bl.id,&c,pcnt);
	return c;
}

/*==========================================
 * friendstatuseq sub
 *------------------------------------------
 */
static int mob_checkslaveoption_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md=((struct mob_data *)bl);
	int id,*c,option;
	id=va_arg(ap,int);
	c=va_arg(ap,int*);
	option=va_arg(ap,int);
	if( md->master_id==id )
		if (md->opt2 == option)
			(*c)++;
	return 0;
}
/*==========================================
 * friendstatuseq
 *------------------------------------------
 */
int mob_checkslaveoption(struct mob_data *md,int option)
{
	int c=0;
	map_foreachinarea(mob_checkslaveoption_sub, md->bl.m,
		md->bl.x-AREA_SIZE,md->bl.y-AREA_SIZE,
		md->bl.x+AREA_SIZE,md->bl.y+AREA_SIZE,BL_MOB,md->bl.id,&c,option);
	return c;
}

//
// MOB�X�L��
//

/*==========================================
 * �X�L���g�p�i�r�������AID�w��j
 *------------------------------------------
 */
int mobskill_castend_id( int tid, unsigned int tick, int id,int data )
{
	struct mob_data* md=NULL;
	struct block_list *bl;
	int range;

	if( (md=(struct mob_data *)map_id2bl(id))==NULL ||
		md->bl.type!=BL_MOB || md->bl.prev==NULL)
		return 0;

	if( md->skilltimer != tid )	// �^�C�}ID�̊m�F
		return 0;

	md->skilltimer=-1;
	if(md->opt1>0 || md->sc_data[SC_DIVINA].timer != -1 || md->sc_data[SC_ROKISWEIL].timer != -1 ||
		md->sc_data[SC_STEELBODY].timer != -1)
		return 0;
	if(md->sc_data[SC_AUTOCOUNTER].timer != -1 && md->skillid != KN_AUTOCOUNTER) return 0;
	md->last_thinktime=tick + battle_get_adelay(&md->bl);

	bl=map_id2bl(md->skilltarget);
	if(bl==NULL || bl->prev==NULL)
		return 0;
	if(md->bl.m != bl->m)
		return 0;
	range = skill_get_range(md->skillid,md->skilllv);
	if(range < 0)
		range = (1 - range) + battle_get_range(&md->bl);
	if(range + battle_config.mob_skill_add_range < distance(md->bl.x,md->bl.y,bl->x,bl->y))
		return 0;
	md->skilldelay[md->skillidx]=tick;

	if(battle_config.mob_skill_log)
		printf("MOB skill castend skill=%d, class = %d\n",md->skillid,md->class);
	mob_stop_walking(md,0);

	switch( skill_get_nk(md->skillid) )
	{
	// �U���n/������΂��n
	case 0:	case 2:
		skill_castend_damage_id(&md->bl,bl,md->skillid,md->skilllv,tick,0);
		break;
	case 1:// �x���n
		if( (md->skillid==AL_HEAL || (md->skillid==ALL_RESURRECTION && bl->type != BL_PC)) && battle_check_undead(battle_get_race(bl),battle_get_elem_type(bl)) )
			skill_castend_damage_id(&md->bl,bl,md->skillid,md->skilllv,tick,0);
		else
			skill_castend_nodamage_id(&md->bl,bl,md->skillid,md->skilllv,tick,0);
		break;
	}


	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������A�ꏊ�w��j
 *------------------------------------------
 */
int mobskill_castend_pos( int tid, unsigned int tick, int id,int data )
{
	struct mob_data* md=NULL;
	int range;

	if( (md=(struct mob_data *)map_id2bl(id))==NULL ||
		md->bl.type!=BL_MOB || md->bl.prev==NULL )
		return 0;

	if( md->skilltimer != tid )	// �^�C�}ID�̊m�F
		return 0;

	md->skilltimer=-1;
	if(md->opt1>0 || md->sc_data[SC_DIVINA].timer != -1 || md->sc_data[SC_ROKISWEIL].timer != -1 ||
		md->sc_data[SC_STEELBODY].timer != -1 || md->sc_data[SC_AUTOCOUNTER].timer != -1)
		return 0;

	if(battle_config.monster_skill_reiteration == 0) {
		range = -1;
		switch(md->skillid) {
			case MG_SAFETYWALL:
			case WZ_FIREPILLAR:
			case HT_SKIDTRAP:
			case HT_LANDMINE:
			case HT_ANKLESNARE:
			case HT_SHOCKWAVE:
			case HT_SANDMAN:
			case HT_FLASHER:
			case HT_FREEZINGTRAP:
			case HT_BLASTMINE:
			case HT_CLAYMORETRAP:
			case HT_TALKIEBOX:
				range = 0;
				break;
			case AL_PNEUMA:
			case AL_WARP:
				range = 1;
				break;
		}
		if(range >= 0) {
			if(skill_check_unit_range(md->bl.m,md->skillx,md->skilly,range,md->skillid) > 0)
				return 0;
		}
	}

	range = skill_get_range(md->skillid,md->skilllv);
	if(range < 0)
		range = (1 - range) + battle_get_range(&md->bl);
	if(range + battle_config.mob_skill_add_range < distance(md->bl.x,md->bl.y,md->skillx,md->skilly))
		return 0;
	md->skilldelay[md->skillidx]=tick;

	if(battle_config.mob_skill_log)
		printf("MOB skill castend skill=%d, class = %d\n",md->skillid,md->class);
	mob_stop_walking(md,0);

	skill_castend_pos2(&md->bl,md->skillx,md->skilly,md->skillid,md->skilllv,tick,0);

	return 0;
}


/*==========================================
 * �X�L���g�p�i�r���J�n�AID�w��j
 *------------------------------------------
 */
int mobskill_use_id(struct mob_data *md,struct block_list *target,int skill_idx)
{
	int casttime,range;
	struct mob_skill *ms=&mob_db[md->class].skill[skill_idx];
	int skill_id=ms->skill_id, skill_lv=ms->skill_lv;

	if(target==NULL && (target=map_id2bl(md->target_id))==NULL)
		return 0;

	if( target->prev==NULL || md->bl.prev==NULL )
		return 0;

	// ���ق�ُ�
	if( md->opt1>0 || md->option&6 || md->sc_data[SC_DIVINA].timer!=-1 ||  md->sc_data[SC_ROKISWEIL].timer!=-1 ||
		md->sc_data[SC_AUTOCOUNTER].timer != -1 || md->sc_data[SC_STEELBODY].timer != -1)
		return 0;

	if(map[md->bl.m].flag.gvg && (skill_id == SM_ENDURE || skill_id == AL_TELEPORT || skill_id == AL_WARP ||
		skill_id == WZ_ICEWALL || skill_id == TF_BACKSLIDING))
		return 0;

	// �˒��Ə�Q���`�F�b�N
	range = skill_get_range(skill_id,skill_lv);
	if(range < 0)
		range = (1 - range) + battle_get_range(&md->bl);
	if(!battle_check_range(&md->bl,target,range))
		return 0;

//	casttime=skill_castfix(&md->bl, skill_get_cast( skill_id,skill_lv) );
//	delay=skill_delayfix(&md->bl, skill_get_delay( skill_id,skill_lv) );
//	sd->skillcastcancel=1;

	casttime=ms->casttime;
	md->state.skillcastcancel=ms->cancel;
	md->skilldelay[skill_idx]=gettick();

	if(battle_config.mob_skill_log)
		printf("MOB skill use target_id=%d skill=%d lv=%d cast=%d, class = %d\n",target->id,skill_id,skill_lv,casttime,md->class);

	if( casttime>0 ){ 	// �r�����K�v
		struct mob_data *md2;
		clif_skillcasting( &md->bl,
			md->bl.id, target->id, 0,0, skill_id,casttime);
	
		// �r�����������X�^�[
		if( target->type==BL_MOB && mob_db[(md2=(struct mob_data *)target)->class].mode&0x10 &&
			md2->state.state!=MS_ATTACK){
				md2->target_id=md->bl.id;
				md->state.targettype = ATTACKABLE;
				md2->min_chase=13;
		}
	}

	if( casttime<=0 )	// �r���̖������̂̓L�����Z������Ȃ�
		md->state.skillcastcancel=0;

	md->skilltarget	= target->id;
	md->skillx		= 0;
	md->skilly		= 0;
	md->skillid		= skill_id;
	md->skilllv		= skill_lv;
	md->skillidx	= skill_idx;

	if( casttime>0 ){
		md->skilltimer =
			add_timer( gettick()+casttime, mobskill_castend_id, md->bl.id, 0 );
	}else{
		md->skilltimer = -1;
		mobskill_castend_id(md->skilltimer,gettick(),md->bl.id, 0);
	}

//	sd->canmove_tick=gettick()+casttime+delay;

	return 1;
}
/*==========================================
 * �X�L���g�p�i�ꏊ�w��j
 *------------------------------------------
 */
int mobskill_use_pos( struct mob_data *md,
	int skill_x, int skill_y, int skill_idx)
{
	int casttime=0,range;
	struct mob_skill *ms=&mob_db[md->class].skill[skill_idx];
	struct block_list bl;
	int skill_id=ms->skill_id, skill_lv=ms->skill_lv;

	if( md->bl.prev==NULL )
		return 0;

	if( md->opt1>0 || md->option&6 || md->sc_data[SC_DIVINA].timer!=-1 ||  md->sc_data[SC_ROKISWEIL].timer!=-1 ||
		md->sc_data[SC_AUTOCOUNTER].timer != -1 || md->sc_data[SC_STEELBODY].timer != -1)
		return 0;	// �ُ�Ⓘ�قȂ�

	if(map[md->bl.m].flag.gvg && (skill_id == SM_ENDURE || skill_id == AL_TELEPORT || skill_id == AL_WARP ||
		skill_id == WZ_ICEWALL || skill_id == TF_BACKSLIDING))
		return 0;

	// �˒��Ə�Q���`�F�b�N
	bl.type = BL_NUL;
	bl.m = md->bl.m;
	bl.x = skill_x;
	bl.y = skill_y;
	range = skill_get_range(skill_id,skill_lv);
	if(range < 0)
		range = (1 - range) + battle_get_range(&md->bl);
	if(!battle_check_range(&md->bl,&bl,range))
		return 0;

//	casttime=skill_castfix(&sd->bl, skill_get_cast( skill_id,skill_lv) );
//	delay=skill_delayfix(&sd->bl, skill_get_delay( skill_id,skill_lv) );
	casttime=ms->casttime;
	md->skilldelay[skill_idx]=gettick();
	md->state.skillcastcancel=ms->cancel;

	if(battle_config.mob_skill_log)
		printf("MOB skill use target_pos=(%d,%d) skill=%d lv=%d cast=%d, class = %d\n",
			skill_x,skill_y,skill_id,skill_lv,casttime,md->class);

	if( casttime>0 )	// �r�����K�v
		clif_skillcasting( &md->bl,
			md->bl.id, 0, skill_x,skill_y, skill_id,casttime);

	if( casttime<=0 )	// �r���̖������̂̓L�����Z������Ȃ�
		md->state.skillcastcancel=0;


	md->skillx		= skill_x;
	md->skilly		= skill_y;
	md->skilltarget	= 0;
	md->skillid		= skill_id;
	md->skilllv		= skill_lv;
	md->skillidx	= skill_idx;
	if( casttime>0 ){
		md->skilltimer =
			add_timer( gettick()+casttime, mobskill_castend_pos, md->bl.id, 0 );
	}else{
		md->skilltimer = -1;
		mobskill_castend_pos(md->skilltimer,gettick(),md->bl.id, 0);
	}

//	sd->canmove_tick=gettick()+casttime+delay;

	return 1;
}

/*==========================================
 * �X�L���g�p����
 *------------------------------------------
 */
int mobskill_use(struct mob_data *md,unsigned int tick,int event)
{
	struct mob_skill *ms=mob_db[md->class].skill;
//	struct block_list *target=NULL;
	int i,max_hp = battle_get_max_hp(&md->bl);

	if(battle_config.mob_skill_use == 0 || md->skilltimer != -1)
		return 0;

	for(i=0;i<mob_db[md->class].maxskill;i++){
		int c2=ms[i].cond2,flag=0;

		// �f�B���C��
		if( DIFF_TICK(tick,md->skilldelay[i])<ms[i].delay )
			continue;

		// ��Ԕ���
		if( ms[i].state>=0 && ms[i].state!=md->state.skillstate )
			continue;

		// ��������
		flag=(event==ms[i].cond1);
		if(!flag){
			switch( ms[i].cond1 ){
			case MSC_ALWAYS:
				flag=1; break;
			case MSC_MYHPLTMAXRATE:		// HP< maxhp%
				flag=( md->hp < max_hp*c2/100 ); break;
			case MSC_FRIENDHPLTMAXRATE:
				flag=( mob_checkslavehp(md, c2) > 0 ); break;	// any slave HP< maxhp%
			case MSC_MYSTATUSEQ:
				flag=( md->opt2 == c2 ); break;	// opt2 = num
			case MSC_FRIENDSTATUSEQ:
				flag=( mob_checkslaveoption(md, c2) > 0 ); break;	// any slave opt2 = num			
			case MSC_SLAVELT:		// slave < num
				flag=( mob_countslave(md) < c2 ); break;
			case MSC_ATTACKPCGT:	// attack pc > num
				flag=( mob_counttargeted(md,NULL) > c2 ); break;
			case MSC_SLAVELE:		// slave <= num
				flag=( mob_countslave(md) <= c2 ); break;
			case MSC_ATTACKPCGE:	// attack pc >= num
				flag=( mob_counttargeted(md,NULL) >= c2 ); break;
			case MSC_SKILLUSED:		// specificated skill used
				flag=( (event&0xffff)==MSC_SKILLUSED && ((event>>16)==c2 || c2==0)); break;
			}
		}

		// �m������
		if( flag && rand()%10000 < ms[i].permillage ){

			if( skill_get_inf(ms[i].skill_id)&2 ){
				// �ꏊ�w��
				struct block_list *bl;
				int x=0,y=0;
				if( ms[i].target<2 ){
					bl=((ms[i].target==MST_TARGET)?map_id2bl(md->target_id):&md->bl);
					if(bl!=NULL){
						x=bl->x; y=bl->y;
					}
				}
				if( x<=0 || y<=0 )
					continue;

				if(!mobskill_use_pos(md,x,y,i))
					return 0;

			}else{
				// ID�w��
				if( ms[i].target<2 )
					if(!mobskill_use_id(md,((ms[i].target==MST_TARGET)?NULL:&md->bl),i))
						return 0;
			}
			return 1;
		}
	}

	return 0;
}
/*==========================================
 * �X�L���g�p�C�x���g����
 *------------------------------------------
 */
int mobskill_event(struct mob_data *md,int flag)
{
	if(flag==-1 && mobskill_use(md,gettick(),MSC_CASTTARGETED))
		return 1;
	if( (flag&BF_SHORT) && mobskill_use(md,gettick(),MSC_CLOSEDATTACKED))
		return 1;
	if( (flag&BF_LONG) && mobskill_use(md,gettick(),MSC_LONGRANGEATTACKED))
		return 1;
	return 0;
}
/*==========================================
 * �X�L���p�^�C�}�[�폜
 *------------------------------------------
 */
int mobskill_deltimer(struct mob_data *md )
{
	if( md->skilltimer!=-1 ){
		if( skill_get_inf( md->skillid )&2 )
			delete_timer( md->skilltimer, mobskill_castend_pos );
		else
			delete_timer( md->skilltimer, mobskill_castend_id );
		md->skilltimer=-1;
	}
	return 0;
}
//
// ������
//
/*==========================================
 * ���ݒ�mob���g��ꂽ�̂Ŏb�菉���l�ݒ�
 *------------------------------------------
 */
static int mob_makedummymobdb(int class)
{
	int i;

	sprintf(mob_db[class].name,"mob%d",class);
	sprintf(mob_db[class].jname,"mob%d",class);
	mob_db[class].lv=1;
	mob_db[class].max_hp=1000;
	mob_db[class].max_sp=1;
	mob_db[class].base_exp=2;
	mob_db[class].job_exp=0;
	mob_db[class].range=1;
	mob_db[class].atk1=7;
	mob_db[class].atk2=10;
	mob_db[class].def=0;
	mob_db[class].mdef=0;
	mob_db[class].str=1;
	mob_db[class].agi=1;
	mob_db[class].vit=1;
	mob_db[class].int_=1;
	mob_db[class].dex=6;
	mob_db[class].luk=2;
	mob_db[class].range2=10;
	mob_db[class].range3=10;
	mob_db[class].size=0;
	mob_db[class].race=0;
	mob_db[class].element=0;
	mob_db[class].mode=0;
	mob_db[class].speed=300;
	mob_db[class].adelay=1000;
	mob_db[class].amotion=500;
	mob_db[class].dmotion=500;
	mob_db[class].dropitem[0].nameid=909;	// Jellopy
	mob_db[class].dropitem[0].p=1000;
	for(i=1;i<8;i++){
		mob_db[class].dropitem[i].nameid=0;
		mob_db[class].dropitem[i].p=0;
	}
	// Item1,Item2
	mob_db[class].mexp=0;
	mob_db[class].mexpper=0;
	for(i=0;i<3;i++){
		mob_db[class].mvpitem[i].nameid=0;
		mob_db[class].mvpitem[i].p=0;
	}
	for(i=0;i<MAX_RANDOMMONSTER;i++)
		mob_db[class].summonper[i]=0;
	return 0;
}

/*==========================================
 * db/mob_db.txt�ǂݍ���
 *------------------------------------------
 */
static int mob_readdb(void)
{
	FILE *fp;
	char line[1024];
	char *filename[]={ "db/mob_db.txt","db/mob_db2.txt" };
	int i;
	
	memset(mob_db,0,sizeof(mob_db));
	
	for(i=0;i<2;i++){

		fp=fopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			printf("can't read %s\n",filename[i]);
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class,i;
			char *str[55],*p,*np;
	
			if(line[0] == '/' && line[1] == '/')
				continue;
	
			for(i=0,p=line;i<55;i++){
				if((np=strchr(p,','))!=NULL){
					str[i]=p;
					*np=0;
					p=np+1;
				} else
					str[i]=p;
			}
	
			class=atoi(str[0]);
			if(class>2000)
				continue;
	
			mob_db[class].view_class=class;
			memcpy(mob_db[class].name,str[1],16);
			memcpy(mob_db[class].jname,str[2],16);
			mob_db[class].lv=atoi(str[3]);
			mob_db[class].max_hp=atoi(str[4]);
			mob_db[class].max_sp=atoi(str[5]);
			mob_db[class].base_exp=atoi(str[6])*
					battle_config.base_exp_rate/100;
			if(mob_db[class].base_exp <= 0)
				mob_db[class].base_exp=1;
//			if(mob_db[class].job_exp <= 0)		// taulin
			mob_db[class].job_exp=0;		// no job exp in alpha
			mob_db[class].range=atoi(str[8]);
			mob_db[class].atk1=atoi(str[9]);
			mob_db[class].atk2=atoi(str[10]);
			mob_db[class].def=atoi(str[11]);
			mob_db[class].mdef=atoi(str[12]);
			mob_db[class].str=atoi(str[13]);
			mob_db[class].agi=atoi(str[14]);
			mob_db[class].vit=atoi(str[15]);
			mob_db[class].int_=atoi(str[16]);
			mob_db[class].dex=atoi(str[17]);
			mob_db[class].luk=atoi(str[18]);
			mob_db[class].range2=atoi(str[19]);
			mob_db[class].range3=atoi(str[20]);
			mob_db[class].size=atoi(str[21]);
			mob_db[class].race=atoi(str[22]);
			mob_db[class].element=atoi(str[23]);
			mob_db[class].mode=atoi(str[24]);
			mob_db[class].speed=atoi(str[25]);
			mob_db[class].adelay=atoi(str[26]);
			mob_db[class].amotion=atoi(str[27]);
			mob_db[class].dmotion=atoi(str[28]);
	
			for(i=0;i<8;i++){
				int rate = 0,type,ratemin,ratemax;
				mob_db[class].dropitem[i].nameid=atoi(str[29+i*2]);
				type = itemdb_type(mob_db[class].dropitem[i].nameid);
				if (type == 4 || type == 5 ) {
					rate = battle_config.item_rate_equip;
					ratemin = battle_config.item_drop_equip_min;
					ratemax = battle_config.item_drop_equip_max;
				}
				else if (type == 6) {
					rate = battle_config.item_rate_card;
					ratemin = battle_config.item_drop_card_min;
					ratemax = battle_config.item_drop_card_max;
				}
				else {
					rate = battle_config.item_rate_common;
					ratemin = battle_config.item_drop_common_min;
					ratemax = battle_config.item_drop_common_max;
				}
				rate = (rate / 100) * atoi(str[30+i*2]);
				rate = (rate < ratemin)? ratemin: (rate > ratemax)? ratemax: rate;
				mob_db[class].dropitem[i].p = rate;
			}
			// Item1,Item2
			mob_db[class].mexp=atoi(str[47])*battle_config.mvp_exp_rate/100;
			mob_db[class].mexpper=atoi(str[48]);
			for(i=0;i<3;i++){
				mob_db[class].mvpitem[i].nameid=atoi(str[49+i*2]);
				mob_db[class].mvpitem[i].p=atoi(str[50+i*2])*battle_config.mvp_item_rate/100;
			}
			for(i=0;i<MAX_RANDOMMONSTER;i++)
				mob_db[class].summonper[i]=0;
			mob_db[class].maxskill=0;
		}
		fclose(fp);
		printf("read %s done\n",filename[i]);
	}
	return 0;
}

/*==========================================
 * MOB�\���O���t�B�b�N�̕ύX
 *------------------------------------------
 */
static int mob_readdb_mobavail(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int class,j,k;
	char *str[10],*p;
	
	if( (fp=fopen("db/mob_avail.txt","r"))==NULL ){
		printf("can't read db/mob_avail.txt\n");
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

		class=atoi(str[0]);
		
		if(class<=1000 || class>2000)	// �l���ُ�Ȃ珈�����Ȃ��B
			continue;
		k=atoi(str[1]);
		if(k >= 0) {
			mob_db[class].view_class=k;
		}
		ln++;
	}
	fclose(fp);
	printf("read db/mob_avail.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * �����_�������X�^�[�f�[�^�̓ǂݍ���
 *------------------------------------------
 */
static int mob_read_randommonster(void)
{
	FILE *fp;
	char line[1024];
	char *str[10],*p;
	int i,j;

	const char* mobfile[] = {
		"db/mob_branch.txt",
		"db/mob_poring.txt",
		"db/mob_boss.txt" };

	for(i=0;i<MAX_RANDOMMONSTER;i++){
		mob_db[0].summonper[i] = 1002;	// �ݒ肵�Y�ꂽ�ꍇ�̓|�������o��悤�ɂ��Ă���
		fp=fopen(mobfile[i],"r");
		if(fp==NULL){
			printf("can't read %s\n",mobfile[i]);
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class,per;
			if(line[0] == '/' && line[1] == '/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			if(str[0]==NULL || str[2]==NULL)
				continue;

			class = atoi(str[0]);
			per=atoi(str[2]);
			if((class>1000 && class<=2000) || class==0)
				mob_db[class].summonper[i]=per;
		}
		fclose(fp);
		printf("read %s done\n",mobfile[i]);
	}
	return 0;
}
/*==========================================
 * db/mob_skill_db.txt�ǂݍ���
 *------------------------------------------
 */
static int mob_readskilldb(void)
{
	FILE *fp;
	char line[1024];
	int i;

	static struct {
		char str[32];
		int id;
	} cond1[] = {
		{	"always",			MSC_ALWAYS				},
		{	"myhpltmaxrate",	MSC_MYHPLTMAXRATE		},
		{	"friendhpltmaxrate",MSC_FRIENDHPLTMAXRATE	},
		{	"mystatuseq",		MSC_MYSTATUSEQ			},
		{	"mystatusne",		MSC_MYSTATUSNE			},
		{	"friendstatuseq",	MSC_FRIENDSTATUSEQ		},
		{	"friendstatusne",	MSC_FRIENDSTATUSNE		},
		{	"attackpcgt",		MSC_ATTACKPCGT			},
		{	"attackpcge",		MSC_ATTACKPCGE			},
		{	"slavelt",			MSC_SLAVELT				},
		{	"slavele",			MSC_SLAVELE				},
		{	"closedattacked",	MSC_CLOSEDATTACKED		},
		{	"longrangeattacked",MSC_LONGRANGEATTACKED	},
		{	"skillused",		MSC_SKILLUSED			},
		{	"casttargeted",		MSC_CASTTARGETED		},
	};
	int x;
	char *filename[]={ "db/mob_skill_db.txt","db/mob_skill_db2.txt" };

	for(x=0;x<2;x++){
	
		fp=fopen(filename[x],"r");
		if(fp==NULL){
			if(x==0)
				printf("can't read %s\n",filename[x]);
			continue;
		}
		while(fgets(line,1020,fp)){
			char *sp[16],*p;
			int mob_id;
			struct mob_skill *ms;
			int j=0;
	
			if(line[0] == '/' && line[1] == '/')
				continue;
	
			memset(sp,0,sizeof(sp));
			for(i=0,p=line;i<13 && p;i++){
				sp[i]=p;
				if((p=strchr(p,','))!=NULL)
					*p++=0;
			}
			if( (mob_id=atoi(sp[0]))<=0 )
				continue;
			
			if( strcmp(sp[1],"clear")==0 ){
				memset(mob_db[mob_id].skill,0,sizeof(mob_db[mob_id].skill));
				mob_db[mob_id].maxskill=0;
				continue;
			}
		
			for(i=0;i<MAX_MOBSKILL;i++)
				if( (ms=&mob_db[mob_id].skill[i])->skill_id == 0)
					break;
			if(i==MAX_MOBSKILL){
				printf("mob_skill: readdb: too many skill ! [%s] in %d[%s]\n",
					sp[1],mob_id,mob_db[mob_id].jname);
				continue;
			}
		
			ms->state=atoi(sp[2]);
			if( strcmp(sp[2],"any")==0 ) ms->state=-1;
			if( strcmp(sp[2],"idle")==0 ) ms->state=MSS_IDLE;
			if( strcmp(sp[2],"walk")==0 ) ms->state=MSS_WALK;
			if( strcmp(sp[2],"attack")==0 ) ms->state=MSS_ATTACK;
			if( strcmp(sp[2],"dead")==0 ) ms->state=MSS_DEAD;
			if( strcmp(sp[2],"loot")==0 ) ms->state=MSS_LOOT;
			if( strcmp(sp[2],"chase")==0 ) ms->state=MSS_CHASE;
			ms->skill_id=atoi(sp[3]);
			ms->skill_lv=atoi(sp[4]);
			ms->permillage=atoi(sp[5]);
			ms->casttime=atoi(sp[6]);
			ms->delay=atoi(sp[7]);
			ms->cancel=atoi(sp[8]);
			if( strcmp(sp[8],"yes")==0 ) ms->cancel=1;
			ms->target=atoi(sp[9]);
			if( strcmp(sp[9],"self")==0 )ms->target=MST_SELF;
			if( strcmp(sp[9],"target")==0 )ms->target=MST_TARGET;
			ms->cond1=-1;
			for(j=0;j<sizeof(cond1)/sizeof(cond1[0]);j++){
				if( strcmp(sp[10],cond1[j].str)==0)
					ms->cond1=cond1[j].id;
			}
			ms->cond2=atoi(sp[11]);
			ms->val1=atoi(sp[12]);
			mob_db[mob_id].maxskill=i+1;
		}
		fclose(fp);
		printf("read %s done\n",filename[x]);
	}
	return 0;
}
/*==========================================
 * mob���菉����
 *------------------------------------------
 */
int do_init_mob(void)
{
	mob_readdb();
	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();

	add_timer_func_list(mob_timer,"mob_timer");
	add_timer_func_list(mob_delayspawn,"mob_delayspawn");
	add_timer_func_list(mob_delay_item_drop,"mob_delay_item_drop");
	add_timer_func_list(mob_delay_item_drop2,"mob_delay_item_drop2");
	add_timer_func_list(mob_ai_hard,"mob_ai_hard");
	add_timer_func_list(mob_ai_lazy,"mob_ai_lazy");
	add_timer_func_list(mobskill_castend_id,"mobskill_castend_id");
	add_timer_func_list(mobskill_castend_pos,"mobskill_castend_pos");
	add_timer_interval(gettick()+MIN_MOBTHINKTIME,mob_ai_hard,0,0,MIN_MOBTHINKTIME);
	add_timer_interval(gettick()+MIN_MOBTHINKTIME*10,mob_ai_lazy,0,0,MIN_MOBTHINKTIME*10);

	return 0;
}
