#include "internal.h"
#include "structs.h"
#include "game.h"
#include "scene.h"

#include "stdlib.h"

extern int scale;

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
				if(abs(x_diff)<npc_speed*2 && abs(y_diff)<npc_speed){
					obj.pos_x = param1*32+param3*16;
					obj.pos_y = param2*16+param3*8;
				}else{
					obj.pos_x += npc_speed*(x_diff<0 ? 2 : -2);
					obj.pos_y += npc_speed*(y_diff<0 ? 1 : -1);
				}

				//afterward check;MUST have,or will not match dospal exactly
				/*if(obj.pos_x==param1*32+param3*16 && obj.pos_y==param2*16+param3*8)
					//printf(addition,"���");
				else{
					//printf(addition,"��ǰX:%x,Y:%x  Ŀ��X:%x,Y:%x",obj.pos_x,obj.pos_y,(param1*32+param3*16),(param2*16+param3*8));
					--id;
				}*/
			}
			break;
		case 0x11:
			if((object&1) != flag_parallel_mutex){
				//printf(addition,"������ ��ǰX:%x,Y:%x  Ŀ��X:%x,Y:%x",obj.pos_x,obj.pos_y,(param1*32+param3*16),(param2*16+param3*8));
				--id;
				break;
			}
			npc_speed=2;
			goto __walk_npc;
		case 0x15:
			game->rpg.team_direction=param1;
			game->rpg.team[param3].frame=param1*3+param2;
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
		case 0x59:
			scene->toload=param1;
			flag_to_load|=0xC;
			break;
		case 0x65:
			game->rpg.roles_properties.avator[param1]=param2;
			if(!flag_battling && param3)
				load_team_mgo();
			break;
		case 0x6c:			
			if(param1){
				(param1>0 ? game->evtobjs[param1] : obj).pos_x += param2;
				(param1>0 ? game->evtobjs[param1] : obj).pos_y += param3;
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
				//printf(addition,"�ɹ�");
			}//else
				//printf(addition,"ʧ��");
			break;
		case 0x70:
			role_speed=2;
