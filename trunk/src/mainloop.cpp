#include "Game.h"
#include "internal.h"
#include "scene.h"
#include "timing.h"

#include <boost/shared_ptr.hpp>

namespace{
	bool running=true;
	void mainloop_proc()
	{
		if(flag_to_load)
			Load_Data();

		//Parse Key
		VKEY keygot=get_key();
		
		GameLoop_OneCycle(true);
		if(flag_to_load)
			Load_Data();

		//clear scene back
		scene->clear_scanlines();
		scene->clear_active();
		scene->calc_team_walking(keygot);
		scene->our_team_setdraw();
		scene->visible_NPC_movment_setdraw();
		scene->move_usable_screen();
		scene->Redraw_Tiles_or_Fade_to_pic();
		scene->draw_normal_scene(1);
		if(keygot==VK_EXPLORE)
			process_Explore();
		if(keygot==VK_MENU)
			running=process_Menu();

		flag_parallel_mutex=!flag_parallel_mutex;
		
		get_key(false);
	}
	END_OF_FUNCTION(mainloop_proc);
}

int Game::run(){
	//��Ϸ��ѭ��10fps,����100fps,����70fps��
	while(running){
		rest(1);
		if(time_interrupt_occurs>=10)
			mainloop_proc(),time_interrupt_occurs=0;
	}
	return 0;
}