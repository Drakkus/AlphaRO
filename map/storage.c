// storage.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "itemdb.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "storage.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

char stor_txt[]="storage.txt";
struct storage *storage;
int storage_num;

int do_init_storage(void) // map.c::do_init()����Ă΂��
{
/*	char line[65536];
	int i=0,set,tmp_int[2];
	FILE *fp;
	fp=fopen(stor_txt,"r");
	if(fp==NULL)
		return 0;
	while(fgets(line,65535,fp)){
		set=sscanf(line,"%d,%d",&tmp_int[0],&tmp_int[1]);
		if(set==2) {
			if(i==0){
				storage_num=1;
				storage=malloc(sizeof(struct storage));
			}else{
				storage=realloc(storage,sizeof(struct storage)*(++storage_num));
			}
			memset(&storage[i],0,sizeof(struct storage));
			storage[i].account_id=tmp_int[0];
			storage_fromstr(line,&storage[i]);
			i++;
		}
	}
	fclose(fp);*/
	storage_num=0;
	storage=NULL;
	return 1;
}

void do_final_storage(void) // map.c::do_final()����Ă΂��
{
/*	char line[65536];
	int i;
	FILE *fp;

	fp=fopen(stor_txt,"w");
	if(fp==NULL)
		return;
	for(i=0;i<storage_num;i++){
		storage_tostr(line,&storage[i]);
		fprintf(fp,"%s%c\n",line,0x0d);
	}
	fclose(fp);*/
}

int account2storage(int account_id)
{
	int i;
	for(i=0;i<storage_num;i++)
		if(account_id==storage[i].account_id)
			return i;
	if(i==0){
		storage=malloc(sizeof(struct storage));
		storage_num=1;
	}else{
		storage=realloc(storage,sizeof(struct storage)*(++storage_num));
	}
	memset(&storage[i],0,sizeof(struct storage));
	storage[i].account_id=account_id;
	return i;
}

/*==========================================
 * �J�v���q�ɂ��J��
 *------------------------------------------
 */
int storage_storageopen(struct map_session_data *sd)
{
	int i;
	for(i=0;i<storage_num;i++){
		if(sd->status.account_id == storage[i].account_id) {
			storage[i].storage_status=1;
			clif_storageitemlist(sd,&storage[i]);
			clif_storageequiplist(sd,&storage[i]);
			clif_updatestorageamount(sd,&storage[i]);
			return 0;
		}
	}
	intif_request_storage(sd->status.account_id);
/*
	struct storage *stor;

	stor=&storage[account2storage(sd->status.account_id)];
	stor->storage_status=1;
	clif_storageitemlist(sd,stor);
	clif_storageequiplist(sd,stor);
	clif_updatestorageamount(sd,stor);
*/
	return 0;
}

/*==========================================
 * �J�v���q�ɂփA�C�e���ǉ�
 *------------------------------------------
 */
int storage_additem(struct map_session_data *sd,struct storage *stor,struct item *item_data,int amount)
{
	int i;
	
	i=MAX_STORAGE;
	if(!itemdb_isequip(item_data->nameid)){
		// �����i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_STORAGE;i++){
			if(stor->storage[i].nameid==item_data->nameid){
				if(stor->storage[i].amount+amount > MAX_AMOUNT)
					return 1;
				stor->storage[i].amount+=amount;
				clif_storageitemadded(sd,stor,i,amount);
				break;
			}
		}
	}
	if(i==MAX_STORAGE){
		// �����i�������L�i�������̂ŋ󂫗��֒ǉ�
		for(i=MAX_STORAGE-1;i>=0;i--){
			if(stor->storage[i].nameid==0 &&
			   (i==0 || stor->storage[i-1].nameid)){
				memcpy(&stor->storage[i],item_data,sizeof(stor->storage[0]));
				stor->storage[i].amount=amount;
				stor->storage_amount++;
				
				clif_storageitemadded(sd,stor,i,amount);
				clif_updatestorageamount(sd,stor);
				break;
			}
		}
		if(i<0)
			return 1;
	}
	return 0;
}
/*==========================================
 * �J�v���q�ɃA�C�e�������炷
 *------------------------------------------
 */
int storage_delitem(struct map_session_data *sd,struct storage *stor,int n,int amount)
{
	if(stor->storage[n].nameid==0 ||
	   stor->storage[n].amount<amount)
		return 1;

	stor->storage[n].amount-=amount;
	if(stor->storage[n].amount==0){
		memset(&stor->storage[n],0,sizeof(stor->storage[0]));
		stor->storage_amount--;
		clif_updatestorageamount(sd,stor);
	}
	clif_storageitemremoved(sd,n,amount);

	return 0;
}
/*==========================================
 * �J�v���q�ɂ֓����
 *------------------------------------------
 */
