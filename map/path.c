// $Id: path.c,v 1.7 2004/01/28 00:06:40 rovert Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "battle.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

//#define PATH_STANDALONETEST

#define MAX_HEAP 150
struct tmp_path { short x,y,dist,before,cost; char dir,flag;};
#define calc_index(x,y) (((x)+(y)*32) & 1023)

/*==========================================
 * �o�H�T���⏕heap push
 *------------------------------------------
 */
static inline void push_heap_path(int *heap,struct tmp_path *tp,int index)
{
	int i,h;

	heap[0]++;

	for(h=heap[0]-1,i=(h-1)/2;
		h>0 && tp[index].cost<tp[heap[i+1]].cost;
		i=(h-1)/2)
		heap[h+1]=heap[i+1],h=i;
	heap[h+1]=index;
}

/*==========================================
 * �o�H�T���⏕heap update
 * cost���������̂ō��̕��ֈړ�
 *------------------------------------------
 */
static inline void update_heap_path(int *heap,struct tmp_path *tp,int index)
{
	int i,h;

	for(h=0;h<heap[0];h++)
		if(heap[h+1]==index)
			break;
	if(h==heap[0]){
		fprintf(stderr,"update_heap_path bug\n");
		exit(1);
	}
	for(i=(h-1)/2;
		h>0 && tp[index].cost<tp[heap[i+1]].cost;
		i=(h-1)/2)
		heap[h+1]=heap[i+1],h=i;
	heap[h+1]=index;
}

/*==========================================
 * �o�H�T���⏕heap pop
 *------------------------------------------
 */
static int pop_heap_path(int *heap,struct tmp_path *tp)
{
	int i,h,k;
	int ret,last;

	if(heap[0]<=0)
		return -1;
	ret=heap[1];
	last=heap[heap[0]];
	heap[0]--;

	for(h=0,k=2;k<heap[0];k=k*2+2){
		if(tp[heap[k+1]].cost>tp[heap[k]].cost)
			k--;
		heap[h+1]=heap[k+1], h=k;
	}
	if(k==heap[0])
		heap[h+1]=heap[k], h=k-1;

	for(i=(h-1)/2;
		h>0 && tp[heap[i+1]].cost>tp[last].cost;
		i=(h-1)/2)
		heap[h+1]=heap[i+1],h=i;
	heap[h+1]=last;

	return ret;
}

/*==========================================
 * ���݂̓_��cost�v�Z
 *------------------------------------------
 */
static inline int calc_cost(struct tmp_path *p,int x1,int y1)
{
	int xd,yd;
	xd=x1-p->x;
	if(xd<0) xd=-xd;
	yd=y1-p->y;
	if(yd<0) yd=-yd;
	return (xd+yd)*10+p->dist;
}

/*==========================================
 * �K�v�Ȃ�path��ǉ�/�C������
 *------------------------------------------
 */
static int add_path(int *heap,struct tmp_path *tp,int x,int y,int dist,int dir,int before,int x1,int y1)
{
	int i;

	i=calc_index(x,y);

	if(tp[i].x==x && tp[i].y==y){
		if(tp[i].dist>dist){
			tp[i].dist=dist;
			tp[i].dir=dir;
			tp[i].before=before;
			tp[i].cost=calc_cost(&tp[i],x1,y1);
			if(tp[i].flag)
				push_heap_path(heap,tp,i);
			else
				update_heap_path(heap,tp,i);
			tp[i].flag=0;
		}
		return 0;
	}

	if(tp[i].x || tp[i].y)
		return 1;

	tp[i].x=x;
	tp[i].y=y;
	tp[i].dist=dist;
	tp[i].dir=dir;
	tp[i].before=before;
	tp[i].cost=calc_cost(&tp[i],x1,y1);
	tp[i].flag=0;
	push_heap_path(heap,tp,i);

	return 0;
}


/*==========================================
 * (x,y)���ړ��s�\�n�т��ǂ���
 * flag 0x10000 �������U������
 *------------------------------------------
 */
static inline int can_place(struct map_data *m,int x,int y,int flag)
{
	int c=read_gatp(m,x,y);
	if(c==1)
		return 0;
	return 1;
}

