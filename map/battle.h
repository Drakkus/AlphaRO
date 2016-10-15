#ifndef _BATTLE_H_
#define _BATTLE_H_

// �_���[�W
struct Damage {
	int damage,damage2;
	int type,div_;
	int amotion,dmotion;
	int blewcount;
};

// �����\�i�ǂݍ��݂�pc.c�Abattle_attr_fix�Ŏg�p�j
extern int attr_fix_table[4][10][10];

struct map_session_data;
struct mob_data;
struct block_list;

// �_���[�W�v�Z

struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_weapon_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);

// �����C���v�Z
int battle_attr_fix(int damage,int atk_elem,int def_elem);

// �_���[�W�ŏI�v�Z
int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag);
enum {	// �ŏI�v�Z�̃t���O
	BF_WEAPON	= 0x0001,
	BF_MAGIC	= 0x0002,
	BF_MISC		= 0x0004,
	BF_SHORT	= 0x0010,
	BF_LONG		= 0x0040,
	BF_SKILL	= 0x0100,
	BF_NORMAL	= 0x0200,
	BF_WEAPONMASK=0x000f,
	BF_RANGEMASK= 0x00f0,
	BF_SKILLMASK= 0x0f00,
};

// ���ۂ�HP�𑝌�
int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage);
int battle_damage(struct block_list *bl,struct block_list *target,int damage);
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp);

// �U����ړ����~�߂�
int battle_stopattack(struct block_list *bl);
int battle_stopwalking(struct block_list *bl,int type);

// �ʏ�U�������܂Ƃ�
int battle_weapon_attack( struct block_list *bl,struct block_list *target,
	 unsigned int tick,int flag);

// �e��p�����[�^�𓾂�
int battle_counttargeted(struct block_list *bl,struct block_list *src);
int battle_get_class(struct block_list *bl);
int battle_get_dir(struct block_list *bl);
int battle_get_lv(struct block_list *bl);
int battle_get_range(struct block_list *bl);
int battle_get_hp(struct block_list *bl);
int battle_get_max_hp(struct block_list *bl);
int battle_get_str(struct block_list *bl);
int battle_get_agi(struct block_list *bl);
int battle_get_vit(struct block_list *bl);
int battle_get_int(struct block_list *bl);
int battle_get_dex(struct block_list *bl);
int battle_get_luk(struct block_list *bl);
int battle_get_hit(struct block_list *bl);
int battle_get_flee(struct block_list *bl);
int battle_get_def(struct block_list *bl);
int battle_get_mdef(struct block_list *bl);
int battle_get_flee2(struct block_list *bl);
int battle_get_def2(struct block_list *bl);
int battle_get_mdef2(struct block_list *bl);
int battle_get_baseatk(struct block_list *bl);
int battle_get_atk(struct block_list *bl);
int battle_get_atk2(struct block_list *bl);
int battle_get_speed(struct block_list *bl);
int battle_get_adelay(struct block_list *bl);
int battle_get_amotion(struct block_list *bl);
int battle_get_dmotion(struct block_list *bl);
int battle_get_element(struct block_list *bl);
int battle_get_attack_element(struct block_list *bl);
int battle_get_attack_element2(struct block_list *bl);  //���蕐�푮���擾
#define battle_get_elem_type(bl)	(battle_get_element(bl)%10)
#define battle_get_elem_level(bl)	(battle_get_element(bl)/10/2)
int battle_get_party_id(struct block_list *bl);
int battle_get_guild_id(struct block_list *bl);
int battle_get_race(struct block_list *bl);
int battle_get_size(struct block_list *bl);
int battle_get_mode(struct block_list *bl);
int battle_get_mexp(struct block_list *bl);

struct status_change *battle_get_sc_data(struct block_list *bl);
short *battle_get_sc_count(struct block_list *bl);
short *battle_get_opt1(struct block_list *bl);
short *battle_get_opt2(struct block_list *bl);
short *battle_get_option(struct block_list *bl);


