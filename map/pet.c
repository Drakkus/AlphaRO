#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "timer.h"
#include "pc.h"
#include "map.h"
#include "intif.h"
#include "clif.h"
#include "chrif.h"
#include "socket.h"
#include "pet.h"
#include "itemdb.h"
#include "battle.h"
#include "mob.h"
#include "npc.h"
#include "script.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MIN_PETTHINKTIME 100

struct pet_db pet_db[MAX_PET_DB];

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

static int pet_timer(int tid,unsigned int tick,int id,int data);
static int pet_walktoxy_sub(struct pet_data *pd);

static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

static int calc_next_walk_step(struct pet_data *pd)
{
	if(pd->walkpath.path_pos>=pd->walkpath.path_len)
		return -1;
	if(pd->walkpath.path[pd->walkpath.path_pos]&1)
		return pd->speed*14/10;
	return pd->speed;
}

static int pet_performance_val(struct map_session_data *sd)
{
	if(sd->pet.intimate > 900)
		return (sd->petDB->s_perfor > 0)? 4:3;
	else if(sd->pet.intimate > 750)
		return 2;
	else
		return 1;
}

int pet_hungry_val(struct map_session_data *sd)
{
	if(sd->pet.hungry > 90)
		return 4;
	else if(sd->pet.hungry > 75)
		return 3;
	else if(sd->pet.hungry > 25)
		return 2;
	else if(sd->pet.hungry > 10)
		return 1;
	else
		return 0;
}

static int pet_can_reach(struct pet_data *pd,int x,int y)
{
	struct walkpath_data wpd;

	if( pd->bl.x==x && pd->bl.y==y )	// �����}�X
		return 1;

	// ��Q������
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,pd->bl.m,pd->bl.x,pd->bl.y,x,y,0)!=-1)?1:0;
}

static int pet_calc_pos(struct pet_data *pd,int tx,int ty,int dir)
{
	int x,y,dx,dy;
	int i,j=0,k;

	pd->to_x = tx;
	pd->to_y = ty;

	if(dir >= 0 && dir < 8) {
		dx = -dirx[dir]*2;
		dy = -diry[dir]*2;
		x = tx + dx;
		y = ty + dy;
		if(!(j=pet_can_reach(pd,x,y))) {
			if(dx > 0) x--;
			else if(dx < 0) x++;
			if(dy > 0) y--;
			else if(dy < 0) y++;
			if(!(j=pet_can_reach(pd,x,y))) {
				for(i=0;i<12;i++) {
					k = rand()%8;
					dx = -dirx[k]*2;
					dy = -diry[k]*2;
					x = tx + dx;
					y = ty + dy;
					if((j=pet_can_reach(pd,x,y)))
						break;
					else {
						if(dx > 0) x--;
						else if(dx < 0) x++;
						if(dy > 0) y--;
						else if(dy < 0) y++;
						if((j=pet_can_reach(pd,x,y)))
							break;
					}
				}
				if(!j) {
					x = tx;
					y = ty;
					if(!pet_can_reach(pd,x,y))
						return 1;
				}
			}
		}
	}
	else
		return 1;

	pd->to_x = x;
	pd->to_y = y;
	return 0;
}

