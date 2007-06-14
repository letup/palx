#include "allegdef.h"

bool get_key()
{
	bool got=false;
	keygot=VK_NONE;
	if(keypressed()){
		switch(int k=(readkey()>>8)){
		case KEY_ESC:
		case KEY_INSERT:
		case KEY_ALT:
		case KEY_ALTGR:
			keygot = VK_MENU;
			break;
		case KEY_ENTER:
		case KEY_SPACE:
		case KEY_LCONTROL:
		case KEY_RCONTROL:
			keygot = VK_EXPLORE;
			break;
		case KEY_LEFT:
			keygot = VK_LEFT;
			break;
		case KEY_UP:
			keygot = VK_UP;
			break;
		case KEY_RIGHT:
			keygot = VK_RIGHT;
			break;
		case KEY_DOWN:
			keygot = VK_DOWN;
			break;
		case KEY_PGUP:
			keygot = VK_PGUP;
			break;
		case KEY_PGDN:
			keygot = VK_PGDN;
			break;
		case KEY_R:
			keygot = VK_REPEAT;
			break;
		case KEY_A:
			keygot = VK_AUTO;
			break;
		case KEY_D:
			keygot = VK_DEFEND;
			break;
		case KEY_E:
			keygot = VK_USE;
			break;
		case KEY_W:
			keygot = VK_THROW;
			break;
		case KEY_Q:
			keygot = VK_QUIT;
			break;
		case KEY_S:
			keygot = VK_STATUS;
			break;
		case KEY_F:
			keygot = VK_FORCE;
			break;
		default:
			keygot = (VKEY)k;
		}
		clear_keybuf();
		got=true;
	}
	return got;
}