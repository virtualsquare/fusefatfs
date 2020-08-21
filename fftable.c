/**
 * Copyright (c) 2020 Renzo Davoli <renzo@cs.unibo.it>
 *
 * This program  is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ff.h>
#include <fftable.h>

static struct fftab *fftab[FF_VOLUMES];

int fftab_new(const char *path, int flags) {
	int index;
	struct fftab *new;
	size_t pathlen = strlen(path) + 1;
	for (index = 0; index < FF_VOLUMES; index++)
		if (fftab[index] == NULL)
			break;
	if (index >= FF_VOLUMES)
		return -1;
	new = malloc(sizeof(struct fftab) + pathlen);
	if (new == NULL)
		return -1;
	new->fd = -1;
	new->index = index;
	new->flags = flags;
	memset(&new->fs, 0, sizeof(new->fs));
	snprintf(new->path, pathlen, "%s", path);
	fftab[index] = new;
	return index;
}

void fftab_del(int index) {
	if (index < 0) return;
	if (index >= FF_VOLUMES) return;
	if (fftab[index] == NULL) return;
	free(fftab[index]);
	fftab[index] = NULL;
}

struct fftab *fftab_get(int index) {
	if (index < 0) return NULL;
	if (index >= FF_VOLUMES) return NULL;
	return fftab[index];
}
