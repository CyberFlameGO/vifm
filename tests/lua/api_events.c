#include <stic.h>

#include "../../src/cfg/config.h"
#include "../../src/compat/os.h"
#include "../../src/lua/vlua.h"
#include "../../src/ui/statusbar.h"
#include "../../src/utils/str.h"
#include "../../src/ops.h"
#include "../../src/status.h"

#include <test-utils.h>

static vlua_t *vlua;

SETUP()
{
	vlua = vlua_init();
}

TEARDOWN()
{
	vlua_finish(vlua);
}

TEST(bad_event_name)
{
	ui_sb_msg("");
	assert_failure(vlua_run_string(vlua,
	      "vifm.events.listen {"
	      "  event = 'random',"
	      "  handler = function() end"
	      "}"));
	assert_true(ends_with(ui_sb_last(), ": No such event: random"));
}

TEST(app_exit_is_called)
{
	assert_success(vlua_run_string(vlua,
	      "i = 0\n"
	      "vifm.events.listen {"
	      "  event = 'app.exit',"
	      "  handler = function() i = i + 1 end"
	      "}"));

	vlua_events_app_exit(vlua);
	vlua_process_callbacks(vlua);

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print(i)"));
	assert_string_equal("1", ui_sb_last());
}

TEST(app_fsop_is_called)
{
	assert_success(vlua_run_string(vlua,
	      "data = {}\n"
	      "vifm.events.listen {"
	      "  event = 'app.fsop',"
	      "  handler = function(info) data[#data + 1] = info end"
	      "}"));

	conf_setup();
	cfg.use_system_calls = 1;
	curr_stats.vlua = vlua;
	assert_success(os_chdir(SANDBOX_PATH));
	assert_int_equal(OPS_SUCCEEDED, perform_operation(OP_USR, NULL, NULL,
				"echo", NULL));
	assert_int_equal(OPS_SUCCEEDED, perform_operation(OP_MKFILE, NULL, NULL,
				"from", NULL));
	assert_int_equal(OPS_SUCCEEDED, perform_operation(OP_MOVE, NULL, NULL,
				"from", "to"));
	assert_int_equal(OPS_SUCCEEDED, perform_operation(OP_REMOVESL, NULL, NULL,
				"to", NULL));
	vlua_process_callbacks(vlua);
	curr_stats.vlua = NULL;
	cfg.use_system_calls = 0;
	conf_teardown();

	ui_sb_msg("");
	assert_success(vlua_run_string(vlua, "print(#data)"));
	assert_string_equal("3", ui_sb_last());
	assert_success(vlua_run_string(vlua, "print(data[1].op,"
	                                           "data[1].path,"
	                                           "data[1].target,"
	                                           "data[1].isdir)"));
	assert_string_equal("create\tfrom\tnil\tfalse", ui_sb_last());
	assert_success(vlua_run_string(vlua, "print(data[2].op,"
	                                           "data[2].path,"
	                                           "data[2].target,"
	                                           "data[2].fromtrash,"
	                                           "data[2].totrash,"
	                                           "data[2].isdir)"));
	assert_string_equal("move\tfrom\tto\tfalse\tfalse\tfalse", ui_sb_last());
	assert_success(vlua_run_string(vlua, "print(data[3].op,"
	                                           "data[3].path,"
	                                           "data[3].target,"
	                                           "data[3].isdir)"));
	assert_string_equal("remove\tto\tnil\tfalse", ui_sb_last());
}

TEST(same_handler_is_called_once)
{
	ui_sb_msg("");

	assert_success(vlua_run_string(vlua,
	      "i = 0\n"
	      "handler = function() i = i + 1 end\n"
	      "vifm.events.listen { event = 'app.exit', handler = handler }"
	      "vifm.events.listen { event = 'app.exit', handler = handler }"));

	vlua_events_app_exit(vlua);
	vlua_process_callbacks(vlua);
	assert_string_equal("", ui_sb_last());

	assert_success(vlua_run_string(vlua, "print(i)"));
	assert_string_equal("1", ui_sb_last());
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 : */
