#include <stic.h>

#include "../../src/cfg/config.h"
#include "../../src/engine/keys.h"
#include "../../src/engine/variables.h"
#include "../../src/lua/vlua.h"
#include "../../src/modes/modes.h"
#include "../../src/modes/wk.h"
#include "../../src/ui/statusbar.h"
#include "../../src/ui/ui.h"
#include "../../src/utils/str.h"
#include "../../src/utils/utils.h"
#include "../../src/cmd_core.h"
#include "../../src/event_loop.h"
#include "../../src/status.h"

#include <test-utils.h>

static vlua_t *vlua;

SETUP()
{
	make_abs_path(cfg.config_dir, sizeof(cfg.config_dir), SANDBOX_PATH, "", NULL);

	vlua = vlua_init();
	curr_view = &lwin;
	other_view = &rwin;

	engine_cmds_setup(/*real_completion=*/0);
}

TEARDOWN()
{
	vlua_finish(vlua);
	engine_cmds_teardown();
}

TEST(print_outputs_to_statusbar)
{
	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print('arg1', 'arg2')"));
	assert_string_equal("arg1\targ2", ui_sb_last());
}

TEST(os_getenv_works)
{
	init_variables();

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print(os.getenv('VIFM_TEST'))"));
	assert_string_equal("nil", ui_sb_last());

	assert_success(let_variables("$VIFM_TEST='test'"));

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print(os.getenv('VIFM_TEST'))"));
	assert_string_equal("test", ui_sb_last());

	clear_variables();
}

TEST(vifm_errordialog)
{
	assert_failure(vlua_run_string(vlua, "vifm.errordialog('title')"));
	assert_success(vlua_run_string(vlua, "vifm.errordialog('title', 'msg')"));
}

TEST(fnamemodify)
{
	ui_sb_msg("");

	assert_success(vlua_run_string(vlua,
				"print(vifm.fnamemodify('/a/b/c.d', ':t:r'))"));
	assert_string_equal("c", ui_sb_last());

	assert_success(vlua_run_string(vlua,
				"print(vifm.fnamemodify('c.d', ':p', '/parent'))"));
	assert_string_equal("/parent/c.d", ui_sb_last());
}

TEST(vifm_escape)
{
	ui_sb_msg("");

	assert_success(vlua_run_string(vlua,
				"print(vifm.escape(' '))"));
	assert_string_equal(get_env_type() == ET_UNIX ? "\\ " : "\" \"",
			ui_sb_last());
}

TEST(vifm_exists)
{
	ui_sb_msg("");

	assert_success(vlua_run_string(vlua, "testdata = '" TEST_DATA_PATH "'"));

	assert_success(vlua_run_string(vlua,
				"print(vifm.exists('/no/such/path!') and 'y' or 'n')"));
	assert_string_equal("n", ui_sb_last());

	assert_success(vlua_run_string(vlua,
				"print(vifm.exists(testdata) and 'y' or 'n')"));
	assert_string_equal("y", ui_sb_last());

	assert_success(vlua_run_string(vlua,
				"print(vifm.exists(testdata..'/existing-files/a') and 'y' or 'n')"));
	assert_string_equal("y", ui_sb_last());
}

TEST(vifm_makepath)
{
	assert_success(vlua_run_string(vlua, "sandbox = '" SANDBOX_PATH "'"));

	assert_success(vlua_run_string(vlua, "vifm.makepath(sandbox..'/dir')"));
	remove_dir(SANDBOX_PATH "/dir");

	assert_success(vlua_run_string(vlua, "vifm.makepath(sandbox..'/dir1/dir2')"));
	remove_dir(SANDBOX_PATH "/dir1/dir2");
	remove_dir(SANDBOX_PATH "/dir1");
}

TEST(vifm_expand)
{
	copy_str(curr_view->curr_dir, sizeof(curr_view->curr_dir), "/tst");

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print(vifm.expand('%d'))"));
	assert_string_equal("/tst", ui_sb_last());

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.expand('%d:p:gs!/!\\\\\\\\!'))"));
	assert_string_equal("\\\\tst", ui_sb_last());
}

TEST(vifmview_cd)
{
	stub_colmgr();
	conf_setup();
	view_setup(curr_view);

	assert_success(vlua_run_string(vlua, "testdata = '" TEST_DATA_PATH "'"));

	copy_str(curr_view->curr_dir, sizeof(curr_view->curr_dir), "/tst");

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.currview():cd(testdata) and 'y' or 'n')"));
	assert_string_equal("y", ui_sb_last());

	view_teardown(curr_view);
	conf_teardown();
}

TEST(sb_info_outputs_to_statusbar)
{
	ui_sb_msg("");
	curr_stats.save_msg = 0;

	assert_success(vlua_run_string(vlua, "vifm.sb.info 'info'"));

	assert_string_equal("info", ui_sb_last());
	assert_int_equal(1, curr_stats.save_msg);
}