__walk_role:
			{
				int16_t x_diff,y_diff;
				while((x_diff=scene->team_pos.x-(param1*32+param3*16)) && (y_diff=scene->team_pos.y-(param2*16+param3*8))){
					scene->team_pos.x += role_speed*(x_diff<0 ? 2 : -2);
					scene->team_pos.y += role_speed*(y_diff<0 ? 1 : -1);
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
			//call    store_team_frame_data
			break;
		case 0x76:
			scene->produce_one_screen();
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
		case 0x80:
			GameLoop_OneCycle(false);
			redraw_everything();
			break;
		case 0x82:
			npc_speed=8;
			goto __walk_npc;
		case 0x8e:
			blit(backup,screen,0,0,0,0,screen->w,screen->h);
			break;
		case 0x92:
			//clear_effective(1,0x41);
			break;
		case 0x93:
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
		backup=create_bitmap(screen->w,screen->h);
	static char *msg,colon[3];static int i=sprintf(colon,"\xA1\x47");//"\xA3\xBA");
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
		//printf("��ռ�ű�%04x:%04x %04x %04x %04x ;",id,curr.func,(uint16_t)param1,(uint16_t)curr.param[1],(uint16_t)curr.param[2]);
		switch(curr.func)
		{
			case 0:
				id = next_id;
				//printf("ִֹͣ��\n");
				return id;
			case -1:
				//printf("��ʾ�Ի� `%s`\n",cut_msg(game->rpg.msgs[param1],game->rpg.msgs[param1+1]));
				if(current_dialog_lines>3)
				{	show_wait_icon();current_dialog_lines=0;blit(backup,screen,0,0,0,0,320,200);}
				else if(current_dialog_lines==0)
					blit(screen,backup,0,0,0,0,screen->w,screen->h);
				msg=msges(game->msg_idxes[param1],game->msg_idxes[param1+1]);
				if(current_dialog_lines==0 && memcmp(msg+strlen(msg)-2,&colon,2)==0)
					dialog_firstline(msg);
				else
					dialog_string(msg,current_dialog_lines),
					current_dialog_lines++;
				break;
			case 1:
				//printf("ִֹͣ�У������õ�ַ�滻Ϊ��һ������\n");
				return id+1;
			case 2:
				//printf("ִֹͣ�У������õ�ַ�滻Ϊ�ű�%x:",param1);
				if(curr.param[1]==0){
					//printf("�ɹ�\n");
					return param1;
				}else if(obj.scr_jmp_count++<curr.param[1]){
					//printf("��%x�γɹ�\n",obj.scr_jmp_count);
					return param1;
				}else{
					//printf("����ʧЧ\n");
					obj.scr_jmp_count = 0;
				}
				break;
			case 3:
				//printf("��ת���ű�%x:",param1);
				if(curr.param[1]==0){
					//printf("�ɹ�\n");
					id = param1;
					continue;
				}else if(obj.scr_jmp_count++<curr.param[1]){
					//printf("��%x�γɹ�\n",obj.scr_jmp_count);
					id = param1;
					continue;
				}else{
					//printf("����ʧЧ\n",param1);
					obj.scr_jmp_count = 0;
				}
				break;
			case 4:
				//printf("���ýű�%x %x\n",param1,curr.param[1]);
				process_script(param1,curr.param[1]?curr.param[1]:object);
				break;
			case 5:
				//printf("���� ��ʽ%x �ӳ�%x,���½�ɫ��Ϣ:%s\n",param1,curr.param[1],curr.param[2]?"��":"��");
				if(current_dialog_lines>0)
					show_wait_icon(),current_dialog_lines=0;
				if(param3)
					stop_and_update_frame();
				redraw_everything();
				//blit(backup,screen,0,0,0,0,screen->w,screen->h);
				break;
			case 6:
				//ʱ���ϵ������ģ��QB7���������				
				//printf("��%d%%������ת���ű�%x:",param1,curr.param[1]);
				if(rnd0()*100<param1){
					//printf("�ɹ�\n");
					id = curr.param[1];
					continue;
				}else
					//printf("ʧ��\n");
				break;
			case 7:
				//printf("��ս ��%x����� ʤ���ű�%x ���ܽű�%x\n",param1,curr.param[1],curr.param[2]);
				if(current_dialog_lines>0)
					show_wait_icon(),current_dialog_lines=0;
				break;
			case 8:
				next_id = id+1;
				//printf("�����õ�ַ�滻Ϊ�ű�%x\n",next_id);
				break;
			case 9:
				//printf("����%xѭ��\n",param1);
				if(current_dialog_lines>0)
					show_wait_icon(),current_dialog_lines=0;
				for(int cycle=1;cycle<=(param1?param1:1);++cycle){
					//printf("��%xѭ��:\n",cycle);
					GameLoop_OneCycle(curr.param[1]!=0);
					redraw_everything();
				}
				break;
			case 0xA:
				//printf("ѡ��:ѡ�������(y/n)");
				current_dialog_lines=0;
				char sele;
				scanf("%c",&sele);
				if(sele=='y'){
					id = param1;
					//printf("��ת\n");
					continue;
				}
				//printf("����\n");
				break;
			default:
				if(current_dialog_lines>0)
					show_wait_icon();
				id = process_script_entry(curr.func,curr.param,id,object);
		}
		++id;
	}
	return id;
}
uint16_t process_autoscript(uint16_t id,int16_t object)
{
	if(id==0)
		return 0;
	SCRIPT &curr=game->scripts[id];
	EVENT_OBJECT &obj=game->evtobjs[object];
	const int16_t &param1=curr.param[0],&param2=curr.param[1],&param3=curr.param[2];
	//printf("��������:%s,��������:(%04x,%s)\n",scr_desc.getdesc("SceneID",scene_curr).c_str(),object,scr_desc.getdesc("ObjectID",object).c_str());
	//printf("�Զ��ű�%04x:%04x %04x %04x %04x ;",id,curr.func,(uint16_t)param1,(uint16_t)param2,(uint16_t)param3);
	switch(curr.func){
		case 0:
			//printf("ִֹͣ��\n");
			return id;
		case 2:
			//printf("ִֹͣ�У������õ�ַ�滻Ϊ�ű�%x:",param1);
			if(param2==0){
				//printf("�ɹ�\n");
				return id = param1;
			}else if(obj.scr_jmp_count_auto++<param2){
				//printf("��%x�γɹ�\n",obj.scr_jmp_count_auto);
				return id = param1;
			}else{
				//printf("ʧ��\n");
				obj.scr_jmp_count_auto = 0;
			}
			break;
		case 3:
			//printf("��ת���ű�%x",param1);
			if(param2==0){
				//printf("�ɹ�\n");
				id = param1;
				process_autoscript(id,object);
			}else if(obj.scr_jmp_count_auto++<param2){
				//printf("��%x�γɹ�\n",obj.scr_jmp_count_auto);
				id = param1;
				process_autoscript(id,object);
			}else{
				//printf("ʧ��\n");
				obj.scr_jmp_count_auto = 0;
			}
			break;
		case 4:
			//printf("���ýű�%x %x\n",param1,param2);
			process_script(param1,param2?param2:object);
			break;
		case 6:
			//ʱ���ϵ������ģ��QB7���������				
			//printf("��%d%%������ת���ű�%x:",param1,param2);
			if(rnd0()*100<param1){
				//printf("�ɹ�\n");
				id = param2;
				process_autoscript(id,object);
			}else
				//printf("ʧ��\n");
			break;
		case 9:
			//printf("�Զ��ű����е�%xѭ��:\n",++obj.scr_jmp_count_auto);
			if(obj.scr_jmp_count_auto<param1)
				return id;
			else
				obj.scr_jmp_count_auto = 0;
			break;
		default:
			id = process_script_entry(curr.func,curr.param,id,object);
	}
	return id+1;
}