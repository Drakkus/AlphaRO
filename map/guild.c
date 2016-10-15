#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guild.h"
#include "db.h"
#include "timer.h"
#include "battle.h"
#include "npc.h"
#include "pc.h"
#include "map.h"
#include "mob.h"
#include "intif.h"
#include "clif.h"
#include "socket.h"

static struct dbt *guild_db;
static struct dbt *castle_db;
static struct dbt *guild_expcache_db;

// �M���h��EXP�L���b�V���̃t���b�V���Ɋ֘A����萔
#define GUILD_PAYEXP_INVERVAL 10000	// �Ԋu(�L���b�V���̍ő吶�����ԁA�~���b)
#define GUILD_PAYEXP_LIST 8192	// �L���b�V���̍ő吔

// �M���h��EXP�L���b�V��
struct guild_expcache {
	int guild_id, account_id, char_id, exp;
};

// �M���h�X�L��db�̃A�N�Z�T�i���͒��ł��ő�p�j
int guild_skill_get_inf(int id){ return 0; }
int guild_skill_get_sp(int id,int lv){ return 0; }
int guild_skill_get_range(int id){ return 0; }
int guild_skill_get_max(int id){ return (id==10004)?10:1; }

// �M���h�X�L�������邩�m�F
int guild_checkskill(struct guild *g,int id){ return g->skill[id-10000].lv; }


int guild_payexp_timer(int tid,unsigned int tick,int id,int data);
int guild_gvg_eliminate_timer(int tid,unsigned int tick,int id,int data);


static int guild_read_castledb(void)
{
	FILE *fp;
	char line[1024];
	int j,ln=0;
	char *str[32],*p;
	struct guild_castle *gc;

	if( (fp=fopen("db/castle_db.txt","r"))==NULL){
		printf("can't read db/castle_db.txt\n");
		return -1;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		gc=malloc(sizeof(struct guild_castle));
		if(gc==NULL){
			printf("guild: out of memory!\n");
			exit(0);
		}
		for(j=0,p=line;j<5 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}

		gc->guild_id=0; // <Agit> Clear Data for Initialize
		gc->economy=0; gc->defense=0; gc->triggerE=0; gc->triggerD=0; gc->nextTime=0; gc->payTime=0;
		gc->createTime=0; gc->visibleC=0; gc->visibleG0=0; gc->visibleG1=0; gc->visibleG2=0;
		gc->visibleG3=0; gc->visibleG4=0; gc->visibleG5=0; gc->visibleG6=0; gc->visibleG7=0;

		gc->castle_id=atoi(str[0]);
		memcpy(gc->map_name,str[1],24);
		memcpy(gc->castle_name,str[2],24);

		numdb_insert(castle_db,gc->castle_id,gc);

		//��̃M���h�₢���킹��GVG�J�n���ɁB
//		intif_guild_castle_info(gc->castle_id);

		ln++;
	}
	fclose(fp);
	printf("read db/castle_db.txt done (count=%d)\n",ln);
	return 0;
}

// ������
void do_init_guild(void)
{
	guild_db=numdb_init();
	castle_db=numdb_init();
	guild_expcache_db=numdb_init();

	guild_read_castledb();

	add_timer_func_list(guild_gvg_eliminate_timer,"guild_gvg_eliminate_timer");
	add_timer_func_list(guild_payexp_timer,"guild_payexp_timer");
	add_timer_interval(gettick()+GUILD_PAYEXP_INVERVAL,guild_payexp_timer,0,0,GUILD_PAYEXP_INVERVAL);
}


// ����
struct guild *guild_search(int guild_id)
{
	return numdb_search(guild_db,guild_id);
}

struct guild_castle *guild_castle_search(int gcid)
{
	return numdb_search(castle_db,gcid);
}

// ���O�C�����̃M���h�����o�[�̂P�l��sd��Ԃ�
struct map_session_data *guild_getavailablesd(struct guild *g)
{
	int i;
	for(i=0;i<g->max_member;i++)
		if(g->member[i].sd!=NULL)
			return g->member[i].sd;
	return NULL;
}

// �M���h�����o�[�̃C���f�b�N�X��Ԃ�
int guild_getindex(struct guild *g,int account_id,int char_id)
{
	int i;
	if(g==NULL)
		return -1;
	for(i=0;i<g->max_member;i++)
		if( g->member[i].account_id==account_id &&
			g->member[i].char_id==char_id )
			return i;
	return -1;
}
// �M���h�����o�[�̖�E��Ԃ�
int guild_getposition(struct map_session_data *sd,struct guild *g)
{
	int i;
	if(g==NULL && (g=guild_search(sd->status.guild_id))==NULL)
		return -1;
	for(i=0;i<g->max_member;i++)
		if( g->member[i].account_id==sd->status.account_id &&
			g->member[i].char_id==sd->status.char_id )
			return g->member[i].position;
	return -1;
}

