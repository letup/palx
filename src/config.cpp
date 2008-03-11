/***************************************************************************
 *   PALx: A platform independent port of classic RPG PAL              *
 *   Copyleft (C) 2006 by Pal Lockheart                        *
 *   palxex@gmail.com                                      *
 *                                                 *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                           *
 *                                                 *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                  *
 *                                                 *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, If not, see                  *
 *   <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/
#include <boost/shared_array.hpp>

#include <cstdio>

#include "resource.h"
#include "allegdef.h"
#include "internal.h"
//#include "UI.h"
#include "pallib.h"
#include "config.h"

#if defined (WIN32)
#   define FONT_PATH "%WINDIR%/fonts/mingliu.ttc"
#   define LOCALE "chinese"
#   define CONF "palx.conf"
#elif defined __MSDOS__
#   define FONT_PATH "mingliu.ttc"
#   define LOCALE "BIG5"
#   define CONF "palx.cfg"
#elif defined __APPLE__
#   define FONT_PATH "/System/Library/Fonts/\xE5\x84\xB7\xE9\xBB\x91 Pro.ttf"
#   define LOCALE "BIG5"
#   define CONF "~/.palxrc"
#else   //predicate *NIX
#   define FONT_PATH "/usr/share/fonts/truetype/arphic/uming.ttf" //ubuntu gutsy gibbon;other distribution has other position but I don't know the unified method to determine it.
#   define LOCALE "BIG5"
#   define CONF "~/.palxrc"
#endif

using namespace std;
using namespace boost;
using namespace Pal::Tools;

string env_expand(string name)
{
    if(name[0]=='~')
        name=getenv("HOME")+name.substr(1,name.size()-1);
    string::iterator begin,end;
	while((begin=find(name.begin(),name.end(),'%'))!=name.end() && (end=find(begin+1,name.end(),'%'))!=name.end())
		name.replace(begin,end+1,getenv(name.substr(begin-name.begin()+1,end-begin-1).c_str()));

    return name;
}
ini_parser::ini_parser(const char *conf,bool once):name(conf),needwrite(false)
{
	ini_parser::section::configmap configprop;
	configprop["path"].value=".";
	configprop["path"].comment="��Դ·��";
	configprop["setup"].value="true";
	configprop["setup"].comment="Bool;�Ƿ���setup.dat������ø�������Ķ�Ӧ����";
	configprop["allow_memory"].value="false";
	configprop["allow_memory"].comment="Bool;�Ƿ���������̼������浵";
	configprop["last"].value="";
	configprop["last"].comment="Int;�������Ĵ浵";
	configprop["resource"].value="dos";
	configprop["resource"].comment="dos/win95/ss(?)";
	configprop["encode"].value=LOCALE;
	configprop["encode"].comment="win32:chs/cht;linux/mac/dos/...(iconv):GBK/BIG5";
	configprop["switch_off"].value="true";
	configprop["switch_off"].comment="�����л�������ʱ�����Ƿ����ִ��";
	section config("config",configprop);
	sections["config"]=config;

	ini_parser::section::configmap debugprop;
	debugprop["resource"].value="mkf";
	debugprop["resource"].comment="��Դʹ�÷�ʽ;mkf/filesystem";
	debugprop["allow_frozen"].value="true";
	debugprop["allow_frozen"].comment="������������/ֹͣ;true/false";
	section debug("debug",debugprop);
	sections["debug"]=debug;

	ini_parser::section::configmap displayprop;
	displayprop["height"].value="200";
	displayprop["height"].comment="����;320x200��������.";
	displayprop["width"].value="320";
	displayprop["scale"].value="2";
	displayprop["scale"].comment="1,2;etc.";
	displayprop["fullscreen"].value="false";
	displayprop["fullscreen"].comment="Bool ;ȫ��";
	section display("display",displayprop);
	sections["display"]=display;

	ini_parser::section::configmap fontprop;
	fontprop["type"].value="truetype";
	fontprop["type"].comment="truetype: ttf/ttc; fon: wor16.fon";
	fontprop["path"].value=FONT_PATH;
	section font("font",fontprop);
	sections["font"]=font;

	ini_parser::section::configmap musicprop;
	musicprop["type"].value="rix";
	musicprop["type"].comment="rix/mid/foreverCD,+gameCD,+gameCDmp3���������ϡ�foreverCDָ�������¼֮FM����";
	musicprop["opltype"].value="mame";
	musicprop["opltype"].comment="real/mame/ken;���OPLоƬ/MAME��ģ��/Ken��ģ��FM����";
	musicprop["oplport"].value="0x388";
	musicprop["oplport"].comment="���OPL2�˿ں�;0x388Ϊadlib����Ĭ��.ֻ��dos build(��δ����)��Ч";
	musicprop["volume"].value="255";
	musicprop["volume"].comment="0-255;����";
	musicprop["volume_sfx"].value="255";
	musicprop["volume_sfx"].comment="0-255;��Ч����";
	section music("music",musicprop,"��setup����");
	sections["music"]=music;

	ini_parser::section::configmap keyprop;
	keyprop["west"].value="0x4b";
	keyprop["north"].value="0x48";
	keyprop["east"].value="0x4d";
	keyprop["south"].value="0x50";
	section keymap("keymap",keyprop,"���߼��̶��壻��setup����");
	sections["keymap"]=keymap;

	ifstream ifs((env_expand(name)).c_str());
	if(ifs.is_open() )
		while(!ifs.eof()){
			section s;
			ifs>>s;
			sections[s.name()]=s;
		}
    else
        needwrite=true;

    if(once){
        write();
        running=false;
    }
}

void ini_parser::write(){
	ofstream ofs((env_expand(name)).c_str(),ios_base::out|ios_base::trunc);
	for(std::map<string,section>::const_iterator i=sections.begin();i!=sections.end();i++)
		ofs<<(*i).second;
	ofs.flush();
}

ini_parser::~ini_parser()
{
    if(getSection("config").get("allow_memory",bool()) && getSection("config").get("last",int())!=rpg_to_load)
        getSection("config").keymap["last"].value=lexical_cast<string>(rpg_to_load),needwrite=true;
	if(needwrite)
		write();
}

namespace{
	uint8_t *denone_file(const char *file,long &len)
	{
		int32_t length;
		FILE *fp=fopen(file,"rb");
		fseek(fp,0,SEEK_END);
		length=ftell(fp);
		rewind(fp);
		uint8_t *buf=new uint8_t[length];
		fread(buf,length,1,fp);
		fclose(fp);
		len=length;
		return buf;
	}
	uint8_t *demkf_ptr(const char *file,int n,long &len)
	{
		int32_t offset=n*4,length;
		FILE *fp=fopen(file,"rb");
		fseek(fp,offset,SEEK_SET);
		fread(&offset,4,1,fp);
		fread(&length,4,1,fp);length-=offset;
		fseek(fp,offset,SEEK_SET);
		uint8_t *buf=new uint8_t[length];
		fread(buf,length,1,fp);
		fclose(fp);
		len=length;
		return buf;
	}
	shared_array<uint8_t> demkf_sp(shared_array<uint8_t> src,int n,long &len,int &files)
	{
		uint32_t *usrc=(uint32_t *)src.get();
		files=usrc[0]/4-2;
		int32_t length=usrc[n+1]-usrc[n];
		uint8_t *buf=new uint8_t[length];
		memcpy(buf,src.get()+usrc[n],length);
		len=length;
		return shared_array<uint8_t>(buf);
	}
	shared_array<uint8_t> demkf_file(const char *file,int n,long &len,shared_array<uint8_t> &buf,long &size,bool cached)
	{
		if(cached){
			len=size;
			return buf;
		}else{
			buf = shared_array<uint8_t>(demkf_ptr(file,n,len));
			size=len;
			return buf;
		}
	}
	uint8_t *desmkf_ptr(shared_array<uint8_t> src,int n,long &len,int &files)
	{
		uint16_t *usrc=(uint16_t *)src.get();
		files=(usrc[0]-((usrc[usrc[0]-1]<=0 || usrc[usrc[0]-1]*2>=len || usrc[usrc[0]-1]<usrc[usrc[0]-2])?1:0));
		int16_t length;
		if(n == files - 1)
			length=len-usrc[n]*2;
		else
			length=(usrc[n+1]-usrc[n])*2;
		uint8_t *buf=new uint8_t[length];
		memcpy(buf,src.get()+usrc[n]*2,length);
		len=length;
		return buf;
	}
	uint8_t *deyj1_ptr(shared_array<uint8_t> src,long &len)
	{
		void *dst;
		uint32_t length;
		DecodeYJ1(src.get(),dst,(uint32&)length);
		len=length;
		return (uint8_t*)dst;
	}
	shared_array<uint8_t> deyj1_sp(shared_array<uint8_t> src,long &len)
	{
		return shared_array<uint8_t>(deyj1_ptr(src,len));
	}
	uint8_t *deyj2_ptr(shared_array<uint8_t> src,long &len)
	{
		void *dst;
		uint32_t length;
		DecodeYJ2(src.get(),dst,(uint32&)length);
		len=length;
		return (uint8_t*)dst;
	}
	shared_array<uint8_t> deyj2_sp(shared_array<uint8_t> src,long &len)
	{
		return shared_array<uint8_t>(deyj2_ptr(src,len));
	}
	uint8_t *defile_dir(const char *dir,int file,long &len,int &files)
	{
		char *buf=new char[80];
		for(int i=0;i<999999 && file == 0;i++)
		{
			char buf2[80];
			sprintf(buf2,"%s/%d",dir,i);
			FILE *fp=fopen(buf2,"rb");
			if(!fp){
				files=i;
				break;
			}
			fclose(fp);
		}
		sprintf(buf,"%s/%d",dir,file);
		return denone_file(shared_array<char>(buf).get(),len);
	}
	uint8_t *defile_sp(shared_array<char> dir,int file,long &len,int &files)
	{
		return defile_dir(dir.get(),file,len,files);
	}
	shared_array<char> dedir_dir(const char *dir,int dir2)
	{
		char *buf=new char[80];
		sprintf(buf,"%s/%d",dir,dir2);
		return shared_array<char>(buf);
	}
}

int CARD=0;
bool is_out;

namespace{
    void close_button_handler(void)
    {
        //if(!yes_or_no(0x13,0)) return;
        running=false;
        remove_timer();
    }
    END_OF_FUNCTION(close_button_handler)
    void switchin_proc()
    {
        is_out=false;
    }
    void switchout_proc()
    {
        is_out=true;
    }
}

ini_parser getconf(int c,char *v[])
{
    string name=CONF;
    bool write_directly=false;
    if(c>1 && !strcmp(v[1],"--conf"))
        name=v[2];
    if(c>1 && (!strcmp(v[1],"--write") || !strcmp(v[1],"w")))
        write_directly=true;
    return ini_parser(name.c_str(),write_directly);
}
global_init::global_init(int c,char *v[]):conf(getconf(c,v))
{
	//version dispatch
	boost::function<uint8_t *(shared_array<uint8_t> ,long &)> extract_ptr;
	boost::function<shared_array<uint8_t>(shared_array<uint8_t> ,long &)> extract_sp;
	if(get<string>("config","resource")=="dos")
		extract_ptr=deyj1_ptr,extract_sp=deyj1_sp,sfx_file="VOC.MKF";
	else if(get<string>("config","resource")=="win95")
		extract_ptr=deyj2_ptr,extract_sp=deyj2_sp,sfx_file="SOUNDS.MKF";

	if(get<string>("config","encode")=="")
	  if(get<string>("config","resource")=="dos")
	    set<string>("config","encode",LOCALE);
	  else if(get<string>("config","resource")=="win95")
	    set<string>("config","encode","GBK");

	if(get<string>("debug","resource")=="mkf")
	{
		de_none			=bind(denone_file,_1,_4);
		de_mkf			=bind(demkf_ptr,_1,_2,_4);
		de_mkf_yj1		=bind(extract_ptr,	bind(demkf_file,		_1,_2,_4,_6,_7,_8),_4);
		de_mkf_mkf_yj1	=bind(extract_ptr,	bind(demkf_sp,bind(demkf_file,_1,_2,_4,_6,_7,_8),_3,_4,_5),_4);
		de_mkf_smkf	=bind(desmkf_ptr,	bind(demkf_file,		_1,_2,_4,_6,_7,_8),_3,_4,_5);
		de_mkf_yj1_smkf=bind(desmkf_ptr,	bind(extract_sp,		bind(demkf_file,_1,_2,_4,_6,_7,_8),_4),_3,_4,_5);
	}else if(get<string>("debug","resource")=="filesystem"){
		de_none			=bind(denone_file,_1,_4);
		de_mkf			=bind(defile_dir,_1,_2,_4,_5);
		de_mkf_yj1		=de_mkf;
		de_mkf_mkf_yj1	=bind(defile_sp,	bind(dedir_dir,_1,_2),_3,_4,_5);
		de_mkf_smkf	=de_mkf_mkf_yj1;
		de_mkf_yj1_smkf=de_mkf_mkf_yj1;
	}
}
void global_init::display_setup(bool ext)
{
	static PALETTE pal;get_palette(pal);
	CARD=(get<bool>("display","fullscreen")?GFX_AUTODETECT:GFX_AUTODETECT_WINDOWED);
	if(!ext){
		if(CARD==GFX_AUTODETECT)
			CARD=GFX_AUTODETECT_WINDOWED;
		else if(CARD==GFX_AUTODETECT_WINDOWED)
			CARD=GFX_AUTODETECT;
	}
	if(get<int>("display","scale")<1)
		set<int>("display","scale",1);
	if(set_gfx_mode(CARD,SCREEN_W*get<int>("display","scale"),SCREEN_H*get<int>("display","scale"),0,0)<0)
        if(set_gfx_mode(GFX_SAFE,SCREEN_W,SCREEN_H,0,0)<0)
            running=false;
	set_palette(pal);
	if(get<bool>("config","switch_off"))
        set_display_switch_mode(SWITCH_BACKGROUND);
	set_display_switch_callback(SWITCH_IN,switchin_proc);
	set_display_switch_callback(SWITCH_OUT,switchout_proc);
	set_color_depth(8);
	set<bool>("display","fullscreen",CARD==GFX_AUTODETECT);
}
int global_init::operator ()()
{
	string path_root=get<string>("config","path");
	SETUP.set(path_root+"/SETUP.DAT",de_none);
	PAT.set	(path_root+"/PAT.MKF"	,de_mkf);
	//MIDI.set(path_root+"/MIDI.MKF"	,de_mkf);
	SFX.set	(path_root+"/"+sfx_file	,de_mkf);
	DATA.set(path_root+"/DATA.MKF"	,de_mkf);
	SSS.set	(path_root+"/SSS.MKF"	,de_mkf);
	FBP.set	(path_root+"/FBP.MKF"	,de_mkf_yj1);
	MAP.set	(path_root+"/MAP.MKF"	,de_mkf_yj1);
	GOP.set	(path_root+"/GOP.MKF"	,de_mkf_smkf);
	BALL.set(path_root+"/BALL.MKF"	,de_mkf_smkf);
	RGM.set	(path_root+"/RGM.MKF"	,de_mkf_smkf);
	ABC.set	(path_root+"/ABC.MKF"	,de_mkf_yj1_smkf);
	F.set	(path_root+"/F.MKF"	,de_mkf_yj1_smkf);
	FIRE.set(path_root+"/FIRE.MKF"	,de_mkf_yj1_smkf);
	MGO.set	(path_root+"/MGO.MKF"	,de_mkf_yj1_smkf);
	RNG.set	(path_root+"/RNG.MKF"	,de_mkf_mkf_yj1);

	msges.set(path_root+"/M.MSG");
	objs.set(path_root+"/WORD.DAT");
	playrix::set((path_root+"/MUS.MKF").c_str());


	//allegro init
	allegro_init();
	display_setup();
	install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
	install_timer();
	install_keyboard();keyboard_lowlevel_callback = key_watcher;
	//install_mouse();enable_hardware_cursor();select_mouse_cursor(MOUSE_CURSOR_ARROW);show_mouse(screen);

	LOCK_FUNCTION(close_button_handler);
	set_close_button_callback(close_button_handler);

	if(get<string>("font","type")=="truetype")
		Font=boost::shared_ptr<def_font>(new ttfont());
	else if(get<string>("font","type")=="fon")
		Font=boost::shared_ptr<def_font>(new pixfont());

	randomize();

	int save=0;
	if(get<bool>("config","allow_memory"))
		save=get<int>("config","last");

	return save;
}
global_init *global;