TEST(sb_error_outputs_to_statusbar)
{
	ui_sb_msg("");
	curr_stats.save_msg = 0;

	assert_success(vlua_run_string(vlua, "vifm.sb.error 'err'"));

	assert_string_equal("err", ui_sb_last());
	assert_int_equal(1, curr_stats.save_msg);
}

TEST(sb_quick_message_is_not_stored)
{
	ui_sb_msg("");
	curr_stats.save_msg = 0;

	assert_success(vlua_run_string(vlua, "vifm.sb.quick 'msg'"));

	assert_string_equal("", ui_sb_last());
	assert_int_equal(0, curr_stats.save_msg);
}

TEST(vifm_currview)
{
	cmds_init();

	conf_setup();
	cfg.pane_tabs = 0;
	view_setup(curr_view);
	view_setup(other_view);
	opt_handlers_setup();
	columns_setup_column(SK_BY_NAME);
	columns_setup_column(SK_BY_SIZE);

	assert_success(vlua_run_string(vlua, "l = vifm.currview()"));
	swap_view_roles();
	assert_success(vlua_run_string(vlua, "r = vifm.currview()"));

	/* Both non-nil and aren't equal. */
	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print(l and 'y' or 'n')"));
	assert_string_equal("y", ui_sb_last());
	assert_success(vlua_run_string(vlua, "print(r and 'y' or 'n')"));
	assert_string_equal("y", ui_sb_last());
	assert_success(vlua_run_string(vlua, "print(r ~= l and 'y' or 'n')"));
	assert_string_equal("y", ui_sb_last());

	/* Can access visible views. */
	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "r:cd('/')"));
	assert_string_equal("", ui_sb_last());
	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "l:cd('/')"));
	assert_string_equal("", ui_sb_last());

	/* Can't access view that can't be found. */
	++curr_view->id;
	ui_sb_msg("");
	assert_failure(vlua_run_string(vlua, "r:cd('bla')"));
	assert_true(ends_with(ui_sb_last(),
				"Invalid VifmView object (associated view is dead)"));

	assert_success(cmds_dispatch1("tabnew", curr_view, CIT_COMMAND));

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "l:cd('/')"));
	assert_string_equal("", ui_sb_last());

	assert_success(cmds_dispatch1("tabonly", curr_view, CIT_COMMAND));

	columns_teardown();
	opt_handlers_teardown();
	view_teardown(curr_view);
	view_teardown(other_view);
	conf_teardown();
}

TEST(vifm_version)
{
	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.version.api.has('feature'))"));
	assert_string_equal("false", ui_sb_last());

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.version.api.atleast(0, 0, 0))"));
	assert_string_equal("true", ui_sb_last());
}

TEST(vifm_run)
{
	conf_setup();

	ui_sb_msg("");
	assert_failure(vlua_run_string(vlua,
				"print(vifm.run({ cmd = 'exit 3',"
				                " usetermmux = false,"
				                " pause = 'unknown' }))"));
	assert_true(ends_with(ui_sb_last(),
				": Unrecognized value for `pause`: unknown"));

	/* This waits for user input on Windows. */
#ifndef _WIN32
	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.run({ cmd = 'exit 0',"
				                " usetermmux = false,"
				                " pause = 'onerror' }))"));
	assert_string_equal("0", ui_sb_last());

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.run({ cmd = 'exit 1',"
				                " usetermmux = false,"
				                " pause = 'always' }))"));
	assert_string_equal("1", ui_sb_last());
#endif

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua,
				"print(vifm.run({ cmd = 'exit 2',"
				                " usetermmux = false,"
				                " pause = 'never' }))"));
	assert_string_equal("2", ui_sb_last());

	conf_teardown();
}

TEST(vifm_input)
{
	view_setup(&lwin);
	view_setup(&rwin);

	cfg.timeout_len = 1;

	modes_init();

	ui_sb_msg("");

	/* Preparing input beforehand, because input() runs nested event loop. */
	feed_keys(L"def" WK_C_m);
	assert_success(vlua_run_string(vlua,
				"print(vifm.input({ prompt = 'write: ',"
				                  " initial = 'abc',"
				                  " complete = 'dir' }))"));
	assert_string_equal("abcdef", ui_sb_last());

	/* Preparing input beforehand, because input() runs nested event loop. */
	feed_keys(L"xyz" WK_C_m);
	assert_success(vlua_run_string(vlua,
				"print(vifm.input({ prompt = 'write: ',"
				                  " complete = 'file' }))"));
	assert_string_equal("xyz", ui_sb_last());

	/* Preparing input beforehand, because input() runs nested event loop. */
	feed_keys(WK_C_c);
	assert_success(vlua_run_string(vlua,
				"print(vifm.input({ prompt = 'write:' }))"));
	assert_string_equal("nil", ui_sb_last());

	assert_failure(vlua_run_string(vlua,
				"print(vifm.input({ prompt = 'write: ',"
				                  " complete = 'bla' }))"));
	assert_true(ends_with(ui_sb_last(),
				": Unrecognized value for `complete`: bla"));

	vle_keys_reset();

	view_teardown(&lwin);
	view_teardown(&rwin);
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 : */