static int pet_attack(struct pet_data *pd,unsigned int tick,int data)
{
	struct mob_data *md;
	int mode,race,range;

	pd->state.state=MS_IDLE;

	md=(struct mob_data *)map_id2bl(pd->target_id);
	if(md == NULL || md->bl.type != BL_MOB || pd->bl.m != md->bl.m || md->bl.prev == NULL ||
		distance(pd->bl.x,pd->bl.y,md->bl.x,md->bl.y) > 13) {
		pd->target_id=0;
		return 0;
	}

	mode=mob_db[pd->class].mode;
	race=mob_db[pd->class].race;
	if(mob_db[pd->class].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race != 4 && race != 6) ) {
		pd->target_id=0;
		return 0;
	}

	range = mob_db[pd->class].range + 1;
	if(distance(pd->bl.x,pd->bl.y,md->bl.x,md->bl.y) > range){
		pd->state.state=MS_IDLE;
		return 0;
	}
	pd->dir=map_calc_dir(&pd->bl, md->bl.x,md->bl.y );

	pd->to_x = pd->bl.x;
	pd->to_y = pd->bl.y;
	clif_fixpetpos(pd);

	battle_weapon_attack(&pd->bl,&md->bl,tick,0);

	pd->attackabletime = tick + battle_get_adelay(&pd->bl);

	pd->timer=add_timer(pd->attackabletime,pet_timer,pd->bl.id,0);
	pd->state.state=MS_ATTACK;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pet_walk(struct pet_data *pd,unsigned int tick,int data)
{
	int moveblock;
	int i,ctype;
	int x,y,dx,dy;

	pd->state.state=MS_IDLE;
	if(pd->walkpath.path_pos >= pd->walkpath.path_len || pd->walkpath.path_pos != data)
		return 0;

	pd->walkpath.path_half ^= 1;
	if(pd->walkpath.path_half==0){
		pd->walkpath.path_pos++;
		if(pd->state.change_walk_target){
			pet_walktoxy_sub(pd);
			return 0;
		}
	}
	else {
		if(pd->walkpath.path[pd->walkpath.path_pos] >= 8)
			return 1;

		x = pd->bl.x;
		y = pd->bl.y;
/*		ctype = map_getcell(pd->bl.m,x,y);
		if(ctype == 1 || ctype == 5) {
			pet_stop_walking(pd,1);
			return 0;
		}*/
		pd->dir=pd->walkpath.path[pd->walkpath.path_pos];
		dx = dirx[pd->dir];
		dy = diry[pd->dir];

		ctype = map_getcell(pd->bl.m,x+dx,y+dy);
		if(ctype == 1 || ctype == 5) {
			pet_walktoxy_sub(pd);
			return 0;
		}

		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		pd->state.state=MS_WALK;
		map_foreachinmovearea(clif_petoutsight,pd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,pd);

		x += dx;
		y += dy;

		if(moveblock) map_delblock(&pd->bl);
		pd->bl.x = x;
		pd->bl.y = y;
		if(moveblock) map_addblock(&pd->bl);

		map_foreachinmovearea(clif_petinsight,pd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,pd);
		pd->state.state=MS_IDLE;
	}
	if((i=calc_next_walk_step(pd))>0){
		i = i>>1;
		if(i < 1 && pd->walkpath.path_half == 0)
			i = 1;
		pd->timer=add_timer(tick+i,pet_timer,pd->bl.id,pd->walkpath.path_pos);
		pd->state.state=MS_WALK;

		if(pd->walkpath.path_pos >= pd->walkpath.path_len)
			clif_fixpetpos(pd);
	}
	return 0;
}

int pet_stopattack(struct pet_data *pd)
{
	pd->target_id=0;
	if(pd->state.state == MS_ATTACK)
		pet_changestate(pd,MS_IDLE,0);

	return 0;
}

int pet_target_check(struct map_session_data *sd,struct block_list *bl,int type)
{
	struct pet_data *pd = sd->pd;
	struct mob_data *md;
	int rate,mode,race;

	if(bl && pd && bl->type == BL_MOB && sd->pet.intimate > 900 && pd->class != battle_get_class(bl)
		&& pd->state.state != MS_DELAY) {
		mode=mob_db[pd->class].mode;
		race=mob_db[pd->class].race;
		md=(struct mob_data *)bl;
		if(md->bl.type != BL_MOB || pd->bl.m != md->bl.m || md->bl.prev == NULL ||
			distance(pd->bl.x,pd->bl.y,md->bl.x,md->bl.y) > 13)
			return 0;
		if(mob_db[pd->class].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race!=4 && race!=6) )
			return 0;
		if(!type) {
			rate = sd->petDB->attack_rate;
			rate = rate * (150 - (sd->pet.intimate - 1000))/100;
			if(battle_config.pet_support_rate != 100)
				rate = rate*battle_config.pet_support_rate/100;
			if(sd->petDB->attack_rate > 0 && rate <= 0)
				rate = 1;
		}
		else {
			rate = sd->petDB->defence_attack_rate;
			rate = rate * (150 - (sd->pet.intimate - 1000))/100;
			if(battle_config.pet_support_rate != 100)
				rate = rate*battle_config.pet_support_rate/100;
			if(sd->petDB->defence_attack_rate > 0 && rate <= 0)
				rate = 1;
		}
		if(rand()%10000 < rate) {
			if(pd->target_id == 0 || rand()%10000 < sd->petDB->change_target_rate)
				pd->target_id = bl->id;
		}
	}
	return 0;
}