/*==========================================
 * (x0,y0)����(x1,y1)��1���ňړ��\���v�Z
 *------------------------------------------
 */
static inline int can_move(struct map_data *m,int x0,int y0,int x1,int y1,int flag)
{
	if(x0-x1<-1 || x0-x1>1 || y0-y1<-1 || y0-y1>1)
		return 0;
	if(x1<0 || y1<0 || x1>=m->xs || y1>=m->ys)
		return 0;
	if(!can_place(m,x0,y0,flag))
		return 0;
	if(!can_place(m,x1,y1,flag))
		return 0;
	if(x0==x1 || y0==y1)
		return 1;
	if(!can_place(m,x0,y1,flag) || !can_place(m,x1,y0,flag))
		return 0;
	return 1;
}
/*==========================================
 * (x0,y0)����(dx,dy)������count�Z����
 * ������΂������Ƃ̍��W������
 *------------------------------------------
 */
int path_blownpos(int m,int x0,int y0,int dx,int dy,int count)
{
	struct map_data *md;

	if(!map[m].gat)
		return -1;
	md=&map[m];

	if(count>15){	// �ő�10�}�X�ɐ���
		if(battle_config.error_log)
			printf("path_blownpos: count too many %d !\n",count);
		count=15;
	}
	if(dx>1 || dx<-1 || dy>1 || dy<-1){
		if(battle_config.error_log)
			printf("path_blownpos: illeagal dx=%d or dy=%d !\n",dx,dy);
		dx=(dx>=0)?1:((dx<0)?-1:0);
		dy=(dy>=0)?1:((dy<0)?-1:0);
	}
	
	while( (count--)>0 && (dx!=0 || dy!=0) ){
		if( !can_move(md,x0,y0,x0+dx,y0+dy,0) ){
			int fx=(dx!=0 && can_move(md,x0,y0,x0+dx,y0,0));
			int fy=(dy!=0 && can_move(md,x0,y0,x0,y0+dy,0));
			if( fx && fy ){
				if(rand()&1) dx=0;
				else		 dy=0;
			}
			if( !fx )		dx=0;
			if( !fy )		dy=0;
		}
		x0+=dx;
		y0+=dy;
	}
	return (x0<<16)|y0;
}

/*==========================================
 * path�T�� (x0,y0)->(x1,y1)
 *------------------------------------------
 */