// �����o�[���̍쐬
void guild_makemember(struct guild_member *m,struct map_session_data *sd)
{
	memset(m,0,sizeof(struct guild_member));
	m->account_id	=sd->status.account_id;
	m->char_id		=sd->status.char_id;
	m->hair			=sd->status.hair;
	m->hair_color	=sd->status.hair_color;
	m->gender		=sd->sex;
	m->class		=sd->status.class;
	m->lv			=sd->status.base_level;
	m->exp			=0;
	m->exp_payper	=0;
	m->online		=1;
	m->position		=MAX_GUILDPOSITION-1;
	memcpy(m->name,sd->status.name,24);
	return;
}
// �M���h�����m�F
int guild_check_conflict(struct map_session_data *sd)
{
	intif_guild_checkconflict(sd->status.guild_id,
		sd->status.account_id,sd->status.char_id);
	return 0;
}

// �M���h��EXP�L���b�V����inter�I�Ƀt���b�V������
int guild_payexp_timer_sub(void *key,void *data,va_list ap)
{
	int i, *dellist,*delp, dataid=(int)key;
	struct guild_expcache *c=(struct guild_expcache *)data;
	struct guild *g;
	dellist=va_arg(ap,int *);
	delp=va_arg(ap,int *);
	
	if( *delp>=GUILD_PAYEXP_LIST || (g=guild_search(c->guild_id))==NULL )
		return 0;
	if( ( i=guild_getindex(g,c->account_id,c->char_id) )<0 )
		return 0;
	
	g->member[i].exp+=c->exp;
	intif_guild_change_memberinfo(g->guild_id,c->account_id,c->char_id,
		GMI_EXP,&g->member[i].exp,sizeof(g->member[i].exp));
	c->exp=0;

	dellist[(*delp)++]=dataid;
	free(c);
	return 0;
}
int guild_payexp_timer(int tid,unsigned int tick,int id,int data)
{
	int dellist[GUILD_PAYEXP_LIST],delp=0,i;
	numdb_foreach(guild_expcache_db,guild_payexp_timer_sub,
		dellist,&delp);
	for(i=0;i<delp;i++)
		numdb_erase(guild_expcache_db,dellist[i]);
//	if(battle_config.etc_log)
//		printf("guild exp %d charactor's exp flushed !\n",delp);
	return 0;
}

//------------------------------------------------------------------------

// �쐬�v��
int guild_create(struct map_session_data *sd,char *name)
{
	if(sd->status.guild_id==0){
		if(!battle_config.guild_emperium_check || pc_search_inventory(sd,714) >= 0) {
			struct guild_member m;
			guild_makemember(&m,sd);
			m.position=0;
			intif_guild_create(name,&m);
		} else
			clif_guild_created(sd,3);	// �G���y���E�������Ȃ�
	}else
		clif_guild_created(sd,1);	// ���łɏ������Ă���

	return 0;
}

// �쐬��
int guild_created(int account_id,int guild_id)
{
	struct map_session_data *sd;
	sd=map_id2sd(account_id);
	if(sd==NULL)
		return 0;
	if(guild_id>0) {
			struct guild *g;
			sd->status.guild_id=guild_id;
			sd->guild_sended=0;
			if((g=numdb_search(guild_db,guild_id))!=NULL){
				printf("guild: id already exists!\n");
				exit(1);
			}
			clif_guild_created(sd,0);
			if(battle_config.guild_emperium_check)
				pc_delitem(sd,pc_search_inventory(sd,714),1,0);	// �G���y���E������
	} else {
		clif_guild_created(sd,2);	// �쐬���s�i�����M���h���݁j
	}
	return 0;
}

// ���v��
int guild_request_info(int guild_id)
{
//	if(battle_config.etc_log)
//		printf("guild_request_info\n");
	return intif_guild_request_info(guild_id);
}