int pet_changestate(struct pet_data *pd,int state,int type)
{
	unsigned int tick;
	int i;

	if(pd->timer != -1)
		delete_timer(pd->timer,pet_timer);
	pd->timer=-1;
	pd->state.state=state;

	switch(state) {
		case MS_WALK:
			if((i=calc_next_walk_step(pd)) > 0){
				i = i>>2;
				pd->timer=add_timer(gettick()+i,pet_timer,pd->bl.id,0);
			} else
				pd->state.state=MS_IDLE;
			break;
		case MS_ATTACK:
			tick = gettick();
			i=DIFF_TICK(pd->attackabletime,tick);
			if(i>0 && i<2000)
				pd->timer=add_timer(pd->attackabletime,pet_timer,pd->bl.id,0);
			else
				pd->timer=add_timer(tick+1,pet_timer,pd->bl.id,0);
			break;
		case MS_DELAY:
			pd->timer=add_timer(gettick()+type,pet_timer,pd->bl.id,0);
			break;
	}

	return 0;
}

static int pet_timer(int tid,unsigned int tick,int id,int data)
{
	struct pet_data *pd;

	pd=(struct pet_data*)map_id2bl(id);
	if(pd == NULL || pd->bl.type != BL_PET)
		return 1;

	if(pd->timer != tid){
		if(battle_config.error_log)
			printf("pet_timer %d != %d\n",pd->timer,tid);
		return 0;
	}
	pd->timer=-1;

	if(pd->bl.prev == NULL)
		return 1;

	switch(pd->state.state){
		case MS_WALK:
			pet_walk(pd,tick,data);
			break;
		case MS_ATTACK:
			pet_attack(pd,tick,data);
			break;
		case MS_DELAY:
			pet_changestate(pd,MS_IDLE,0);
			break;
		default:
			if(battle_config.error_log)
				printf("pet_timer : %d ?\n",pd->state.state);
			break;
	}

	return 0;
}

static int pet_walktoxy_sub(struct pet_data *pd)
{
	struct walkpath_data wpd;

	if(path_search(&wpd,pd->bl.m,pd->bl.x,pd->bl.y,pd->to_x,pd->to_y,0))
		return 1;
	memcpy(&pd->walkpath,&wpd,sizeof(wpd));

	pd->state.change_walk_target=0;
	pet_changestate(pd,MS_WALK,0);
	clif_movepet(pd);
//	if(battle_config.etc_log)
//		printf("walkstart\n");

	return 0;
}

int pet_walktoxy(struct pet_data *pd,int x,int y)
{
	struct walkpath_data wpd;

	if(pd->state.state == MS_WALK && path_search(&wpd,pd->bl.m,pd->bl.x,pd->bl.y,x,y,0))
		return 1;

	pd->to_x=x;
	pd->to_y=y;

	if(pd->state.state == MS_WALK) {
		pd->state.change_walk_target=1;
	} else {
		return pet_walktoxy_sub(pd);
	}

	return 0;
}

int pet_stop_walking(struct pet_data *pd,int type)
{
	if(pd->state.state == MS_WALK) {
		pd->walkpath.path_len=0;
		pd->to_x=pd->bl.x;
		pd->to_y=pd->bl.y;
	}
	if(type&0x01)
		clif_fixpetpos(pd);
	if(type&~0xff)
		pet_changestate(pd,MS_DELAY,type>>8);
	else
		pet_changestate(pd,MS_IDLE,0);

	return 0;
}

static int pet_hungry(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int interval,t;

	sd=map_id2sd(id);
	if(sd==NULL)
		return 1;

	if(sd->pet_hungry_timer != tid){
		if(battle_config.error_log)
			printf("pet_hungry_timer %d != %d\n",sd->pet_hungry_timer,tid);
		return 0;
	}
	sd->pet_hungry_timer = -1;
	if(!sd->status.pet_id || !sd->pd || !sd->petDB)
		return 1;

	sd->pet.hungry--;
	t = sd->pet.intimate;
	if(sd->pet.hungry < 0) {
		sd->pet.hungry = 0;
		sd->pet.intimate--;
		if(sd->pet.intimate <= 0) {
			sd->pet.intimate = 0;
			if(battle_config.pet_status_support && t > 0) {
				if(sd->bl.prev != NULL)
					pc_calcstatus(sd,0);
				else
					pc_calcstatus(sd,2);
			}
		}
		clif_send_petdata(sd,1,sd->pet.intimate);
	}
	clif_send_petdata(sd,2,sd->pet.hungry);

	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd->petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd->petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd->pet_hungry_timer = add_timer(tick+interval,pet_hungry,sd->bl.id,0);

	return 0;
}

