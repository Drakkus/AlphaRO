#ifndef _ITEMDB_H_
#define _ITEMDB_H_

#include "map.h"

struct item_data {
	int nameid;
	char name[24],jname[24];
	int value_buy;
	int value_sell;
	int type;
	int class;
	int sex;
	int equip;
	int weight;
	int atk;
	int def;
	int range;
	int slot;
	int look;
	int elv;
	int wlv;
	char *use_script;	// �񕜂Ƃ����S�����̒��ł�낤���Ȃ�
	char *equip_script;	// �U��,�h��̑����ݒ�����̒��ŉ\����?
	char desc[500];
	struct {
		unsigned available : 1;
		unsigned value_notdc : 1;
		unsigned value_notoc : 1;
		unsigned no_equip : 3;
	} flag;
	int view_id;
};

struct random_item_data {
	int nameid;
	int per;
};

struct item_data* itemdb_searchname(const char *name);
struct item_data* itemdb_search(int nameid);
struct item_data* itemdb_exists(int nameid);
#define itemdb_type(n) itemdb_search(n)->type
#define itemdb_atk(n) itemdb_search(n)->atk
#define itemdb_def(n) itemdb_search(n)->def
#define itemdb_look(n) itemdb_search(n)->look
#define itemdb_weight(n) itemdb_search(n)->weight
#define itemdb_equip(n) itemdb_search(n)->equip
#define itemdb_usescript(n) itemdb_search(n)->use_script
#define itemdb_equipscript(n) itemdb_search(n)->equip_script
#define itemdb_wlv(n) itemdb_search(n)->wlv
#define itemdb_range(n) itemdb_search(n)->range
#define itemdb_slot(n) itemdb_search(n)->slot
#define	itemdb_available(n) (itemdb_exists(n) && itemdb_search(n)->flag.available)
#define	itemdb_viewid(n) (itemdb_search(n)->view_id)


int itemdb_searchrandomid(int flags);

#define itemdb_value_buy(n) itemdb_search(n)->value_buy
#define itemdb_value_sell(n) itemdb_search(n)->value_sell
#define itemdb_value_notdc(n) itemdb_search(n)->flag.value_notdc
#define itemdb_value_notoc(n) itemdb_search(n)->flag.value_notoc
int itemdb_isequip(int);
int itemdb_isequip2(struct item_data *);
// itemdb_equip�}�N����itemdb_equippoint�Ƃ̈Ⴂ��
// �O�҂��I��db�Œ�`���ꂽ�l���̂��̂�Ԃ��̂ɑ΂�
// ��҂�sessiondata���l�������Ƒ��ł̑����\�ꏊ
// ���ׂĂ̑g�ݍ��킹��Ԃ�

void do_final_itemdb(void);
int do_init_itemdb(void);

#endif
