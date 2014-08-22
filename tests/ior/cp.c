#include "seatest.h"

#include <unistd.h> /* F_OK access() */

#include "../../src/io/iop.h"
#include "../../src/io/ior.h"
#include "../../src/utils/fs.h"
#include "utils.h"

static void
test_file_is_copied(void)
{
	{
		io_args_t args =
		{
			.arg1.src = "../read/binary-data",
			.arg2.dst = "binary-data",
		};
		assert_int_equal(0, ior_cp(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "binary-data",
		};
		assert_int_equal(0, iop_rmfile(&args));
	}
}

static void
test_empty_directory_is_copied(void)
{
	create_empty_dir("empty-dir");

	{
		io_args_t args =
		{
			.arg1.src = "empty-dir",
			.arg2.dst = "empty-dir-copy",
		};
		assert_int_equal(0, ior_cp(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "empty-dir",
		};
		assert_int_equal(0, iop_rmdir(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "empty-dir-copy",
		};
		assert_int_equal(0, iop_rmdir(&args));
	}
}

static void
test_non_empty_directory_is_copied(void)
{
	create_non_empty_dir("non-empty-dir", "a-file");

	{
		io_args_t args =
		{
			.arg1.src = "non-empty-dir",
			.arg2.dst = "non-empty-dir-copy",
		};
		assert_int_equal(0, ior_cp(&args));
	}

	assert_int_equal(0, access("non-empty-dir-copy/a-file", F_OK));

	{
		io_args_t args =
		{
			.arg1.path = "non-empty-dir",
		};
		assert_int_equal(0, ior_rm(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "non-empty-dir-copy",
		};
		assert_int_equal(0, ior_rm(&args));
	}
}

static void
test_empty_nested_directory_is_copied(void)
{
	create_empty_nested_dir("non-empty-dir", "empty-nested-dir");

	{
		io_args_t args =
		{
			.arg1.src = "non-empty-dir",
			.arg2.dst = "non-empty-dir-copy",
		};
		assert_int_equal(0, ior_cp(&args));
	}

	assert_int_equal(0,
			access("non-empty-dir-copy/empty-nested-dir", F_OK));

	{
		io_args_t args =
		{
			.arg1.path = "non-empty-dir",
		};
		assert_int_equal(0, ior_rm(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "non-empty-dir-copy",
		};
		assert_int_equal(0, ior_rm(&args));
	}
}

static void
test_non_empty_nested_directory_is_copied(void)
{
	create_non_empty_nested_dir("non-empty-dir", "nested-dir", "a-file");

	{
		io_args_t args =
		{
			.arg1.src = "non-empty-dir",
			.arg2.dst = "non-empty-dir-copy",
		};
		assert_int_equal(0, ior_cp(&args));
	}

	assert_int_equal(0, access("non-empty-dir-copy/nested-dir/a-file", F_OK));

	{
		io_args_t args =
		{
			.arg1.path = "non-empty-dir",
		};
		assert_int_equal(0, ior_rm(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "non-empty-dir-copy",
		};
		assert_int_equal(0, ior_rm(&args));
	}
}

static void
test_fails_to_overwrite_file_by_default(void)
{
	create_empty_file("a-file");

	{
		io_args_t args =
		{
			.arg1.src = "../read/two-lines",
			.arg2.dst = "a-file",
		};
		assert_false(ior_cp(&args) == 0);
	}

	{
		io_args_t args =
		{
			.arg1.path = "a-file",
		};
		assert_int_equal(0, iop_rmfile(&args));
	}
}

static void
test_fails_to_overwrite_dir_by_default(void)
{
	create_empty_dir("empty-dir");

	{
		io_args_t args =
		{
			.arg1.src = "../read",
			.arg2.dst = "empty-dir",
		};
		assert_false(ior_cp(&args) == 0);
	}

	{
		io_args_t args =
		{
			.arg1.path = "empty-dir",
		};
		assert_int_equal(0, iop_rmdir(&args));
	}
}

static void
test_overwrites_file_when_asked(void)
{
	create_empty_file("a-file");

	{
		io_args_t args =
		{
			.arg1.src = "../read/two-lines",
			.arg2.dst = "a-file",
			.arg3.crs = IO_CRS_REPLACE_FILES,
		};
		assert_int_equal(0, ior_cp(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "a-file",
		};
		assert_int_equal(0, iop_rmfile(&args));
	}
}

static void
test_overwrites_dir_when_asked(void)
{
	create_empty_dir("dir");

	{
		io_args_t args =
		{
			.arg1.src = "../read",
			.arg2.dst = "dir",
			.arg3.crs = IO_CRS_REPLACE_ALL,
		};
		assert_int_equal(0, ior_cp(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "dir",
		};
		assert_false(iop_rmdir(&args) == 0);
	}

	{
		io_args_t args =
		{
			.arg1.path = "dir",
		};
		assert_int_equal(0, ior_rm(&args));
	}
}

static void
test_directories_can_be_merged(void)
{
	create_empty_dir("first");

	assert_int_equal(0, chdir("first"));
	create_empty_file("first-file");
	assert_int_equal(0, chdir(".."));

	create_empty_dir("second");

	assert_int_equal(0, chdir("second"));
	create_empty_file("second-file");
	assert_int_equal(0, chdir(".."));

	{
		io_args_t args =
		{
			.arg1.src = "first",
			.arg2.dst = "second",
			.arg3.crs = IO_CRS_REPLACE_FILES,
		};
		assert_int_equal(0, ior_cp(&args));
	}

	assert_int_equal(0, access("second/second-file", F_OK));
	assert_int_equal(0, access("second/first-file", F_OK));

	{
		io_args_t args =
		{
			.arg1.path = "first",
		};
		assert_int_equal(0, ior_rm(&args));
	}

	{
		io_args_t args =
		{
			.arg1.path = "second",
		};
		assert_int_equal(0, ior_rm(&args));
	}
}

static void
test_fails_to_copy_directory_inside_itself(void)
{
	create_empty_dir("empty-dir");

	{
		io_args_t args =
		{
			.arg1.src = "empty-dir",
			.arg2.dst = "empty-dir/empty-dir-copy",
		};
		assert_false(ior_cp(&args) == 0);
	}

	{
		io_args_t args =
		{
			.arg1.path = "empty-dir",
		};
		assert_int_equal(0, iop_rmdir(&args));
	}
}

void
cp_tests(void)
{
	test_fixture_start();

	run_test(test_file_is_copied);
	run_test(test_empty_directory_is_copied);
	run_test(test_non_empty_directory_is_copied);
	run_test(test_empty_nested_directory_is_copied);
	run_test(test_non_empty_nested_directory_is_copied);
	run_test(test_fails_to_overwrite_file_by_default);
	run_test(test_fails_to_overwrite_dir_by_default);
	run_test(test_overwrites_file_when_asked);
	run_test(test_overwrites_dir_when_asked);
	run_test(test_directories_can_be_merged);
	run_test(test_fails_to_copy_directory_inside_itself);

	test_fixture_end();
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 : */
