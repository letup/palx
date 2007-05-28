#include "internal.h"
#include "structs.h"
#include "game.h"
#include "scene.h"

#include "stdlib.h"

extern int scale;

extern int process_Battle(uint16_t,uint16_t);

BITMAP *backup=0;
void destroyit(){destroy_bitmap(backup);}

inline void sync_viewport()
{
	viewport_x_bak=game->rpg.viewport_x;
	viewport_y_bak=game->rpg.viewport_y;
	game->rpg.viewport_x=scene->team_pos.toXY().x-x_scrn_offset;
	game->rpg.viewport_y=scene->team_pos.toXY().y-y_scrn_offset;
}
inline void backup_position()
{
	abstract_x_bak=scene->team_pos.toXY().x;
	abstract_y_bak=scene->team_pos.toXY().y;
}
inline int16_t absdec(int16_t &s)
{
	int16_t s0=s;
	if(s>0)	s--;
	else if(s<0) s++;
	return s0;
}
typedef std::vector<EVENT_OBJECT>::iterator evt_obj;
void GameLoop_OneCycle(bool trigger)
{
	if(trigger)
		for(evt_obj iter=scene->sprites_begin;iter!=scene->sprites_end&&game->rpg.scene_id==map_toload;++iter)
			if(absdec(iter->vanish_time)==0)
				if(iter->status>0 && iter->trigger_method>=4){
					if(abs(scene->team_pos.x-iter->pos_x)+abs(scene->team_pos.y-iter->pos_y)*2<(iter->trigger_method-4)*32+16)// in the distance that can trigger
					{
						if(iter->frames)
						{
							stop_and_update_frame();
							iter->curr_frame=0;
							iter->direction=calc_faceto(scene->team_pos.toXY().x-iter->pos_x,scene->team_pos.toXY().y-iter->pos_y);
							redraw_everything();
						}
						//x_off=0,y_off=0;
						uint16_t &triggerscript=iter->trigger_script;
						triggerscript=process_script(triggerscript,(int16_t)(iter-game->evtobjs.begin()));
					}
				}else if(iter->status<0){//&& in the screen
					iter->status=abs(iter->status);
					//step==0
				}
	for(evt_obj iter=scene->sprites_begin;iter!=scene->sprites_end&&game->rpg.scene_id==map_toload;++iter)
	{
		if(iter->status!=0)
			if(uint16_t &autoscript=iter->auto_script)
				autoscript=process_autoscript(autoscript,(int16_t)(iter-game->evtobjs.begin()));
		if(iter->status==2 && iter->image>0 && trigger
			&& abs(iter->pos_x-scene->team_pos.toXY().x)+2*abs(iter->pos_y-scene->team_pos.toXY().y)<0xD)//&& beside role && face to it
		{
			//check barrier;this means, role status 2 means it takes place
			/*
			backup_position();
			for(int direction=(iter->direction+1)%4,i=0;i<4;direction=(direction+1)%4,i++)
				if(!barrier_check(0,scene->team_pos.toXY().x+direction_offs[direction][0],scene->team_pos.toXY().y+direction_offs[direction][1]))
				{
					scene->team_pos.toXY().x+=direction_offs[direction][0];
					scene->team_pos.toXY().y+=direction_offs[direction][1];
					break;
				}
			sync_viewport();
			scene->move_usable_screen();
			*/
		}
	}
}
void process_Explore()
{
	for(evt_obj iter=scene->sprites_begin;iter!=scene->sprites_end&&game->rpg.scene_id==map_toload;++iter)
	{
		if(iter->status==2 && iter->image>0 && abs(scene->team_pos.x-iter->pos_x)+abs(scene->team_pos.y-iter->pos_y)*2<iter->trigger_method*32+16
		   && (iter->pos_x-scene->team_pos.toXY().x)/direction_offs[game->rpg.team_direction][0]>=0
		   && (iter->pos_y-scene->team_pos.toXY().y)/direction_offs[game->rpg.team_direction][1]>=0)//&& beside role && face to it
		{
			iter->trigger_script=process_script(iter->trigger_script,iter-game->evtobjs.begin());
			return;
		}
	}
}
uint16_t process_script_entry(uint16_t func,int16_t param[],uint16_t id,int16_t object)
{
	//printf("%s\n",scr_desc(func,param).c_str());
	EVENT_OBJECT &obj=game->evtobjs[object];
	const int16_t &param1=param[0],&param2=param[1],&param3=param[2];
	char addition[100];memset(addition,0,sizeof(addition));
	int npc_speed,role_speed;
	switch(func){
		case 0x10:
			npc_speed=3;
__walk_npc:
			{
				int16_t x_diff=obj.pos_x-(param1*32+param3*16),y_diff=obj.pos_y-(param2*16+param3*8);
				obj.direction=calc_faceto(-x_diff,-y_diff);
				if(abs(x_diff)<npc_speed*2 || abs(y_diff)<npc_speed){
					obj.pos_x = param1*32+param3*16;
					obj.pos_y = param2*16+param3*8;
				}else{
					obj.pos_x += npc_speed*(x_diff<0 ? 2 : -2);
					obj.pos_y += npc_speed*(y_diff<0 ? 1 : -1);
				}
				obj.curr_frame=(obj.curr_frame+1)%(obj.frames?obj.frames:obj.frames_auto);

				//afterward check;MUST have,or will not match dospal exactly
				if(obj.pos_x==param1*32+param3*16 && obj.pos_y==param2*16+param3*8)
					obj.curr_frame=0;//printf(addition,"完成");
				else{
					//printf(addition,"当前X:%x,Y:%x  目的X:%x,Y:%x",obj.pos_x,obj.pos_y,(param1*32+param3*16),(param2*16+param3*8));
					--id;
				}
			}
			break;
		case 0x11:
			if((object&1) != flag_parallel_mutex){
				//printf(addition,"被堵塞 当前X:%x,Y:%x  目的X:%x,Y:%x",obj.pos_x,obj.pos_y,(param1*32+param3*16),(param2*16+param3*8));
				--id;
				break;
			}
			npc_speed=2;
			goto __walk_npc;
		case 0x14:
			obj.curr_frame=param1;
			break;
		case 0x15:
			game->rpg.team_direction=param1;
			game->rpg.team[param3].frame=param1*3+param2;
			break;
		case 0x16:
			(param1>0?game->evtobjs[param1]:obj).direction=param2;
			(param1>0?game->evtobjs[param1]:obj).curr_frame=param3;
			break;
		case 0x24:
			if(param1)
				(param1>0 ? game->evtobjs[param1] : obj).auto_script= param2;
			break;
		case 0x25:
			if(param1)
				(param1>0 ? game->evtobjs[param1] : obj).trigger_script= param2;
			break;
		case 0x3c:
			if(param1)
				sprite_prim().getsource(RGM.decode(param1,0)).getsprite(0)->blit_middle(screen,0x30*scale,0x37*scale);
			dialog_x=(param1?0x50:0xC)*scale;
			dialog_y=8*scale;
			frame_text_x=(param1?0x60:0x2C)*scale;
			frame_text_y=0x1A*scale;
			break;
		case 0x3d:
			if(param1)
				sprite_prim().getsource(RGM.decode(param1,0)).getsprite(0)->blit_middle(screen,0x10E*scale,0x90*scale);
			dialog_x=(param1?4:0xC)*scale;
			dialog_y=0x6C*scale;
			frame_text_x=(param1?0x14:0x2C)*scale;
			frame_text_y=0x7E*scale;
			break;
		case 0x3e:
			break;
		case 0x40:
			break;
		case 0x43:
			game->rpg.music=param1;
			rix->play(param1);
			break;
		case 0x44:
			GameLoop_OneCycle(false);
			redraw_everything();
			break;
		case 0x45:
			game->rpg.battle_music=param1;
			break;
		case 0x46:
			scene->team_pos.toXYH().x=param1;
			scene->team_pos.toXYH().y=param2;
			scene->team_pos.toXYH().h=param3;
			sync_viewport();
			scene->produce_one_screen();
			break;
		case 0x49:
			game->evtobjs[param1!=-1?param1:object].status=param2;
			break;
		case 0x50:
			pal_fade_out(param1==0?1:param1);
			break;
		case 0x51:
			pal_fade_in(param1==0?1:param1);
			break;
		case 0x52:
			obj.status=-obj.status;
			obj.vanish_time=(param1?param1:0x320);
			break;
		case 0x59:
			if(param1>0 && game->rpg.scene_id!=param1)
			{
				map_toload=param1;
				flag_to_load|=0xC;
				game->rpg.layer=0;
			}
			break;
		case 0x65:
			game->rpg.roles_properties.avator[param1]=param2;
			if(!flag_battling && param3)
				load_team_mgo();
			break;
		case 0x6c:			
			if(param1){
				EVENT_OBJECT &target=(param1>0 ? game->evtobjs[param1] : obj);
				target.pos_x += param2;
				target.pos_y += param3;
				if(!target.frames && !target.frames_auto){
					uint16_t *usrc=(uint16_t *)MGO.decode(target.image);
					target.frames_auto=usrc[0]-(usrc[usrc[0]-1]==0?1:0);
				}
				target.curr_frame=(target.curr_frame+1)%(target.frames?target.frames:target.frames_auto);
			}
			break;
		case 0x6e:
			backup_position();
			sync_viewport();
			game->rpg.viewport_x+=param1;
			game->rpg.viewport_y+=param2;
			game->rpg.layer=param3*8;
			if(param1&&param2){
				team_walk_one_step();
				scene->move_usable_screen();
			}
			break;
		case 0x6f:
			if(game->evtobjs[param1].status==param2){
				obj.status=param2;
				//printf(addition,"成功");
			}//else
				//printf(addition,"失败");
			break;
		case 0x70:
			role_speed=2;
__walk_role:
			{
				int16_t x_diff,y_diff;
				while((x_diff=scene->team_pos.x-(param1*32+param3*16)) && (y_diff=scene->team_pos.y-(param2*16+param3*8))){
					backup_position();
					scene->team_pos.toXY().x += role_speed*(x_diff<0 ? 2 : -2);
					scene->team_pos.toXY().y += role_speed*(y_diff<0 ? 1 : -1);
					game->rpg.team_direction=calc_faceto(scene->team_pos.x-abstract_x_bak,scene->team_pos.y-abstract_y_bak);
					sync_viewport();
					team_walk_one_step();
					GameLoop_OneCycle(false);
					scene->move_usable_screen();
					redraw_everything();
				}
			}
			break;
		case 0x73:
			//clear_effective(param1,param2);
			break;
		case 0x75:
			game->rpg.team[0].role=(param2-1<0?0:param2-1);
			game->rpg.team[1].role=(param2-1<0?0:param2-1);
			game->rpg.team[2].role=(param3-1<0?0:param3-1);
			game->rpg.team_roles=(param1?1:0)+(param2?1:0)+(param3?1:0)-1;
			load_team_mgo();
			//call    setup_our_team_data_things
			store_team_frame_data();
			break;
		case 0x76:
			scene->produce_one_screen();
			break;
		case 0x78:
			flag_battling=0;
			Load_Data();
			break;
		case 0x7a:
			role_speed=4;
			goto __walk_role;
		case 0x7b:
			role_speed=8;
			goto __walk_role;
		case 0x7c:
			if(!flag_parallel_mutex){
				--id;
				break;
			}
			npc_speed=4;
			goto __walk_npc;
		case 0x7f:
			GameLoop_OneCycle(false);
			redraw_everything();
			break;
		case 0x80://todo:
			GameLoop_OneCycle(false);
			redraw_everything();
			mutex_can_change_palette=false;
			break;
		case 0x82:
			npc_speed=8;
			goto __walk_npc;
		case 0x8e:
			blit(backup,screen,0,0,0,0,SCREEN_W,SCREEN_H);
			break;
		case 0x92:
			//clear_effective(1,0x41);
			break;
		case 0x93:
			break;
		case 0x98:
			load_team_mgo();
			store_team_frame_data();
			break;
		case 0x9b:
			scene->produce_one_screen();
			break;
		case 0x9d:
			//clear_effective(2,0x4E);
			//clear_effective(1,0x2A);
			break;
		case 0x9e:
			//clear_effective(1,0x4E);
			//clear_effective(1,0x2A);
			break;
		case 0x9f:
			//clear_effective(1,0x48);
			break;
	}
	return id;
}

