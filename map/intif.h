#ifndef _INTIF_H_
#define _INFIF_H_

int intif_parse(int fd);

int intif_GMmessage(char* mes,int len,int flag);

int intif_wis_message(struct map_session_data *sd,char *nick,char *mes,int mes_len);

int intif_request_storage(int account_id);
int intif_send_storage(int account_id);


int intif_create_party(struct map_session_data *sd,char *name);
int intif_request_partyinfo(int party_id);
int intif_party_addmember(int party_id,int account_id);
int intif_party_changeoption(int party_id,int account_id,int exp,int item);
int intif_party_leave(int party_id,int accound_id);
int intif_party_changemap(struct map_session_data *sd,int online);
int intif_break_party(int party_id);
int intif_party_message(int party_id,int account_id,char *mes,int len);
int intif_party_checkconflict(int party_id,int account_id,char *nick);


int intif_guild_create(const char *name,const struct guild_member *master);
int intif_guild_request_info(int guild_id);
int intif_guild_addmember(int guild_id,struct guild_member *m);
int intif_guild_leave(int guild_id,int account_id,int char_id,int flag,const char *mes);
int intif_guild_memberinfoshort(int guild_id,
	int account_id,int char_id,int online,int lv,int class);
int intif_guild_break(int guild_id);
int intif_guild_message(int guild_id,int account_id,char *mes,int len);
int intif_guild_checkconflict(int guild_id,int account_id,int char_id);
int intif_guild_change_basicinfo(int guild_id,int type,const void *data,int len);
int intif_guild_change_memberinfo(int guild_id,int account_id,int char_id,
	int type,const void *data,int len);
int intif_guild_position(int guild_id,int idx,struct guild_position *p);
int intif_guild_skillup(int guild_id,int skill_num,int account_id);
int intif_guild_alliance(int guild_id1,int guild_id2,int account_id1,int account_id2,int flag);
int intif_guild_notice(int guild_id,const char *mes1,const char *mes2);
int intif_guild_emblem(int guild_id,int len,const char *data);
int intif_guild_castle_dataload(int castle_id,int index);
int intif_guild_castle_datasave(int castle_id,int index, int value);

int intif_create_pet(int account_id,int char_id,short pet_type,short pet_lv,short pet_egg_id,
	short pet_equip,short intimate,short hungry,char rename_flag,char incuvate,char *pet_name);
int intif_request_petdata(int account_id,int char_id,int pet_id);
int intif_save_petdata(int account_id,struct s_pet *p);
int intif_delete_petdata(int pet_id);

#endif
