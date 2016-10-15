#ifndef _LOGIN_H_
#define _LOGIN_H_

#define MAX_SERVERS 30

#define LOGIN_CONF_NAME	"conf/login_alpha.conf"
#define LOGIN_LAN_CONF_NAME     "conf/lan_support.conf"

#define PASSWORDENC		3	// �Í����p�X���[�h�ɑΉ�������Ƃ���`����
							// passwordencrypt�̂Ƃ���1�A
							// passwordencrypt2�̂Ƃ���2�ɂ���B
							// 3�ɂ���Ɨ����ɑΉ�

#define START_ACCOUNT_NUM	  2000000
#define END_ACCOUNT_NUM		100000000

struct mmo_account {
	char* userid;
	char* passwd;
	int passwdenc;

	long account_id;
	long login_id1;
	long login_id2;
	long char_id;
	char lastlogin[24];
	int sex;
};

struct mmo_char_server {
	char name[20];
	long ip;
	short port;
	int users;
	int maintenance;
	int new;
};


#endif
