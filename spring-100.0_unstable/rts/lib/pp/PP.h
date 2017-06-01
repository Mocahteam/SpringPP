/*
    This file is part of Prog&Play: A library to enable interaction between a
    strategy game and program of a player.
    Copyright 2009 Mathieu Muratet

    Prog&Play is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Prog&Play is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Mathieu Muratet
    muratet@irit.fr
*/

#ifndef PP_H
#define PP_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* Data definitions                                                          */
/*****************************************************************************/

#define NB_COALITIONS 3

/* Define Coalition type. A coalition is a set of units. */
typedef enum {
  MY_COALITION,
  ALLY_COALITION,
  ENEMY_COALITION
} PP_Coalition;

typedef int PP_Unit;

typedef int PP_Resource;

typedef struct {
  float x;
  float y;
} PP_Pos;

#ifdef __cplusplus
}
#endif

#endif /* PP_H */
