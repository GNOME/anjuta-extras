/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <glib.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

pid_t process_fork (const char *path, char **argv, gboolean redirect, int ignfd,
		    int *infd, int *outfd, int *errfd, GError **err);

int process_wait (pid_t pid);

int process_kill (pid_t pid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PROCESS_H__ */
