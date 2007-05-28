/***************************************************************************
 *   Copyright (C) 2006 by Pal Lockheart   *
 *   palxex@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "Game.h"
#include "begin.h"
#include "pallib.h"

#include <iostream>
#include <cstdlib>
using namespace std;

#include "internal.h"
#include "game.h"
#include "scene.h"
#include "allegdef.h"
int main(int argc, char *argv[])
{
	randomize();
	Game  thegame(argc>=2? atoi(argv[1]) : begin_scene()()); game=&thegame;
	Scene normal;	scene=&normal;
	BattleScene battle;	battle_scene=&battle;
	playrix player;				rix=&player;
	return thegame.run();
}
END_OF_MAIN();