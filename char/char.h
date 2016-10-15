#ifndef _CHAR_H_
#define _CHAR_H_

#define MAX_MAP_SERVERS 30

#define CHAR_CONF_NAME	"conf/char_alpha.conf"
#define LOGIN_LAN_CONF_NAME     "conf/lan_support.conf"

#define UNKNOWN_CHAR_NAME "Unknown"

#define DEFAULT_AUTOSAVE_INTERVAL 300*1000

struct mmo_map_server{
  long ip;
  short port;
  int users;
  char map[MAX_MAP_PER_SERVER][16];
};

int mapif_sendall(unsigned char *buf,unsigned int len);
int mapif_sendallwos(int fd,unsigned char *buf,unsigned int len);
int mapif_send(int fd,unsigned char *buf,unsigned int len);

extern int autosave_interval;

#endif