enum {
	BCT_NOENEMY	=0x00000,
	BCT_PARTY	=0x10000,
	BCT_ENEMY	=0x40000,
	BCT_NOPARTY	=0x50000,
	BCT_ALL		=0x20000,
	BCT_NOONE	=0x60000,
};

int battle_check_undead(int race,int element);
int battle_check_target( struct block_list *src, struct block_list *target,int flag);
int battle_check_range(struct block_list *src,struct block_list *bl,int range);


// �ݒ�
extern struct Battle_Config {
	int warp_point_debug;
	int enemy_critical;
	int enemy_critical_rate;
	int enemy_perfect_flee;
	int cast_rate,delay_rate,delay_dependon_dex;
	int sdelay_attack_enable;
	int left_cardfix_to_right;
	int pc_skill_add_range;
	int skill_out_range_consume;
	int mob_skill_add_range;
	int pc_damage_delay;
	int pc_damage_delay_rate;
	int defnotenemy;
	int random_monster_checklv;
	int attr_recover;
	int flooritem_lifetime;
	int item_first_get_time;
	int item_second_get_time;
	int item_third_get_time;
	int mvp_item_first_get_time;
	int mvp_item_second_get_time;
	int mvp_item_third_get_time;
	int base_exp_rate,job_exp_rate;
	int drop_rate0item;
	int death_penalty_type;
	int death_penalty_base,death_penalty_job;
	int restart_hp_rate;
	int restart_sp_rate;
	int mvp_item_rate,mvp_exp_rate;
	int mvp_hp_rate;
	int monster_hp_rate;
	int monster_max_aspd;
	int atc_gmonly,gm_allskill;
	int skillfree;
	int skillup_limit;
	int wp_rate;
	int pp_rate;
	int monster_active_enable;
	int monster_damage_delay_rate;
	int monster_loot_type;
	int mob_skill_use;
	int mob_count_rate;
	int quest_skill_learn;
	int quest_skill_reset;
	int basic_skill_check;
	int guild_emperium_check;
	int pc_invincible_time;
	int pet_catch_rate;
	int pet_rename;
	int pet_friendly_rate;
	int pet_hungry_delay_rate;
	int pet_status_support;
	int pet_attack_support;
	int pet_damage_support;
	int pet_support_rate;
	int pet_attack_exp_to_master;
	int pet_attack_exp_rate;
	int skill_min_damage;
	int finger_offensive_type;
	int heal_exp,shop_exp;
	int combo_delay_rate;
	int item_check;
	int wedding_modifydisplay;
	int natural_healhp_interval;
	int natural_healsp_interval;
	int natural_heal_skill_interval;
	int natural_heal_weight_rate;
	int arrow_decrement;
	int max_aspd;
	int max_hp;
	int max_sp;
	int max_parameter;
	int max_cart_weight;
	int pc_skill_log;
	int mob_skill_log;
	int battle_log;
	int save_log;
	int error_log;
	int etc_log;
	int save_clothcolor;
	int undead_detect_type;
	int pc_auto_counter_type;
	int monster_auto_counter_type;
	int agi_penaly_type;
	int agi_penaly_count;
	int agi_penaly_num;
	int vit_penaly_type;
	int vit_penaly_count;
	int vit_penaly_num;
	int pc_skill_reiteration;
	int monster_skill_reiteration;
	int pc_cloak_check_wall;
	int monster_cloak_check_wall;
	int gvg_short_damage_rate;
	int gvg_long_damage_rate;
	int gvg_magic_damage_rate;
	int gvg_misc_damage_rate;
	int gvg_eliminate_time;
	int mob_changetarget_byskill;
	int item_rate_common,item_rate_card,item_rate_equip;	// Added by RoVeRT
	int item_drop_common_min,item_drop_common_max;	// Added by TyrNemesis^
	int item_drop_card_min,item_drop_card_max;
	int item_drop_equip_min,item_drop_equip_max;
	int item_drop_mvp_min,item_drop_mvp_max;	// End Addition

	int prevent_logout;	// Added by RoVeRT
} battle_config;

#define BATTLE_CONF_FILENAME	"conf/battle_alpha.conf"
int battle_config_read(const char *cfgName);

#endif