uint16_t process_script(uint16_t id,int16_t object)
{
	static cut_msg_impl msges;
	static cut_msg_impl objs("word.dat");
	static int _t_=atexit(destroyit);
	if(!backup)
		backup=create_bitmap(SCREEN_W,SCREEN_H);
	static char *msg,colon[3];static int i=sprintf(colon,msges(0xc94,0xc96));
	EVENT_OBJECT &obj=game->evtobjs[object];
	uint16_t next_id=id;	
	current_dialog_lines = 0;
	glbvar_fontcolor  = 0x4F;
	font_color_yellow = 0x2D;
	font_color_red    = 0x1A;
	font_color_cyan   = 0x8D;
	font_color_cyan_1 = 0x8C;
	frame_pos_flag = 1;
	dialog_x = 12;
	dialog_y = 8;
	frame_text_x = 0x2C;
	frame_text_y = 0x1A;
	while(id)
	{
		SCRIPT &curr=game->scripts[id];
		const int16_t &param1=curr.param[0],&param2=curr.param[1],&param3=curr.param[2];
		//printf("独占脚本%04x:%04x %04x %04x %04x ;",id,curr.func,(uint16_t)param1,(uint16_t)curr.param[1],(uint16_t)curr.param[2]);
		switch(curr.func)
		{
			case 0:
				id = next_id;
				//printf("停止执行\n");
				goto exit;
			case -1:
				//printf("显示对话 `%s`\n",cut_msg(game->rpg.msgs[param1],game->rpg.msgs[param1+1]));
				if(current_dialog_lines>3)
				{	show_wait_icon();current_dialog_lines=0;blit(backup,screen,0,0,0,0,SCREEN_W,SCREEN_H);}
				else if(current_dialog_lines==0)
					blit(screen,backup,0,0,0,0,SCREEN_W,SCREEN_H);
				msg=msges(game->msg_idxes[param1],game->msg_idxes[param1+1]);
				if(current_dialog_lines==0 && memcmp(msg+strlen(msg)-2,&colon,2)==0)
					dialog_firstline(msg);
				else
					dialog_string(msg,current_dialog_lines),
					current_dialog_lines++;
				break;
			case 1:
				//printf("停止执行，将调用地址替换为下一条命令\n");
				id++;
				goto exit;
			case 2:
				//printf("停止执行，将调用地址替换为脚本%x:",param1);
				if(curr.param[1]==0){
					//printf("成功\n");
					id = param1;
					goto exit;
				}else if(obj.scr_jmp_count++<curr.param[1]){
					//printf("第%x次成功\n",obj.scr_jmp_count);
					id = param1;
					goto exit;
				}else{
					//printf("过期失效\n");
					obj.scr_jmp_count = 0;
				}
				break;
			case 3:
				//printf("跳转到脚本%x:",param1);
				if(curr.param[1]==0){
					//printf("成功\n");
					id = param1;
					continue;
				}else if(obj.scr_jmp_count++<curr.param[1]){
					//printf("第%x次成功\n",obj.scr_jmp_count);
					id = param1;
					continue;
				}else{
					//printf("过期失效\n",param1);
					obj.scr_jmp_count = 0;
				}
				break;
			case 4:
				//printf("调用脚本%x %x\n",param1,curr.param[1]);
				process_script(param1,curr.param[1]?curr.param[1]:object);
				break;
			case 5:
				//printf("清屏 方式%x 延迟%x,更新角色信息:%s\n",param1,curr.param[1],curr.param[2]?"是":"否");
				if(current_dialog_lines>0)
					show_wait_icon(),current_dialog_lines=0;
				if(param3)
					stop_and_update_frame();
				redraw_everything();
				//blit(backup,screen,0,0,0,0,SCREEN_W,SCREEN_H);
				break;
			case 6:
				//时间关系，不再模拟QB7的随机函数				
				//printf("以%d%%几率跳转到脚本%x:",param1,curr.param[1]);
				if(rnd0()*100<param1){
					//printf("成功\n");
					id = curr.param[1];
					continue;
				}else
					//printf("失败\n");
				break;
			case 7:
				//printf("开战 第%x组敌人 胜利脚本%x 逃跑脚本%x\n",param1,curr.param[1],curr.param[2]);
				if(current_dialog_lines>0)
					show_wait_icon();
				i=process_Battle(param1,param3);
				if( param2 && i == 1){
					id = param2;
					continue;
				}if( param3 && i == 2){
					id = param3;
					continue;
				}
				break;
			case 8:
				next_id = id+1;
				//printf("将调用地址替换为脚本%x\n",next_id);
				break;
			case 9:
				//printf("空闲%x循环\n",param1);
				if(current_dialog_lines>0)
					show_wait_icon(),current_dialog_lines=0;
				for(int cycle=1;cycle<=(param1?param1:1);++cycle){
					//printf("第%x循环:\n",cycle);
					GameLoop_OneCycle(param2!=0);
					redraw_everything();
				}
				break;
			case 0xA:
				//printf("选择:选否则继续(y/n)");
				current_dialog_lines=0;
				char sele;
				scanf("%c",&sele);
				if(sele=='y'){
					id = param1;
					//printf("跳转\n");
					continue;
				}
				//printf("继续\n");
				break;
			default:
				if(current_dialog_lines>0)
					show_wait_icon();
				id = process_script_entry(curr.func,curr.param,id,object);
		}
		++id;
	}
exit:
	if(current_dialog_lines>0)
		show_wait_icon();
	return id;
}
uint16_t process_autoscript(uint16_t id,int16_t object)
{
	if(id==0)
		return 0;
	SCRIPT &curr=game->scripts[id];
	EVENT_OBJECT &obj=game->evtobjs[object];
	const int16_t &param1=curr.param[0],&param2=curr.param[1],&param3=curr.param[2];
	//printf("触发场景:%s,触发对象:(%04x,%s)\n",scr_desc.getdesc("SceneID",scene_curr).c_str(),object,scr_desc.getdesc("ObjectID",object).c_str());
	//printf("自动脚本%04x:%04x %04x %04x %04x ;",id,curr.func,(uint16_t)param1,(uint16_t)param2,(uint16_t)param3);
	switch(curr.func){
		case 0:
			//printf("停止执行\n");
			return id;
		case 2:
			//printf("停止执行，将调用地址替换为脚本%x:",param1);
			if(param2==0){
				//printf("成功\n");
				return id = param1;
			}else if(obj.scr_jmp_count_auto++<param2){
				//printf("第%x次成功\n",obj.scr_jmp_count_auto);
				return id = param1;
			}else{
				//printf("失败\n");
				obj.scr_jmp_count_auto = 0;
			}
			break;
		case 3:
			//printf("跳转到脚本%x",param1);
			if(param2==0){
				//printf("成功\n");
				id = param1;
				process_autoscript(id,object);
			}else if(obj.scr_jmp_count_auto++<param2){
				//printf("第%x次成功\n",obj.scr_jmp_count_auto);
				id = param1;
				process_autoscript(id,object);
			}else{
				//printf("失败\n");
				obj.scr_jmp_count_auto = 0;
			}
			break;
		case 4:
			//printf("调用脚本%x %x\n",param1,param2);
			process_script(param1,param2?param2:object);
			break;
		case 6:
			//时间关系，不再模拟QB7的随机函数				
			//printf("以%d%%几率跳转到脚本%x:",param1,param2);
			if(rnd0()*100<param1){
				//printf("成功\n");
				id = param2;
				process_autoscript(id,object);
			}else
				//printf("失败\n");
			break;
		case 9:
			//printf("自动脚本空闲第%x循环:\n",++obj.scr_jmp_count_auto);
			if(obj.scr_jmp_count_auto++<param1)
				return id;
			else
				obj.scr_jmp_count_auto = 0;
			break;
		default:
			id = process_script_entry(curr.func,curr.param,id,object);
	}
	return id+1;
}