int search_petDB_index(int key,int type)
{
	int i;

	for(i=0;i<MAX_PET_DB;i++) {
		if(pet_db[i].class <= 0)
			continue;
		switch(type) {
			case PET_CLASS:
				if(pet_db[i].class == key)
					return i;
				break;
			case PET_CATCH:
				if(pet_db[i].itemID == key)
					return i;
				break;
			case PET_EGG:
				if(pet_db[i].EggID == key)
					return i;
				break;
			case PET_EQUIP:
				if(pet_db[i].AcceID == key)
					return i;
				break;
			case PET_FOOD:
				if(pet_db[i].FoodID == key)
					return i;
				break;
			default:
				return -1;
		}
	}
	return -1;
}

int pet_hungry_timer_delete(struct map_session_data *sd)
{
	if(sd->pet_hungry_timer != -1) {
		delete_timer(sd->pet_hungry_timer,pet_hungry);
		sd->pet_hungry_timer = -1;
	}

	return 0;
}

int pet_remove_map(struct map_session_data *sd)
{
	if(sd->status.pet_id && sd->pd) {
		pet_changestate(sd->pd,MS_IDLE,0);
		if(sd->pet_hungry_timer != -1)
			pet_hungry_timer_delete(sd);
		clif_clearchar_area(&sd->pd->bl,0);
		map_delblock(&sd->pd->bl);
		map_deliddb(&sd->pd->bl);
		map_freeblock(sd->pd);
	}
	return 0;
}

int pet_performance(struct map_session_data *sd)
{
	pet_stop_walking(sd->pd,2000<<8);
	clif_pet_performance(sd->pd,rand()%pet_performance_val(sd) + 1);

	return 0;
}

int pet_return_egg(struct map_session_data *sd)
{
	struct item tmp_item;
	int flag;

	if(sd->status.pet_id && sd->pd) {
		pet_remove_map(sd);
		sd->status.pet_id = 0;
		sd->pd = NULL;

		if(sd->petDB == NULL)
			return 1;
		sd->pet.incuvate = 1;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid = sd->petDB->EggID;
		tmp_item.identify = 1;
		tmp_item.card[0] = 0xff00;
		*((long *)(&tmp_item.card[1])) = sd->pet.pet_id;
		tmp_item.card[3] = sd->pet.rename_flag;
		if((flag = pc_additem(sd,&tmp_item,1))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
		if(battle_config.pet_status_support && sd->pet.intimate > 0) {
			if(sd->bl.prev != NULL)
				pc_calcstatus(sd,0);
			else
				pc_calcstatus(sd,2);
		}

		intif_save_petdata(sd->status.account_id,&sd->pet);
		pc_makesavestatus(sd);
		chrif_save(sd);
		storage_storage_save(sd);

		sd->petDB = NULL;
	}

	return 0;
}

int pet_data_init(struct map_session_data *sd)
{
	struct pet_data *pd;
	int i,interval;

	if(sd->status.account_id != sd->pet.account_id || sd->status.char_id != sd->pet.char_id ||
		sd->status.pet_id != sd->pet.pet_id) {
		sd->status.pet_id = 0;
		return 1;
	}

	i = search_petDB_index(sd->pet.class,PET_CLASS);
	if(i < 0) {
		sd->status.pet_id = 0;
		return 1;
	}
	sd->petDB = &pet_db[i];
	sd->pd = pd = malloc(sizeof(struct pet_data));
	if(pd==NULL){
		printf("out of memory : pet_data_init\n");
		exit(1);
	}

	pd->n = 0;
	pd->bl.m = sd->bl.m;
	pd->bl.prev = pd->bl.next = NULL;
	pd->bl.x = pd->to_x = sd->bl.x;
	pd->bl.y = pd->to_y = sd->bl.y;
	pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->dir);
	pd->bl.x = pd->to_x;
	pd->bl.y = pd->to_y;
	pd->bl.id = npc_get_new_npc_id();
	memcpy(pd->name,sd->pet.name,24);
	pd->class = sd->pet.class;
	pd->equip = sd->pet.equip;
	pd->dir = sd->dir;
	pd->speed = sd->petDB->speed;
	pd->bl.subtype = MONS;
	pd->bl.type = BL_PET;
	memset(&pd->state,0,sizeof(pd->state));
	pd->state.state = MS_IDLE;
	pd->state.change_walk_target = 0;
	pd->timer = -1;
	pd->target_id = 0;
	pd->move_fail_count = 0;
	pd->next_walktime = pd->attackabletime = pd->last_thinktime = gettick();
	pd->msd = sd;

	map_addiddb(&pd->bl);

	if(sd->pet_hungry_timer != -1)
		pet_hungry_timer_delete(sd);
	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd->petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd->petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd->pet_hungry_timer = add_timer(gettick()+interval,pet_hungry,sd->bl.id,0);

	return 0;
}