int path_search(struct walkpath_data *wpd,int m,int x0,int y0,int x1,int y1,int flag)
{
	int heap[MAX_HEAP+1];
	struct tmp_path tp[32*32];
	int i,rp,x,y;
	struct map_data *md;
	int dx,dy;

	if(!map[m].gat)
		return -1;
	md=&map[m];
	if(x1<0 || x1>=md->xs || y1<0 || y1>=md->ys || (i=read_gatp(md,x1,y1))==1)
		return -1;

	// easy
	dx = (x1-x0<0) ? -1 : 1;
	dy = (y1-y0<0) ? -1 : 1;
	for(x=x0,y=y0,i=0;x!=x1 || y!=y1;){
		if(i>=sizeof(wpd->path))
			return -1;
		if(x!=x1 && y!=y1){
			if(!can_move(md,x,y,x+dx,y+dy,flag))
				break;
			x+=dx;
			y+=dy;
			wpd->path[i++]=(dx<0) ? ((dy>0)? 1 : 3) : ((dy<0)? 5 : 7);
		} else if(x!=x1){
			if(!can_move(md,x,y,x+dx,y   ,flag))
				break;
			x+=dx;
			wpd->path[i++]=(dx<0) ? 2 : 6;
		} else { // y!=y1
			if(!can_move(md,x,y,x   ,y+dy,flag))
				break;
			y+=dy;
			wpd->path[i++]=(dy>0) ? 0 : 4;
		}
		if(x==x1 && y==y1){
			wpd->path_len=i;
			wpd->path_pos=0;
			wpd->path_half=0;
			return 0;
		}
	}
	if(flag&1)
		return -1;

	memset(tp,0,sizeof(tp));

	i=calc_index(x0,y0);
	tp[i].x=x0;
	tp[i].y=y0;
	tp[i].dist=0;
	tp[i].dir=0;
	tp[i].before=0;
	tp[i].cost=calc_cost(&tp[i],x1,y1);
	tp[i].flag=0;
	heap[0]=0;
	push_heap_path(heap,tp,calc_index(x0,y0));
	while(1){
		int e=0,fromdir;

		if(heap[0]==0)
			return -1;
		rp=pop_heap_path(heap,tp);
		x=tp[rp].x;
		y=tp[rp].y;
		if(x==x1 && y==y1){
			int len,j;

			for(len=0,i=rp;len<100 && i!=calc_index(x0,y0);i=tp[i].before,len++);
			if(len==100 || len>=sizeof(wpd->path))
				return -1;
			wpd->path_len=len;
			wpd->path_pos=0;
			wpd->path_half=0;
			for(i=rp,j=len-1;j>=0;i=tp[i].before,j--)
				wpd->path[j]=tp[i].dir;

			return 0;
		}
		fromdir=tp[rp].dir;
		if(can_move(md,x,y,x+1,y-1,flag))
			e+=add_path(heap,tp,x+1,y-1,tp[rp].dist+14,5,rp,x1,y1);
		if(can_move(md,x,y,x+1,y  ,flag))
			e+=add_path(heap,tp,x+1,y  ,tp[rp].dist+10,6,rp,x1,y1);
		if(can_move(md,x,y,x+1,y+1,flag))
			e+=add_path(heap,tp,x+1,y+1,tp[rp].dist+14,7,rp,x1,y1);
		if(can_move(md,x,y,x  ,y+1,flag))
			e+=add_path(heap,tp,x  ,y+1,tp[rp].dist+10,0,rp,x1,y1);
		if(can_move(md,x,y,x-1,y+1,flag))
			e+=add_path(heap,tp,x-1,y+1,tp[rp].dist+14,1,rp,x1,y1);
		if(can_move(md,x,y,x-1,y  ,flag))
			e+=add_path(heap,tp,x-1,y  ,tp[rp].dist+10,2,rp,x1,y1);
		if(can_move(md,x,y,x-1,y-1,flag))
			e+=add_path(heap,tp,x-1,y-1,tp[rp].dist+14,3,rp,x1,y1);
		if(can_move(md,x,y,x  ,y-1,flag))
			e+=add_path(heap,tp,x  ,y-1,tp[rp].dist+10,4,rp,x1,y1);
		tp[rp].flag=1;
		if(e || heap[0]>=MAX_HEAP-5)
			return -1;
	}
	return -1;
}

#ifdef PATH_STANDALONETEST
char gat[64][64]={
	{0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0},
};
struct map_data map[1];

/*==========================================
 * �o�H�T�����[�`���P�̃e�X�g�pmain�֐�
 *------------------------------------------
 */
void main(int argc,char *argv[])
{
	struct walkpath_data wpd;

	map[0].gat=gat;
	map[0].xs=64;
	map[0].ys=64;

	path_search(&wpd,0,3,4,5,4);
	path_search(&wpd,0,5,4,3,4);
	path_search(&wpd,0,6,4,3,4);
	path_search(&wpd,0,7,4,3,4);
	path_search(&wpd,0,4,3,4,5);
	path_search(&wpd,0,4,2,4,5);
	path_search(&wpd,0,4,1,4,5);
	path_search(&wpd,0,4,5,4,3);
	path_search(&wpd,0,4,6,4,3);
	path_search(&wpd,0,4,7,4,3);
	path_search(&wpd,0,7,4,3,4);
	path_search(&wpd,0,8,4,3,4);
	path_search(&wpd,0,9,4,3,4);
	path_search(&wpd,0,10,4,3,4);
	path_search(&wpd,0,11,4,3,4);
	path_search(&wpd,0,12,4,3,4);
	path_search(&wpd,0,13,4,3,4);
	path_search(&wpd,0,14,4,3,4);
	path_search(&wpd,0,15,4,3,4);
	path_search(&wpd,0,16,4,3,4);
	path_search(&wpd,0,17,4,3,4);
	path_search(&wpd,0,18,4,3,4);
}
#endif