// �����L�����̊m�F
int guild_check_member(const struct guild *g)
{
	int i;
	struct map_session_data *sd;
	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
			if(sd->status.guild_id==g->guild_id){
				int j,f=1;
				for(j=0;j<MAX_GUILD;j++){	// �f�[�^�����邩
					if(	g->member[j].account_id==sd->status.account_id &&
						g->member[j].char_id==sd->status.char_id)
						f=0;
				}
				if(f){
					sd->status.guild_id=0;
					sd->guild_sended=0;
					sd->guild_emblem_id=0;
					if(battle_config.error_log)
						printf("guild: check_member %d[%s] is not member\n",sd->status.account_id,sd->status.name);
				}
			}
		}
	}
	return 0;
}
// ��񏊓����s�i����ID�̃L������S���������ɂ���j
int guild_recv_noinfo(int guild_id)
{
	int i;
	struct map_session_data *sd;
	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
			if(sd->status.guild_id==guild_id)
				sd->status.guild_id=0;
		}
	}
	return 0;
}
// ��񏊓�
int guild_recv_info(struct guild *sg)
{
	struct guild *g,before;
	int i,bm,m;
	
	if((g=numdb_search(guild_db,sg->guild_id))==NULL){
		g=malloc(sizeof(struct guild));
		if(g==NULL){
			printf("guild_recv_info: out of memory!\n");
			exit(1);
		}
		numdb_insert(guild_db,sg->guild_id,g);
		before=*sg;

		// �ŏ��̃��[�h�Ȃ̂Ń��[�U�[�̃`�F�b�N���s��
		guild_check_member(sg);
	}else
		before=*g;
	memcpy(g,sg,sizeof(struct guild));

	for(i=bm=m=0;i<g->max_member;i++){	// sd�̐ݒ�Ɛl���̊m�F
		if(g->member[i].account_id>0){
			struct map_session_data *sd = map_id2sd(g->member[i].account_id);
			g->member[i].sd=(sd!=NULL &&
				sd->status.char_id==g->member[i].char_id &&
				sd->status.guild_id==g->guild_id)?	sd:NULL;
			m++;
		}else
			g->member[i].sd=NULL;
		if(before.member[i].account_id>0)
			bm++;
	}

	for(i=0;i<g->max_member;i++){	// ���̑��M
		struct map_session_data *sd = g->member[i].sd;
		if( sd==NULL )
			continue;

		if(	before.guild_lv!=g->guild_lv || bm!=m ||
			before.max_member!=g->max_member ){
			clif_guild_basicinfo(sd);	// ��{��񑗐M
			clif_guild_emblem(sd,g);	// �G���u�������M
		}

		if(bm!=m){		// �����o�[��񑗐M
			clif_guild_memberlist(g->member[i].sd);
		}

		if( before.skill_point!=g->skill_point)
			clif_guild_skillinfo(sd);	// �X�L����񑗐M

		if( sd->guild_sended==0){	// �����M�Ȃ珊����������
			clif_guild_belonginfo(sd,g);
			clif_guild_notice(sd,g);
			sd->guild_emblem_id=g->emblem_id;
			sd->guild_sended=1;
		}
	}

	return 0;
}


// �M���h�ւ̊��U
int guild_invite(struct map_session_data *sd,int account_id)
{
	struct map_session_data *tsd= map_id2sd(account_id);
	struct guild *g=guild_search(sd->status.guild_id);
	int i;
	
	if(tsd==NULL || g==NULL)
		return 0;
	if( tsd->status.guild_id>0 || tsd->guild_invite>0 ){	// ����̏����m�F
		clif_guild_inviteack(sd,0);
		return 0;
	}
	
	// ����m�F
	for(i=0;i<g->max_member;i++)
		if(g->member[i].account_id==0)
			break;
	if(i==g->max_member){
		clif_guild_inviteack(sd,3);
		return 0;
	}

	tsd->guild_invite=sd->status.guild_id;
	tsd->guild_invite_account=sd->status.account_id;

	clif_guild_invite(tsd,g);
	return 0;
}
// �M���h���U�ւ̕ԓ�
int guild_reply_invite(struct map_session_data *sd,int guild_id,int flag)
{
	struct map_session_data *tsd= map_id2sd( sd->guild_invite_account );

	if(sd->guild_invite!=guild_id)	// ���U�ƃM���hID���Ⴄ
		return 0;

	if(flag==1){	// ����
		struct guild_member m;
		struct guild *g;
		int i;

		// ����m�F
		if( (g=guild_search(tsd->status.guild_id))==NULL ){
			sd->guild_invite=0;
			sd->guild_invite_account=0;
			return 0;
		}
		for(i=0;i<g->max_member;i++)
			if(g->member[i].account_id==0)
				break;
		if(i==g->max_member){
			sd->guild_invite=0;
			sd->guild_invite_account=0;
			clif_guild_inviteack(tsd,3);
			return 0;
		}


		//inter�I�֒ǉ��v��
		guild_makemember(&m,sd);
		intif_guild_addmember( sd->guild_invite, &m );
		return 0;
	}else{		// ����
		sd->guild_invite=0;
		sd->guild_invite_account=0;
		if(tsd==NULL)
			return 0;
		clif_guild_inviteack(tsd,1);
	}
	return 0;
}
// �M���h�����o���ǉ����ꂽ
int guild_member_added(int guild_id,int account_id,int char_id,int flag)
{
	struct map_session_data *sd= map_id2sd(account_id),*sd2;
	struct guild *g;

	if( (g=guild_search(guild_id))==NULL )
		return 0;

	if((sd==NULL || sd->guild_invite==0) && flag==0){
		// �L�������ɓo�^�ł��Ȃ��������ߒE�ޗv�����o��
		if(battle_config.error_log)
			printf("guild: member added error %d is not online\n",account_id);
 		intif_guild_leave(guild_id,account_id,char_id,0,"**�o�^���s**");
		return 0;
	}
	sd->guild_invite=0;
	sd->guild_invite_account=0;

	sd2=map_id2sd(sd->guild_invite_account);
	
	if(flag==1){	// ���s
		if( sd2!=NULL )
			clif_guild_inviteack(sd2,3);
		return 0;
	}
	
		// ����
	sd->guild_sended=0;
	sd->status.guild_id=guild_id;

	if( sd2!=NULL )
		clif_guild_inviteack(sd2,2);
	
	// �������������m�F
	guild_check_conflict(sd);

	return 0;
}