int pet_birth_process(struct map_session_data *sd)
{
	if(sd->status.pet_id && sd->pet.incuvate == 1) {
		sd->status.pet_id = 0;
		return 1;
	}

	sd->pet.incuvate = 0;
	sd->pet.account_id = sd->status.account_id;
	sd->pet.char_id = sd->status.char_id;
	sd->status.pet_id = sd->pet.pet_id;
	if(pet_data_init(sd)) {
		sd->status.pet_id = 0;
		sd->pet.incuvate = 1;
		sd->pet.account_id = 0;
		sd->pet.char_id = 0;
		return 1;
	}

	intif_save_petdata(sd->status.account_id,&sd->pet);
	pc_makesavestatus(sd);
	chrif_save(sd);
	storage_storage_save(sd);
	map_addblock(&sd->pd->bl);
	clif_spawnpet(sd->pd);
	clif_send_petdata(sd,0,0);
	clif_send_petdata(sd,5,0x14);
	clif_pet_equip(sd->pd,sd->pet.equip);
	clif_send_petstatus(sd);

	return 0;
}

int pet_recv_petdata(int account_id,struct s_pet *p,int flag)
{
	struct map_session_data *sd;

	sd = map_id2sd(account_id);
	if(sd == NULL)
		return 1;
	if(flag == 1) {
		sd->status.pet_id = 0;
		return 1;
	}
	memcpy(&sd->pet,p,sizeof(struct s_pet));
	if(sd->pet.incuvate == 1)
		pet_birth_process(sd);
	else {
		pet_data_init(sd);
		if(sd->bl.prev != NULL) {
			map_addblock(&sd->pd->bl);
			clif_spawnpet(sd->pd);
			clif_send_petdata(sd,0,0);
			clif_send_petdata(sd,5,0x14);
//			clif_pet_equip(sd->pd,sd->pet.equip);
			clif_send_petstatus(sd);
		}
	}
	if(battle_config.pet_status_support && sd->pet.intimate > 0) {
		if(sd->bl.prev != NULL)
			pc_calcstatus(sd,0);
		else
			pc_calcstatus(sd,2);
	}

	return 0;
}

int pet_select_egg(struct map_session_data *sd,short egg_index)
{
	if(sd->status.inventory[egg_index].card[0] == (short)0xff00)
		intif_request_petdata(sd->status.account_id,sd->status.char_id,*((long *)&sd->status.inventory[egg_index].card[1]));
	else {
		if(battle_config.error_log)
			printf("wrong egg item inventory %d\n",egg_index);
	}
	pc_delitem(sd,egg_index,1,0);

	return 0;
}

int pet_catch_process1(struct map_session_data *sd,int target_class)
{
	sd->catch_target_class = target_class;
	clif_catch_process(sd);

	return 0;
}

int pet_catch_process2(struct map_session_data *sd,int target_id)
{
	struct mob_data *md;
	int i,pet_catch_rate;

	md=(struct mob_data*)map_id2bl(target_id);

	i = search_petDB_index(md->class,PET_CLASS);
	if(md == NULL || md->bl.type != BL_MOB || md->bl.prev == NULL || i < 0 || sd->catch_target_class != md->class) {
		clif_pet_rulet(sd,0);
		return 1;
	}

	//target_id�ɂ��G��������
//	if(battle_config.etc_log)
//		printf("mob_id = %d, mob_class = %d\n",md->bl.id,md->class);
		//�����̏ꍇ
	pet_catch_rate = (pet_db[i].capture + (sd->status.base_level - mob_db[md->class].lv)*30 + sd->paramc[5]*20)*(200 - md->hp*100/mob_db[md->class].max_hp)/100;
	if(pet_catch_rate < 1) pet_catch_rate = 1;
	if(battle_config.pet_catch_rate != 100)
		pet_catch_rate = (pet_catch_rate*battle_config.pet_catch_rate)/100;

	if(rand()%10000 < pet_catch_rate) {
		mob_catch_delete(md);
		clif_pet_rulet(sd,1);
//		if(battle_config.etc_log)
//			printf("rulet success %d\n",target_id);
		intif_create_pet(sd->status.account_id,sd->status.char_id,pet_db[i].class,mob_db[pet_db[i].class].lv,
			pet_db[i].EggID,0,pet_db[i].intimate,100,0,1,pet_db[i].jname);
	}
	else
		clif_pet_rulet(sd,0);

	return 0;
}

