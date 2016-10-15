#ifndef _GUILD_H_
#define _GUILD_H_


struct map_session_data;
struct mob_data;
struct guild;
struct guild_member;
struct guild_position;

int guild_skill_get_inf(int id);
int guild_skill_get_sp(int id,int lv);
int guild_skill_get_range(int id);
int guild_skill_get_max(int id);

int guild_checkskill(struct guild *g,int id);

void do_init_guild(void);
struct guild *guild_search(int guild_id);
struct guild_castle *guild_castle_search(int gcid);

struct map_session_data *guild_getavailablesd(struct guild *g);
int guild_getindex(struct guild *g,int account_id,int char_id);
int guild_getposition(struct map_session_data *sd,struct guild *g);
int guild_payexp(struct map_session_data *sd,int exp);

int guild_create(struct map_session_data *sd,char *name);
int guild_created(int account_id,int guild_id);
int guild_request_info(int guild_id);
int guild_recv_noinfo(int guild_id);
int guild_recv_info(struct guild *sg);
int guild_invite(struct map_session_data *sd,int account_id);
int guild_reply_invite(struct map_session_data *sd,int guild_id,int flag);
int guild_member_added(int guild_id,int account_id,int char_id,int flag);
int guild_leave(struct map_session_data *sd,int guild_id,
	int account_id,int char_id,const char *mes);
int guild_member_leaved(int guild_id,int account_id,int char_id,int flag,
	const char *name,const char *mes);
int guild_explusion(struct map_session_data *sd,int guild_id,
	int account_id,int char_id,const char *mes);
int guild_skillup(struct map_session_data *sd,int skill_num);
int guild_reqalliance(struct map_session_data *sd,int account_id);
int guild_reply_reqalliance(struct map_session_data *sd,int account_id,int flag);
int guild_alliance(int guild_id1,int guild_id2,int account_id1,int account_id2);
int guild_allianceack(int guild_id1,int guild_id2,int account_id1,int account_id2,
	int flag,const char *name1,const char *name2);
int guild_delalliance(struct map_session_data *sd,int guild_id,int flag);
int guild_opposition(struct map_session_data *sd,int char_id);

int guild_send_memberinfoshort(struct map_session_data *sd,int online);
int guild_recv_memberinfoshort(int guild_id,int account_id,int char_id,int online,int lv,int class);
int guild_change_memberposition(int guild_id,int account_id,int char_id,int idx);
int guild_memberposition_changed(struct guild *g,int idx,int pos);
int guild_change_position(struct map_session_data *sd,int idx,
	int mode,int exp_mode,const char *name);
int guild_position_changed(int guild_id,int idx,struct guild_position *p);
int guild_change_notice(struct map_session_data *sd,int guild_id,const char *mes1,const char *mes2);
int guild_notice_changed(int guild_id,const char *mes1,const char *mes2);
int guild_change_emblem(struct map_session_data *sd,int len,const char *data);
int guild_emblem_changed(int len,int guild_id,int emblem_id,const char *data);
int guild_send_message(struct map_session_data *sd,char *mes,int len);
int guild_recv_message(int guild_id,int account_id,char *mes,int len);
int guild_skillupack(int guild_id,int skill_num,int account_id);
int guild_break(struct map_session_data *sd,char *name);
int guild_broken(int guild_id,int flag);

int guild_agit_start(void);
int guild_agit_end(void);
int guild_agit_break(struct mob_data *md);


#endif