int storage_storageadd(struct map_session_data *sd,int index,int amount)
{
//	int i;
	struct storage *stor;

	stor=&storage[account2storage(sd->status.account_id)];
	if( (stor->storage_amount <= MAX_STORAGE) && (stor->storage_status == 1) ) { // storage not full & storage open
		if(index>=0 && index<MAX_INVENTORY) { // valid index
			if( (amount <= sd->status.inventory[index].amount) && (amount > 0) ) { //valid amount
/*				int ep=0; // used for a check
				if(!itemdb_isequip(sd->status.inventory[index].nameid)){
					for(i=0;i<MAX_STORAGE;i++) {
						if(stor->storage[i].nameid == sd->status.inventory[index].nameid)
							break;
					}
					if(i<MAX_STORAGE)
						goto found;
				}
				for(i=0;i<MAX_STORAGE;i++) {
					if(stor->storage[i].amount==0)
						break;
				}
				if(i>=MAX_STORAGE)
					return 1;
				found:
				// add item to storage
				if(stor->storage[i].amount < 1) {
					stor->storage[i] = sd->status.inventory[index];
					stor->storage[i].amount = amount;
				}
				else {
					stor->storage[i].amount += amount;
					ep=1; // item type was already in storage -> dont raise storage amount
				}*/
				if(storage_additem(sd,stor,&sd->status.inventory[index],amount)==0)
				// remove item from inventory
					pc_delitem(sd,index,amount,0);
				// send packet
/*				clif_storageitemadded(sd,stor,i,amount);
				if(ep!=1) { // item type was not already in storage
					// update vars
					stor->storage_amount++;
					clif_updatestorageamount(sd,stor);
				}
				do_final_storage(); // map.exe���������Ƃ��̂��߂ɁA�O�̂��߃Z�[�u
				*/
			} // valid amount
		}// valid index
	}// storage not full & storage open

	return 0;
}

/*==========================================
 * �J�v���q�ɂ���o��
 *------------------------------------------
 */
int storage_storageget(struct map_session_data *sd,int index,int amount)
{
	struct storage *stor;
	int flag;

	stor=&storage[account2storage(sd->status.account_id)];
	if(stor->storage_status == 1) { //  storage open
		if(index>=0 && index<MAX_STORAGE) { // valid index
			if( (amount <= stor->storage[index].amount) && (amount > 0) ) { //valid amount
				if((flag = pc_additem(sd,&stor->storage[index],amount)) == 0){
/*					stor->storage[index].amount -= amount; // new amount of item in storage
					// send packet
					clif_storageitemremoved(sd,index,amount);
					if(stor->storage[index].amount < 1) { // no item left in storage
						// update vars
						stor->storage[index].nameid=0;
						stor->storage_amount--;
						clif_updatestorageamount(sd,stor);
					}
					do_final_storage(); // map.exe���������Ƃ��̂��߂ɁA�O�̂��߃Z�[�u
*/
					storage_delitem(sd,stor,index,amount);
				}
				else
					clif_additem(sd,0,0,flag);
			} // valid amount
		}// valid index
	}// storage open

	return 0;
}
/*==========================================
 * �J�v���q�ɂփJ�[�g��������
 *------------------------------------------
 */
int storage_storageaddfromcart(struct map_session_data *sd,int index,int amount)
{
	struct storage *stor;

	stor=&storage[account2storage(sd->status.account_id)];
	if( (stor->storage_amount <= MAX_STORAGE) && (stor->storage_status == 1) ) { // storage not full & storage open
		if(index>=0 && index<MAX_INVENTORY) { // valid index
			if( (amount <= sd->status.cart[index].amount) && (amount > 0) ) { //valid amount
				if(storage_additem(sd,stor,&sd->status.cart[index],amount)==0)
					pc_cart_delitem(sd,index,amount,0);
			} // valid amount
		}// valid index
	}// storage not full & storage open

	return 0;
}

/*==========================================
 * �J�v���q�ɂ���J�[�g�֏o��
 *------------------------------------------
 */
int storage_storagegettocart(struct map_session_data *sd,int index,int amount)
{
	struct storage *stor;

	stor=&storage[account2storage(sd->status.account_id)];
	if(stor->storage_status == 1) { //  storage open
		if(index>=0 && index<MAX_STORAGE) { // valid index
			if( (amount <= stor->storage[index].amount) && (amount > 0) ) { //valid amount
				if(pc_cart_additem(sd,&stor->storage[index],amount)==0){
					storage_delitem(sd,stor,index,amount);
				}
			} // valid amount
		}// valid index
	}// storage open

	return 0;
}


/*==========================================
 * �J�v���q�ɂ����
 *------------------------------------------
 */
int storage_storageclose(struct map_session_data *sd)
{
	struct storage *stor;

	stor=&storage[account2storage(sd->status.account_id)];
	stor->storage_status=0;
	clif_storageclose(sd);
//	do_final_storage(); // map.exe���������Ƃ��̂��߂ɁA�O�̂��߃Z�[�u
	return 0;
}

/*==========================================
 * ���O�A�E�g���J���Ă���J�v���q�ɂ̕ۑ�
 *------------------------------------------
 */
int storage_storage_quit(struct map_session_data *sd)
{
	int i,account_id=sd->status.account_id;
	for(i=0;i<storage_num;i++){
		if(account_id==storage[i].account_id){
			if(storage[i].storage_status) {
				storage[i].storage_status=0;
				break;
			}
		}
	}
	return 0;
}

int storage_storage_save(struct map_session_data *sd)
{
	int i,account_id=sd->status.account_id;
	for(i=0;i<storage_num;i++){
		if(account_id==storage[i].account_id) {
			intif_send_storage(account_id);
			break;
		}
	}
	return 0;
}