int pet_get_egg(int account_id,int pet_id,int flag)
{
	struct map_session_data *sd;
	struct item tmp_item;
	int i,ret;

	if(!flag) {
		sd = map_id2sd(account_id);
		if(sd == NULL)
			return 1;

		i = search_petDB_index(sd->catch_target_class,PET_CLASS);
		if(i >= 0) {
			memset(&tmp_item,0,sizeof(tmp_item));
			tmp_item.nameid = pet_db[i].EggID;
			tmp_item.identify = 1;
			tmp_item.card[0] = 0xff00;
			*((long *)(&tmp_item.card[1])) = pet_id;
			tmp_item.card[3] = sd->pet.rename_flag;
			if((ret = pc_additem(sd,&tmp_item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		else
			intif_delete_petdata(pet_id);
	}

	return 0;
}

int pet_menu(struct map_session_data *sd,int menunum)
{
	switch(menunum) {
		case 0:
			clif_send_petstatus(sd);
			break;
		case 1:
			pet_food(sd);
			break;
		case 2:
			pet_performance(sd);
			break;
		case 3:
			pet_return_egg(sd);
			break;
		case 4:
			pet_unequipitem(sd);
			break;
	}
	return 0;
}

int pet_change_name(struct map_session_data *sd,char *name)
{
	if(sd->pet.rename_flag == 1 && battle_config.pet_rename == 0)
		return 1;

	pet_stop_walking(sd->pd,1);
	memcpy(sd->pet.name,name,24);
	memcpy(sd->pd->name,name,24);
	clif_clearchar_area(&sd->pd->bl,0);
	clif_spawnpet(sd->pd);
	clif_send_petdata(sd,0,0);
	clif_send_petdata(sd,5,0x14);
	sd->pet.rename_flag = 1;
	clif_pet_equip(sd->pd,sd->pet.equip);
	clif_send_petstatus(sd);

	return 0;
}

int pet_equipitem(struct map_session_data *sd,int index)
{
	int nameid;

	nameid = sd->status.inventory[index].nameid;
	if(sd->petDB == NULL)
		return 1;
	if(sd->petDB->AcceID == 0 || nameid != sd->petDB->AcceID || sd->pet.equip != 0) {
		clif_equipitemack(sd,0,0,0);
		return 1;
	}
	else {
		pc_delitem(sd,index,1,0);
		sd->pet.equip = sd->pd->equip = nameid;
		clif_pet_equip(sd->pd,nameid);
	}

	return 0;
}

int pet_unequipitem(struct map_session_data *sd)
{
	struct item tmp_item;
	int nameid,flag;

	if(sd->petDB == NULL)
		return 1;
	if(sd->pet.equip == 0)
		return 1;

	nameid = sd->pet.equip;
	sd->pet.equip = sd->pd->equip = 0;
	clif_pet_equip(sd->pd,0);
	memset(&tmp_item,0,sizeof(tmp_item));
	tmp_item.nameid = nameid;
	tmp_item.identify = 1;
	if((flag = pc_additem(sd,&tmp_item,1))) {
		clif_additem(sd,0,0,flag);
		map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
	}

	return 0;
}

int pet_food(struct map_session_data *sd)
{
	int i,k,t;

	if(sd->petDB == NULL)
		return 1;
	i=pc_search_inventory(sd,sd->petDB->FoodID);
	if(i < 0) {
		clif_pet_food(sd,sd->petDB->FoodID,0);
		return 1;
	}
	pc_delitem(sd,i,1,0);
	t = sd->pet.intimate;
	if(sd->pet.hungry > 90)
		sd->pet.intimate -= sd->petDB->r_full;
	else if(sd->pet.hungry > 75) {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd->petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd->petDB->r_hungry;
		k = k >> 1;
		if(k <= 0)
			k = 1;
		sd->pet.intimate += k;
	}
	else {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd->petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd->petDB->r_hungry;
		sd->pet.intimate += k;
	}
	if(sd->pet.intimate <= 0) {
		sd->pet.intimate = 0;
		if(battle_config.pet_status_support && t > 0) {
			if(sd->bl.prev != NULL)
				pc_calcstatus(sd,0);
			else
				pc_calcstatus(sd,2);
		}
	}
	else if(sd->pet.intimate > 1000)
		sd->pet.intimate = 1000;
	sd->pet.hungry += sd->petDB->fullness;
	if(sd->pet.hungry > 100)
		sd->pet.hungry = 100;

	clif_send_petdata(sd,2,sd->pet.hungry);
	clif_send_petdata(sd,1,sd->pet.intimate);
	clif_pet_food(sd,sd->petDB->FoodID,1);

	return 0;
}

static int pet_randomwalk(struct pet_data *pd,int tick)
{
	const int retrycount=20;
	int speed = battle_get_speed(&pd->bl);
	if(DIFF_TICK(pd->next_walktime,tick) < 0){
		int i,x,y,c,d=12-pd->move_fail_count;
		if(d<5) d=5;
		for(i=0;i<retrycount;i++){
			int r=rand();
			x=pd->bl.x+r%(d*2+1)-d;
			y=pd->bl.y+r/(d*2+1)%(d*2+1)-d;
			if((c=map_getcell(pd->bl.m,x,y))!=1 && c!=5 && pet_walktoxy(pd,x,y)==0){
				pd->move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				pd->move_fail_count++;
				if(pd->move_fail_count>1000){
					if(battle_config.error_log)
						printf("PET cant move. hold position %d, class = %d\n",pd->bl.id,pd->class);
					pd->move_fail_count=0;
					pet_changestate(pd,MS_DELAY,60000);
					return 0;
				}
			}
		}
		for(i=c=0;i<pd->walkpath.path_len;i++){
			if(pd->walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		pd->next_walktime = tick+rand()%3000+3000+c;

		return 1;
	}
	return 0;
}

static int pet_unlocktarget(struct pet_data *pd)
{
	pd->target_id=0;

	return 0;
}

static int pet_ai_sub_hard(struct pet_data *pd,unsigned int tick)
{
	struct map_session_data *sd = pd->msd;
	struct mob_data *md = NULL;
	int dist,i,dx,dy,ret;
	int mode,race;

	if(pd->bl.prev == NULL || sd == NULL || sd->bl.prev == NULL)
		return 0;

	if(DIFF_TICK(tick,pd->last_thinktime) < MIN_PETTHINKTIME)
		return 0;
	pd->last_thinktime=tick;

	if(pd->state.state == MS_DELAY || pd->bl.m != sd->bl.m)
		return 0;

	if(sd->pet.intimate > 0) {
		dist = distance(sd->bl.x,sd->bl.y,pd->bl.x,pd->bl.y);
		if(dist > 12) {
			if(pd->target_id > 0)
				pet_unlocktarget(pd);
			if(pd->timer != -1 && pd->state.state == MS_WALK && distance(pd->to_x,pd->to_y,sd->bl.x,sd->bl.y) < 3)
				return 0;
			pd->speed = (sd->speed>>1);
			if(pd->speed <= 0)
				pd->speed = 1;
			pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->dir);
			if(pet_walktoxy(pd,pd->to_x,pd->to_y))
				pet_randomwalk(pd,tick);
		}
		else if(pd->target_id > 0) {
			mode=mob_db[pd->class].mode;
			race=mob_db[pd->class].race;
			md=(struct mob_data *)map_id2bl(pd->target_id);
			if(md == NULL || md->bl.type != BL_MOB || pd->bl.m != md->bl.m || md->bl.prev == NULL ||
				distance(pd->bl.x,pd->bl.y,md->bl.x,md->bl.y) > 13)
				pet_unlocktarget(pd);
			else if(mob_db[pd->class].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race!=4 && race!=6) )
				pet_unlocktarget(pd);
			else if(!battle_check_range(&pd->bl,&md->bl,mob_db[pd->class].range)){
				if(pd->timer != -1 && pd->state.state == MS_WALK && distance(pd->to_x,pd->to_y,md->bl.x,md->bl.y) < 2)
					return 0;
				if( !pet_can_reach(pd,md->bl.x,md->bl.y))
					pet_unlocktarget(pd);
				else {
					i=0;
					pd->speed = battle_get_speed(&pd->bl);
					do {
						if(i==0) {	// �ŏ���AEGIS�Ɠ������@�Ō���
							dx=md->bl.x - pd->bl.x;
							dy=md->bl.y - pd->bl.y;
							if(dx<0) dx++;
							else if(dx>0) dx--;
							if(dy<0) dy++;
							else if(dy>0) dy--;
						}
						else {	// ���߂Ȃ�Athena��(�����_��)
							dx=md->bl.x - pd->bl.x + rand()%3 - 1;
							dy=md->bl.y - pd->bl.y + rand()%3 - 1;
						}
						ret=pet_walktoxy(pd,pd->bl.x+dx,pd->bl.y+dy);
						i++;
					} while(ret && i<5);

					if(ret) { // �ړ��s�\�ȏ�����̍U���Ȃ�2������
						if(dx<0) dx=2;
						else if(dx>0) dx=-2;
						if(dy<0) dy=2;
						else if(dy>0) dy=-2;
						pet_walktoxy(pd,pd->bl.x+dx,pd->bl.y+dy);
					}
				}
			}
			else {
				if(pd->state.state==MS_WALK)
					pet_stop_walking(pd,1);
				if(pd->state.state==MS_ATTACK)
					return 0;
				pet_changestate(pd,MS_ATTACK,0);
			}
		}
		else {
			if(dist <= 3 || (pd->timer != -1 && pd->state.state == MS_WALK && distance(pd->to_x,pd->to_y,sd->bl.x,sd->bl.y) < 3) )
				return 0;
			pd->speed = battle_get_speed(&pd->bl);
			pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->dir);
			if(pet_walktoxy(pd,pd->to_x,pd->to_y))
				pet_randomwalk(pd,tick);
		}
	}
	else {
		pd->speed = battle_get_speed(&pd->bl);
		if(pd->state.state == MS_ATTACK)
			pet_stopattack(pd);
		pet_randomwalk(pd,tick);
	}

	return 0;
}

static int pet_ai_sub_foreachclient(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;

	tick=va_arg(ap,unsigned int);
	if(sd->status.pet_id && sd->pd && sd->petDB)
		pet_ai_sub_hard(sd->pd,tick);

	return 0;
}

static int pet_ai_hard(int tid,unsigned int tick,int id,int data)
{
	clif_foreachclient(pet_ai_sub_foreachclient,tick);

	return 0;
}

/*==========================================
 *�y�b�g�f�[�^�ǂݍ���
 *------------------------------------------
 */ 
int read_petdb()
{
	FILE *fp;
	char line[1024];
	int j=0;
	
	memset(pet_db,0,sizeof(pet_db));
	fp=fopen("db/pet_db.txt","r");
	if(fp==NULL){
		printf("can't read db/pet_db.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		int nameid,i;
		char *str[32],*p,*np;

		if(line[0] == '/' && line[1] == '/')
			continue;

		for(i=0,p=line;i<20;i++){
			if((np=strchr(p,','))!=NULL){
				str[i]=p;
				*np=0;
				p=np+1;
			} else {
				str[i]=p;
				p+=strlen(p);
			}
		}

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>2000)
			continue;
		
		//MobID,Name,JName,ItemID,EggID,AcceID,FoodID,"Fullness (1��̉a�ł̖����x������%)","HungryDeray (/min)","R_Hungry (�󕠎��a���e���x������%)","R_Full (�ƂĂ��������a���e���x������%)","Intimate (�ߊl���e���x%)","Die (���S���e���x������%)","Capture (�ߊl��%)",(Name)
		pet_db[j].class = nameid;
		memcpy(pet_db[j].name,str[1],24);
		memcpy(pet_db[j].jname,str[2],24);
		pet_db[j].itemID=atoi(str[3]);
		pet_db[j].EggID=atoi(str[4]);
		pet_db[j].AcceID=atoi(str[5]);
		pet_db[j].FoodID=atoi(str[6]);
		pet_db[j].fullness=atoi(str[7]);
		pet_db[j].hungry_delay=atoi(str[8])*1000;
		pet_db[j].r_hungry=atoi(str[9]);
		if(pet_db[j].r_hungry <= 0)
			pet_db[j].r_hungry=1;
		pet_db[j].r_full=atoi(str[10]);
		pet_db[j].intimate=atoi(str[11]);
		pet_db[j].die=atoi(str[12]);
		pet_db[j].capture=atoi(str[13]);
		pet_db[j].speed=atoi(str[14]);
		pet_db[j].s_perfor=(char)atoi(str[15]);
		pet_db[j].talk_convert_class=atoi(str[16]);
		pet_db[j].attack_rate=atoi(str[17]);
		pet_db[j].defence_attack_rate=atoi(str[18]);
		pet_db[j].change_target_rate=atoi(str[19]);
		pet_db[j].script = NULL;
		if((np=strchr(p,'{'))==NULL)
			continue;
		pet_db[j].script = parse_script(np,0);
		j++;
	}
	fclose(fp);
	printf("read db/pet_db.txt done (count=%d)\n",j);
	return 0;
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_pet(void)
{
	read_petdb();

	add_timer_func_list(pet_timer,"pet_timer");
	add_timer_func_list(pet_hungry,"pet_hungry");
	add_timer_func_list(pet_ai_hard,"pet_ai_hard");
	add_timer_interval(gettick()+MIN_PETTHINKTIME,pet_ai_hard,0,0,MIN_PETTHINKTIME);

	return 0;
}
