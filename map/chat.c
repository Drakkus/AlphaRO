// $Id: chat.c,v 1.4 2004/01/19 17:47:48 rovert Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "map.h"
#include "battle.h"
#include "clif.h"
#include "pc.h"
#include "chat.h"
#include "npc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

/*==========================================
 * �`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createchat(struct map_session_data *sd,int limit,int pub,char* pass,char* title,int titlelen)
{
	struct chat_data *cd;

	cd = malloc(sizeof(*cd));
	if(cd==NULL){
		printf("out of memory : chat_createchat\n");
		exit(1);
	}
	memset(cd,0,sizeof(*cd));

	cd->limit = limit;
	cd->pub = pub;
	cd->users = 1;
	memcpy(cd->pass,pass,8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->owner = (struct block_list **)(&cd->usersd[0]);
	cd->usersd[0] = sd;
	cd->bl.m = sd->bl.m;
	cd->bl.x = sd->bl.x;
	cd->bl.y = sd->bl.y;
	cd->bl.type = BL_CHAT;

	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		clif_createchat(sd,1);
		free(cd);
		return 0;
	}
	pc_setchatid(sd,cd->bl.id);

	clif_createchat(sd,0);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �����`���b�g���[���ɎQ��
 *------------------------------------------
 */
int chat_joinchat(struct map_session_data *sd,int chatid,char* pass)	// Modified by RoVeRT
{
	struct chat_data *cd;

	cd=(struct chat_data*)map_id2bl(chatid);
	if(cd==NULL)
		return 1;

	if(cd->bl.m != sd->bl.m || cd->limit <= cd->users){
		clif_joinchatfail(sd,0);
		return 0;
	}
	if(cd->pub==0 && strncmp(pass,cd->pass,8)){
		clif_joinchatfail(sd,1);
		return 0;
	}

	cd->usersd[cd->users] = sd;
	cd->users++;

	pc_setchatid(sd,cd->bl.id);

	// �V���ɎQ�������l�ɂ͑S���̃��X�g
	clif_joinchatok(sd,cd);

	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
	clif_addchat(cd,sd);

	// ���͂̐l�ɂ͐l���ω���
	clif_dispchat(cd,0);

	// �����ŃC�x���g����`����Ă�Ȃ���s
	if(cd->users>=cd->trigger && cd->npc_event[0])
		npc_event(sd,cd->npc_event,0);
		
	return 0;
}

/*==========================================
 * �`���b�g���[�����甲����
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data *sd)
{
	struct chat_data *cd;
	int i,leavechar;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL)
		return 1;

	for(i = 0,leavechar=-1;i < cd->users;i++){
		if(cd->usersd[i] == sd){
			leavechar=i;
			break;
		}
	}
	if(leavechar<0)	// ����chat�ɏ������Ă��Ȃ��炵�� (�o�O���̂�)
		return -1;

	if(leavechar==0 && cd->users>1 && (*cd->owner)->type==BL_PC){
		// ���L�҂�����&���ɐl������&PC�̃`���b�g
		clif_changechatowner(cd,cd->usersd[1]);
		clif_clearchat(cd,0);
	}

	// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
	clif_leavechat(cd,sd);

	cd->users--;
	pc_setchatid(sd,0);

	if(cd->users == 0 && (*cd->owner)->type==BL_PC){
			// �S�����Ȃ��Ȃ���&PC�̃`���b�g�Ȃ̂ŏ���
		clif_clearchat(cd,0);
		map_delobject(cd->bl.id);	// free�܂ł��Ă����
	} else {
		for(i=leavechar;i < cd->users;i++)
			cd->usersd[i] = cd->usersd[i+1];
		if(leavechar==0 && (*cd->owner)->type==BL_PC){
			// PC�̃`���b�g�Ȃ̂ŏ��L�҂��������̂ňʒu�ύX
			cd->bl.x=cd->usersd[0]->bl.x;
			cd->bl.y=cd->usersd[0]->bl.y;
		}
		clif_dispchat(cd,0);
	}

	return 0;
}

/*==========================================
 * �`���b�g���[���̎����������
 *------------------------------------------
 */
int chat_changechatowner(struct map_session_data *sd,char *nextownername)
{
	struct chat_data *cd;
	struct map_session_data *tmp_sd;
	int i,nextowner;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd!=(*cd->owner))
		return 1;

	for(i = 1,nextowner=-1;i < cd->users;i++){
		if(strcmp(cd->usersd[i]->status.name,nextownername)==0){
			nextowner=i;
			break;
		}
	}
	if(nextowner<0) // ����Ȑl�͋��Ȃ�
		return -1;

	clif_changechatowner(cd,cd->usersd[nextowner]);
	// ��U����
	clif_clearchat(cd,0);

	// userlist�̏��ԕύX (0�����L�҂Ȃ̂�)
	tmp_sd = cd->usersd[0];
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	// �V�������L�҂̈ʒu�֕ύX
	cd->bl.x=cd->usersd[0]->bl.x;
	cd->bl.y=cd->usersd[0]->bl.y;

	// �ēx�\��
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g�̏��(�^�C�g����)��ύX
 *------------------------------------------
 */
int chat_changechatstatus(struct map_session_data *sd,int limit,int pub,char* pass,char* title,int titlelen)
{
	struct chat_data *cd;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd!=(*cd->owner))
		return 1;

	cd->limit = limit;
	cd->pub = pub;
	memcpy(cd->pass,pass,8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	clif_changechatstatus(cd);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g���[������R��o��
 *------------------------------------------
 */
int chat_kickchat(struct map_session_data *sd,char *kickusername)
{
	struct chat_data *cd;
	int i,kickuser;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd!=(*cd->owner))
		return 1;

	for(i = 0,kickuser=-1;i < cd->users;i++){
		if(strcmp(cd->usersd[i]->status.name,kickusername)==0){
			kickuser=i;
			break;
		}
	}
	if(kickuser<0) // ����Ȑl�͋��Ȃ�
		return -1;

	chat_leavechat(cd->usersd[kickuser]);

	return 0;
}

/*==========================================
 * npc�`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createcnpchat(struct npc_data *nd,int limit,int trigger,char* title,int titlelen,const char *ev)	// Modified by RoVeRT
{
	struct chat_data *cd;

	cd = malloc(sizeof(*cd));
	if(cd==NULL){
		printf("out of memory : chat_createchat\n");
		exit(1);
	}
	memset(cd,0,sizeof(*cd));

	cd->limit = limit;
	cd->trigger = trigger;
	cd->pub = 1;
	cd->users = 0;
	memcpy(cd->pass,"",8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->bl.m = nd->bl.m;
	cd->bl.x = nd->bl.x;
	cd->bl.y = nd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->owner_ = (struct block_list *)nd;
	cd->owner = &cd->owner_;
	memcpy(cd->npc_event,ev,sizeof(cd->npc_event));

	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		free(cd);
		return 0;
	}
	nd->chat_id=cd->bl.id;

	clif_dispchat(cd,0);

	return 0;
}

