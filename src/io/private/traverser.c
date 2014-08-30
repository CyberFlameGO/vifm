/* vifm
 * Copyright (C) 2014 xaizek.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "traverser.h"

#include <dirent.h> /* DIR dirent opendir() readdir() closedir() */

#include <stddef.h> /* NULL */
#include <stdlib.h> /* free() */

#include "../../utils/fs.h"
#include "../../utils/path.h"
#include "../../utils/str.h"

static int traverse_subtree(const char path[], subtree_visitor visitor,
		void *param);

int
traverse(const char path[], subtree_visitor visitor, void *param)
{
	if(is_dir(path))
	{
		return traverse_subtree(path, visitor, param);
	}
	else
	{
		return visitor(path, VA_FILE, param);
	}
}

/* A generic subtree traversing.  Returns zero on success, otherwise non-zero is
 * returned. */
static int
traverse_subtree(const char path[], subtree_visitor visitor, void *param)
{
	DIR *dir;
	struct dirent *d;
	int result;
	VisitResult enter_result;

	dir = opendir(path);
	if(dir == NULL)
	{
		return 1;
	}

	enter_result = visitor(path, VA_DIR_ENTER, param);
	if(enter_result == VR_ERROR)
	{
		(void)closedir(dir);
		return 1;
	}

	result = 0;
	while((d = readdir(dir)) != NULL)
	{
		if(!is_builtin_dir(d->d_name))
		{
			char *const full_path = format_str("%s/%s", path, d->d_name);
			if(entry_is_dir(full_path, d))
			{
				result = traverse_subtree(full_path, visitor, param);
			}
			else
			{
				result = visitor(full_path, VA_FILE, param);
			}
			free(full_path);

			if(result != 0)
			{
				break;
			}
		}
	}
	(void)closedir(dir);

	if(result == 0 && enter_result != VR_SKIP_DIR_LEAVE)
	{
		result = visitor(path, VA_DIR_LEAVE, param);
	}

	return result;
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 : */