// �M���h�E�ޗv��
int guild_leave(struct map_session_data *sd,int guild_id,
	int account_id,int char_id,const char *mes)
{
	struct guild *g = guild_search(sd->status.guild_id);
	int i;
	if(g==NULL)
		return 0;
	
	if(	sd->status.account_id!=account_id ||
		sd->status.char_id!=char_id || sd->status.guild_id!=guild_id)
		return 0;
	
	for(i=0;i<g->max_member;i++){	// �������Ă��邩
		if(	g->member[i].account_id==sd->status.account_id &&
			g->member[i].char_id==sd->status.char_id ){
			intif_guild_leave(g->guild_id,sd->status.account_id,sd->status.char_id,0,mes);
			return 0;
		}
	}
	return 0;
}
// �M���h�Ǖ��v��
int guild_explusion(struct map_session_data *sd,int guild_id,
	int account_id,int char_id,const char *mes)
{
	struct guild *g = guild_search(sd->status.guild_id);
	int i,ps;
	if(g==NULL)
		return 0;

	if(	sd->status.guild_id!=guild_id)
		return 0;

	if( (ps=guild_getposition(sd,g))<0 || !(g->position[ps].mode&0x0010) )
		return 0;	// ������������

	for(i=0;i<g->max_member;i++){	// �������Ă��邩
		if(	g->member[i].account_id==account_id &&
			g->member[i].char_id==char_id ){
			intif_guild_leave(g->guild_id,account_id,char_id,1,mes);
			return 0;
		}
	}
	return 0;
}
// �M���h�����o���E�ނ���
int guild_member_leaved(int guild_id,int account_id,int char_id,int flag,
	const char *name,const char *mes)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);
	int i;
	
	if(g!=NULL){
		int i;
		for(i=0;i<g->max_member;i++)
			if(	g->member[i].account_id==account_id &&
				g->member[i].char_id==char_id ){
				struct map_session_data *sd2=sd;
				if(sd2==NULL)
					sd2=guild_getavailablesd(g);
				if(sd2!=NULL){
					if(flag==0)
						clif_guild_leave(sd2,name,mes);
					else
						clif_guild_explusion(sd2,name,mes,account_id);
				}
				g->member[i].account_id=0;
				g->member[i].sd=NULL;
			}
	}
	if(sd!=NULL && sd->status.guild_id==guild_id){
		sd->status.guild_id=0;
		sd->guild_emblem_id=0;
		sd->guild_sended=0;
	}

	// �����o�[���X�g��S���ɍĒʒm
	for(i=0;i<g->max_member;i++){
		if( g->member[i].sd!=NULL )
			clif_guild_memberlist(g->member[i].sd);
	}

	return 0;
}
// �M���h�����o�̃I�����C�����/Lv�X�V���M
int guild_send_memberinfoshort(struct map_session_data *sd,int online)
{
	struct guild *g;
	if(sd->status.guild_id<=0)
		return 0;
	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	
	intif_guild_memberinfoshort(g->guild_id,
		sd->status.account_id,sd->status.char_id,online,sd->status.base_level,sd->status.class);

	if( !online ){	// ���O�A�E�g����Ȃ�sd���N���A���ďI��
		int i=guild_getindex(g,sd->status.account_id,sd->status.char_id);
		if(i>=0)
			g->member[i].sd=NULL;
		return 0;
	}

	if( sd->guild_sended!=0 )	// �M���h�������M�f�[�^�͑��M�ς�
		return 0;

	// �����m�F	
	guild_check_conflict(sd);
	
	// ����Ȃ�M���h�������M�f�[�^���M
	if( (g=guild_search(sd->status.guild_id))!=NULL ){
		guild_check_member(g);	// �������m�F����
		if(sd->status.guild_id==g->guild_id){
			clif_guild_belonginfo(sd,g);
			clif_guild_notice(sd,g);
			sd->guild_sended=1;
			sd->guild_emblem_id=g->emblem_id;
		}
	}
	return 0;
}
// �M���h�����o�̃I�����C�����/Lv�X�V�ʒm
int guild_recv_memberinfoshort(int guild_id,int account_id,int char_id,int online,int lv,int class)
{
	int i,alv,c,idx=0,om=0,oldonline=-1;
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;
	for(i=0,alv=0,c=0,om=0;i<g->max_member;i++){
		struct guild_member *m=&g->member[i];
		if(m->account_id==account_id && m->char_id==char_id ){
			oldonline=m->online;
			m->online=online;
			m->lv=lv;
			m->class=class;
			idx=i;
		}
		if(m->account_id>0){
			alv+=m->lv;
			c++;
		}
		if(m->online)
			om++;
	}
	if(idx==g->max_member){
		if(battle_config.error_log)
			printf("guild: not found member %d,%d on %d[%s]\n",	account_id,char_id,guild_id,g->name);
		return 0;
	}
	g->average_lv=alv/c;
	g->connect_member=om;

	if(oldonline!=online)	// �I�����C����Ԃ��ς�����̂Œʒm
		clif_guild_memberlogin_notice(g,idx,online);

	for(i=0;i<g->max_member;i++){	// sd�Đݒ�
		struct map_session_data *sd= map_id2sd(g->member[i].account_id);
		g->member[i].sd=(sd!=NULL &&
			sd->status.char_id==g->member[i].char_id &&
			sd->status.guild_id==guild_id)?sd:NULL;
	}
	
	// �����ɃN���C�A���g�ɑ��M�������K�v
	
	return 0;
}
// �M���h��b���M
int guild_send_message(struct map_session_data *sd,char *mes,int len)
{
	if(sd->status.guild_id==0)
		return 0;
	intif_guild_message(sd->status.guild_id,sd->status.account_id,mes,len);
	return 0;
}
// �M���h��b��M
int guild_recv_message(int guild_id,int account_id,char *mes,int len)
{
	struct guild *g;
	if( (g=guild_search(guild_id))==NULL)
		return 0;
	clif_guild_message(g,account_id,mes,len);
	return 0;
}
// �M���h�����o�̖�E�ύX
int guild_change_memberposition(int guild_id,int account_id,int char_id,int idx)
{
	return intif_guild_change_memberinfo(
		guild_id,account_id,char_id,GMI_POSITION,&idx,sizeof(idx));
}
// �M���h�����o�̖�E�ύX�ʒm
int guild_memberposition_changed(struct guild *g,int idx,int pos)
{
	g->member[idx].position=pos;
	clif_guild_memberpositionchanged(g,idx);
	return 0;
}
// �M���h��E�ύX
int guild_change_position(struct map_session_data *sd,int idx,
	int mode,int exp_mode,const char *name)
{
	struct guild_position p;
	if(exp_mode>100)exp_mode=100;
	if(exp_mode<0)exp_mode=0;
	p.mode=mode;
	p.exp_mode=exp_mode;
	memcpy(p.name,name,24);
	return intif_guild_position(sd->status.guild_id,idx,&p);
}
// �M���h��E�ύX�ʒm
int guild_position_changed(int guild_id,int idx,struct guild_position *p)
{
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;
	memcpy(&g->position[idx],p,sizeof(struct guild_position));
	clif_guild_positionchanged(g,idx);
	return 0;
}
// �M���h���m�ύX
int guild_change_notice(struct map_session_data *sd,int guild_id,const char *mes1,const char *mes2)
{
	if(guild_id!=sd->status.guild_id)
		return 0;
	return intif_guild_notice(guild_id,mes1,mes2);
}
// �M���h���m�ύX�ʒm
int guild_notice_changed(int guild_id,const char *mes1,const char *mes2)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;

	memcpy(g->mes1,mes1,60);
	memcpy(g->mes2,mes2,120);

	for(i=0;i<g->max_member;i++){
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_notice(sd,g);
	}
	return 0;
}
// �M���h�G���u�����ύX
int guild_change_emblem(struct map_session_data *sd,int len,const char *data)
{
	return intif_guild_emblem(sd->status.guild_id,len,data);
}
// �M���h�G���u�����ύX�ʒm
int guild_emblem_changed(int len,int guild_id,int emblem_id,const char *data)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;
	
	memcpy(g->emblem_data,data,len);
	g->emblem_len=len;
	g->emblem_id=emblem_id;
	
	for(i=0;i<g->max_member;i++){
		if((sd=g->member[i].sd)!=NULL){
			sd->guild_emblem_id=emblem_id;
			clif_guild_belonginfo(sd,g);
			clif_guild_emblem(sd,g);
		}
	}
	return 0;
}

// �M���h��EXP��[
int guild_payexp(struct map_session_data *sd,int exp)
{
	struct guild *g;
	struct guild_expcache *c;
	int per,exp2;
	if(sd->status.guild_id==0 || (g=guild_search(sd->status.guild_id))==NULL )
		return 0;
	if( (per=g->position[guild_getposition(sd,g)].exp_mode)<=0 )
		return 0;
	if( per>100 )per=100;

	if( (exp2=exp*per/100)<=0 )
		return 0;
	
	if( (c=numdb_search(guild_expcache_db,sd->status.char_id))==NULL ){
		c=malloc(sizeof(struct guild_expcache));
		if(c==NULL){
			printf("guild_payexp: out of memory !\n");
			exit(1);
		}
		c->guild_id=sd->status.guild_id;
		c->account_id=sd->status.account_id;
		c->char_id=sd->status.char_id;
		c->exp=exp2;
		numdb_insert(guild_expcache_db,c->char_id,c);
	}else{
		c->exp+=exp2;
	}
	return exp2;
}

// �X�L���|�C���g����U��
int guild_skillup(struct map_session_data *sd,int skill_num)
{
	struct guild *g;
	int idx;
	if(sd->status.guild_id==0 || (g=guild_search(sd->status.guild_id))==NULL)
		return 0;
	if(strcmp(sd->status.name,g->master))
		return 0;
	
	if( g->skill_point>0 &&
		g->skill[(idx=skill_num-10000)].id!=0 &&
		g->skill[idx].lv < guild_skill_get_max(skill_num) ){
		intif_guild_skillup(g->guild_id,skill_num,sd->status.account_id);
	}
	return 0;
}
// �X�L���|�C���g����U��ʒm
int guild_skillupack(int guild_id,int skill_num,int account_id)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);
	int i;
	if(g==NULL)
		return 0;
	if(sd!=NULL)
		clif_guild_skillup(sd,skill_num,g->skill[skill_num-10000].lv);
	// �S���ɒʒm
	for(i=0;i<g->max_member;i++)
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_skillinfo(sd);
	return 0;
}

// �M���h����������
int guild_get_alliance_count(struct guild *g,int flag)
{
	int i,c;
	for(i=c=0;i<MAX_GUILDALLIANCE;i++){
		if(	g->alliance[i].guild_id>0 &&
			g->alliance[i].opposition==flag )
			c++;
	}
	return c;
}
// �M���h�����v��
int guild_reqalliance(struct map_session_data *sd,int account_id)
{
	struct map_session_data *tsd= map_id2sd(account_id);
	struct guild *g[2];
	int i;
	
	if(tsd==NULL || tsd->status.guild_id<=0)
		return 0;
	
	g[0]=guild_search(sd->status.guild_id);
	g[1]=guild_search(tsd->status.guild_id);
	
	if(g[0]==NULL || g[1]==NULL)
		return 0;
	
	if( guild_get_alliance_count(g[0],0)>3 )	// �������m�F
		clif_guild_allianceack(sd,4);
	if( guild_get_alliance_count(g[1],0)>3 )
		clif_guild_allianceack(sd,3);
	
	if( tsd->guild_alliance>0 ){	// ���肪�����v����Ԃ��ǂ����m�F
		clif_guild_allianceack(sd,1);
		return 0;
	}

	for(i=0;i<MAX_GUILDALLIANCE;i++){	// ���łɓ�����Ԃ��m�F
		if(	g[0]->alliance[i].guild_id==tsd->status.guild_id &&
			g[0]->alliance[i].opposition==0){
			clif_guild_allianceack(sd,0);
			return 0;
		}
	}
	
	tsd->guild_alliance=sd->status.guild_id;
	tsd->guild_alliance_account=sd->status.account_id;

	clif_guild_reqalliance(tsd,sd->status.account_id,g[0]->name);
	return 0;
}
// �M���h���U�ւ̕ԓ�
int guild_reply_reqalliance(struct map_session_data *sd,int account_id,int flag)
{
	struct map_session_data *tsd= map_id2sd( account_id );

	if(sd->guild_alliance!=tsd->status.guild_id)	// ���U�ƃM���hID���Ⴄ
		return 0;

	if(flag==1){	// ����
		int i;
		
		struct guild *g;	// �������Ċm�F
		if( (g=guild_search(sd->status.guild_id))==NULL ||
			guild_get_alliance_count(g,0)>3 ){
			clif_guild_allianceack(sd,4);
			clif_guild_allianceack(tsd,3);
			return 0;
		}
		if( (g=guild_search(tsd->status.guild_id))==NULL ||
			guild_get_alliance_count(g,0)>3 ){
			clif_guild_allianceack(sd,3);
			clif_guild_allianceack(tsd,4);
			return 0;
		}
		
		// �G�Ί֌W�Ȃ�G�΂��~�߂�
		g=guild_search(sd->status.guild_id);
		for(i=0;i<MAX_GUILDALLIANCE;i++){
			if(	g->alliance[i].guild_id==tsd->status.guild_id &&
				g->alliance[i].opposition==1)
				intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
					sd->status.account_id,tsd->status.account_id,9 );
		}
		g=guild_search(tsd->status.guild_id);
		for(i=0;i<MAX_GUILDALLIANCE;i++){
			if(	g->alliance[i].guild_id==sd->status.guild_id &&
				g->alliance[i].opposition==1)
				intif_guild_alliance( tsd->status.guild_id,sd->status.guild_id,
					tsd->status.account_id,sd->status.account_id,9 );
		}

		// inter�I�֓����v��
		intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
			sd->status.account_id,tsd->status.account_id,0 );
		return 0;
	}else{		// ����
		sd->guild_alliance=0;
		sd->guild_alliance_account=0;
		if(tsd!=NULL)
			clif_guild_allianceack(tsd,3);
	}
	return 0;
}
// �M���h�֌W����
int guild_delalliance(struct map_session_data *sd,int guild_id,int flag)
{
	intif_guild_alliance( sd->status.guild_id,guild_id,
		sd->status.account_id,0,flag|8 );
	return 0;
}
// �M���h�G��
int guild_opposition(struct map_session_data *sd,int char_id)
{
	struct map_session_data *tsd=map_id2sd(char_id);
	struct guild *g=guild_search(sd->status.guild_id);
	int i;
	
	if(g==NULL || tsd==NULL)
		return 0;	
	
	if( guild_get_alliance_count(g,1)>3 )	// �G�ΐ��m�F
		clif_guild_oppositionack(sd,1);
	
	for(i=0;i<MAX_GUILDALLIANCE;i++){	// ���łɊ֌W�������Ă��邩�m�F
		if(g->alliance[i].guild_id==tsd->status.guild_id){
			if(g->alliance[i].opposition==1){	// ���łɓG��
				clif_guild_oppositionack(sd,2);
				return 0;
			}else	// �����j��
				intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
					sd->status.account_id,tsd->status.account_id,8 );
		}
	}

	// inter�I�ɓG�Ηv��
	intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
			sd->status.account_id,tsd->status.account_id,1 );
	return 0;
}
// �M���h����/�G�Βʒm
int guild_allianceack(int guild_id1,int guild_id2,int account_id1,int account_id2,
	int flag,const char *name1,const char *name2)
{
	struct guild *g[2];
	int guild_id[2]={guild_id1,guild_id2};
	const char *guild_name[2]={name1,name2};
	struct map_session_data *sd[2]={map_id2sd(account_id1),map_id2sd(account_id2)};
	int j,i;
	
	g[0]=guild_search(guild_id1);
	g[1]=guild_search(guild_id2);
	
	if(sd[0]!=NULL && (flag&0x0f)==0){
		sd[0]->guild_alliance=0;
		sd[0]->guild_alliance_account=0;
	}
	
	if(flag&0x70){	// ���s
		for(i=0;i<2-(flag&1);i++)
			if( sd[i]!=NULL )
				clif_guild_allianceack(sd[i],((flag>>4)==i+1)?3:4);
		return 0;
	}
//	if(battle_config.etc_log)
//		printf("guild alliance_ack %d %d %d %d %d %s %s\n",guild_id1,guild_id2,account_id1,account_id2,flag,name1,name2);

	if(!(flag&0x08)){	// �֌W�ǉ�
		for(i=0;i<2-(flag&1);i++)
			if(g[i]!=NULL)
				for(j=0;j<MAX_GUILDALLIANCE;j++)
					if(g[i]->alliance[j].guild_id==0){
						g[i]->alliance[j].guild_id=guild_id[1-i];
						memcpy(g[i]->alliance[j].name,guild_name[1-i],24);
						g[i]->alliance[j].opposition=flag&1;
						break;
					}
	}else{				// �֌W����
		for(i=0;i<2-(flag&1);i++){
			if(g[i]!=NULL)
				for(j=0;j<MAX_GUILDALLIANCE;j++)
					if(	g[i]->alliance[j].guild_id==guild_id[1-i] &&
						g[i]->alliance[j].opposition==(flag&1)){
						g[i]->alliance[j].guild_id=0;
						break;
					}
			if( sd[i]!=NULL )	// �����ʒm
				clif_guild_delalliance(sd[i],guild_id[1-i],(flag&1));
		}
	}
	
	if((flag&0x0f)==0){			// �����ʒm
		if( sd[1]!=NULL )
			clif_guild_allianceack(sd[1],2);
	}else if((flag&0x0f)==1){	// �G�Βʒm
		if( sd[0]!=NULL )
			clif_guild_oppositionack(sd[0],0);
	}


	for(i=0;i<2-(flag&1);i++){	// ����/�G�΃��X�g�̍đ��M
		struct map_session_data *sd;
		if(g[i]!=NULL)
			for(j=0;j<g[i]->max_member;j++)
				if((sd=g[i]->member[j].sd)!=NULL)
					clif_guild_allianceinfo(sd);
	}
	return 0;
}
// �M���h���U�ʒm�p
int guild_broken_sub(void *key,void *data,va_list ap)
{
	struct guild *g=(struct guild *)data;
	int guild_id=va_arg(ap,int);
	int i,j;
	struct map_session_data *sd=NULL;
	
	for(i=0;i<MAX_GUILDALLIANCE;i++){	// �֌W��j��
		if(g->alliance[i].guild_id==guild_id){
			for(j=0;j<g->max_member;j++)
				if( (sd=g->member[j].sd)!=NULL )
					clif_guild_delalliance(sd,guild_id,g->alliance[i].opposition);
			g->alliance[i].guild_id=0;
		}
	}
	return 0;
}
// �M���h���U�ʒm
int guild_broken(int guild_id,int flag)
{
	struct guild *g=guild_search(guild_id);
	struct map_session_data *sd;
	int i;
	if(flag!=0 || g==NULL)
		return 0;
	
	for(i=0;i<g->max_member;i++){	// �M���h���U��ʒm
		if((sd=g->member[i].sd)!=NULL){
			sd->status.guild_id=0;
			sd->guild_sended=0;
			clif_guild_broken(g->member[i].sd,0);
		}
	}
	
	numdb_foreach(guild_db,guild_broken_sub,guild_id);
	numdb_erase(guild_db,guild_id);
	free(g);
	return 0;
}

// �M���h���U
int guild_break(struct map_session_data *sd,char *name)
{
	struct guild *g;
	int i;
	if( (g=guild_search(sd->status.guild_id))==NULL )
		return 0;
	if(strcmp(g->name,name)!=0)
		return 0;
	if(strcmp(sd->status.name,g->master)!=0)
		return 0;
	for(i=0;i<g->max_member;i++){
		if(	g->member[i].account_id>0 && (
			g->member[i].account_id!=sd->status.account_id ||
			g->member[i].char_id!=sd->status.char_id ))
			break;
	}
	if(i<g->max_member){
		clif_guild_broken(sd,2);
		return 0;
	}
	
	intif_guild_break(g->guild_id);
	return 0;
}

//-----------------------------
// <Agit>
//-----------------------------

int guild_agit_start(void)
{	// Run All NPC_Event[OnAgitStart]
	int c = npc_event_doall("OnAgitStart");
	printf("NPC_Event:[OnAgitStart] Run (%d) Events by @AgitStart.\n",c);
	return 0;
}

int guild_agit_end(void)
{	// Run All NPC_Event[OnAgitEnd]
	int c = npc_event_doall("OnAgitEnd");
	printf("NPC_Event:[OnAgitEnd] Run (%d) Events by @AgitEnd.\n",c);
	return 0;
}

int guild_gvg_eliminate_timer(int tid,unsigned int tick,int id,int data)
{	// Run One NPC_Event[OnAgitEliminate]
	char *evname=malloc(strlen((const char *)data)+4);
	int c=0;
	if(!agit_flag) return 0;	// Agit already End
	memcpy(evname,(const char *)data,strlen((const char *)data)-5);
	strcpy(evname+strlen((const char *)data)-5,"Eliminate");
	c = npc_event_do(evname);
	printf("NPC_Event:[%s] Run (%d) Events.\n",evname,c);
	return 0;
}

int guild_agit_break(struct mob_data *md)
{	// Run One NPC_Event[OnAgitBreak]
	char *evname=malloc(strlen(md->npc_event)+1);
	strcpy(evname,md->npc_event);
// Now By User to Run [OnAgitBreak] NPC Event...
// It's a little impossible to null point with player disconnect in this!
// But Script will be stop, so nothing...
// Maybe will be changed in the futher..
//      int c = npc_event_do(evname);
	if(!agit_flag) return 0;	// Agit already End
	add_timer(gettick()+battle_config.gvg_eliminate_time,guild_gvg_eliminate_timer,md->bl.m,(int)evname);
	return 0;
}

