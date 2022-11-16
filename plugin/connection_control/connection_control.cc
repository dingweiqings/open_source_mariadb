/* Copyright (c) 2016, 2022, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "connection_control.h"
#include "event_handler.h"
#include "mysql/psi/mysql_stage.h"
#include <mysql/plugin.h>
#include "table.h"
#include "field.h"
#include "sql_acl.h"
#include "memory_store.h"
#include "coordinator.h"

Memory_store<std::string, int64> *data_store= nullptr;
connection_control::Connection_control_coordinator *coordinator= nullptr;
/**
 * @brief EventHandler declaration
 *
 */
Connection_event_handler *event_handler;

/* Performance Schema instrumentation */

PSI_mutex_key key_connection_delay_mutex= PSI_NOT_INSTRUMENTED;

static PSI_mutex_info all_connection_delay_mutex_info[]= {
    {&key_connection_delay_mutex, "connection_delay_mutex", 0}};

PSI_rwlock_key key_connection_event_delay_lock= PSI_NOT_INSTRUMENTED;

static PSI_rwlock_info all_connection_delay_rwlock_info[]= {
    {&key_connection_event_delay_lock, "connection_event_delay_lock", 0}};

PSI_cond_key key_connection_delay_wait= PSI_NOT_INSTRUMENTED;

static PSI_cond_info all_connection_delay_cond_info[]= {
    {&key_connection_delay_wait, "connection_delay_wait_condition", 0}};

PSI_stage_info stage_waiting_in_connection_control_plugin= {
    0, "Waiting in connection_control plugin", 0};

static PSI_stage_info *all_connection_delay_stage_info[]= {
    &stage_waiting_in_connection_control_plugin};

static void init_performance_schema()
{
  const char *category= "conn_delay";

  int count_mutex= array_elements(all_connection_delay_mutex_info);
  mysql_mutex_register(category, all_connection_delay_mutex_info, count_mutex);

  int count_rwlock= array_elements(all_connection_delay_rwlock_info);
  mysql_rwlock_register(category, all_connection_delay_rwlock_info,
                        count_rwlock);

  int count_cond= array_elements(all_connection_delay_cond_info);
  mysql_cond_register(category, all_connection_delay_cond_info, count_cond);

  int count_stage= array_elements(all_connection_delay_stage_info);
  mysql_stage_register(category, all_connection_delay_stage_info, count_stage);
}

static int64 max_connection_delay;
static int64 min_connection_delay;
static int64 failed_connections_threshold;
std::atomic<int64> connection_delay_total_count;
/**
  check() function for connection_control_max_connection_delay

  Check whether new value is within valid bounds or not.

  @param thd        Not used.
  @param var        Not used.
  @param save       Not used.
  @param value      New value for the option.

  @returns whether new value is within valid bounds or not.
    @retval 0 Value is ok
    @retval 1 Value is not within valid bounds
*/

static int check_max_connection_delay(MYSQL_THD thd,
                                      struct st_mysql_sys_var *var, void *save,
                                      struct st_mysql_value *value)
{
  longlong new_value;
  if (value->val_int(value, &new_value))
    return 1; /* NULL value */
  if (new_value >= connection_control::MIN_DELAY &&
      new_value <= connection_control::MAX_DELAY &&
      new_value >= min_connection_delay)
  {
    *(reinterpret_cast<longlong *>(save))= new_value;
    return 0;
  }
  return 1;
}

/**
  update() function for connection_control_max_connection_delay

  Updates g_connection_event_coordinator with new value.
  Also notifies observers about the update.

  @param thd        Not used.
  @param var        Not used.
  @param var_ptr    Variable information
  @param save       New value for connection_control_max_connection_delay
*/

static void update_max_connection_delay(MYSQL_THD thd,
                                        struct st_mysql_sys_var *var,
                                        void *var_ptr, const void *save)
{
  longlong new_value= *(reinterpret_cast<const longlong *>(save));
  max_connection_delay= new_value;
  coordinator->g_variables.max_connection_delay= new_value;
  return;
}

/**
  check() function for connection_control_failed_connections_threshold

  Check whether new value is within valid bounds or not.

  @param thd        Not used.
  @param var        Not used.
  @param save       Not used.
  @param value      New value for the option.

  @returns whether new value is within valid bounds or not.
    @retval 0 Value is ok
    @retval 1 Value is not within valid bounds
*/

static int check_failed_connections_threshold(MYSQL_THD thd,
                                              struct st_mysql_sys_var *var,
                                              void *save,
                                              struct st_mysql_value *value)
{
  longlong new_value;
  if (value->val_int(value, &new_value))
    return 1; /* NULL value */

  if (new_value >= connection_control::MIN_THRESHOLD &&
      new_value <= connection_control::MAX_THRESHOLD)
  {
    *(reinterpret_cast<longlong *>(save))= new_value;
    return 0;
  }

  return 1;
}

/**
  update() function for connection_control_failed_connections_threshold

  Updates g_connection_event_coordinator with new value.
  Also notifies observers about the update.

  @param thd        Not used.
  @param var        Not used.
  @param var_ptr    Variable information
  @param save       New value for
  connection_control_failed_connections_threshold
*/

static void update_failed_connections_threshold(MYSQL_THD thd,
                                                struct st_mysql_sys_var *var,
                                                void *var_ptr,
                                                const void *save)
{
  /*
    This won't result in overflow because we have already checked that this is
    within valid bounds.
  */
  longlong new_value= *(reinterpret_cast<const longlong *>(save));
  failed_connections_threshold= new_value;
  coordinator->write_lock();
  coordinator->g_variables.failed_connections_threshold=new_value;
  connection_delay_total_count=0;
  data_store->clear();
  coordinator->unlock();
  return;
}

/**
  check() function for connection_control_min_connection_delay

  Check whether new value is within valid bounds or not.

  @param thd        Not used.
  @param var        Not used.
  @param save       Not used.
  @param value      New value for the option.

  @returns whether new value is within valid bounds or not.
    @retval 0 Value is ok
    @retval 1 Value is not within valid bounds
*/

static int check_min_connection_delay(MYSQL_THD thd,
                                      struct st_mysql_sys_var *var, void *save,
                                      struct st_mysql_value *value)
{
  long long new_value;
  if (value->val_int(value, &new_value))
    return 1; /* NULL value */
  if (new_value >= connection_control::MIN_DELAY &&
      new_value <= connection_control::MAX_DELAY &&
      new_value <= max_connection_delay)
  {
    *(reinterpret_cast<longlong *>(save))= new_value;
    return 0;
  }
  return 1;
}

/**
  update() function for connection_control_min_connection_delay

  Updates g_connection_event_coordinator with new value.
  Also notifies observers about the update.

  @param thd        Not used.
  @param var        Not used.
  @param var_ptr    Variable information
  @param save       New value for connection_control_min_connection_delay
*/

static void update_min_connection_delay(MYSQL_THD thd,
                                        struct st_mysql_sys_var *var,
                                        void *var_ptr, const void *save)
{
  longlong new_value= *(reinterpret_cast<const longlong *>(save));
  min_connection_delay= new_value;
  coordinator->g_variables.min_connection_delay= new_value;
  return;
}

/** Declaration of connection_control_failed_connections_threshold */
static MYSQL_SYSVAR_LONGLONG(
    failed_connections_threshold, failed_connections_threshold,
    PLUGIN_VAR_RQCMDARG,
    "Failed connection threshold to trigger delay. Default is 3.",
    check_failed_connections_threshold, update_failed_connections_threshold,
    connection_control::DEFAULT_THRESHOLD, connection_control::MIN_THRESHOLD,
    connection_control::MAX_THRESHOLD, 1);

/** Declaration of connection_control_max_connection_delay */
static MYSQL_SYSVAR_LONGLONG(
    min_connection_delay, min_connection_delay, PLUGIN_VAR_RQCMDARG,
    "Maximum delay to be introduced. Default is 1000.",
    check_min_connection_delay, update_min_connection_delay,
    connection_control::DEFAULT_MIN_DELAY, connection_control::MIN_DELAY,
    connection_control::MAX_DELAY, 1);

/** Declaration of connection_control_max_connection_delay */
static MYSQL_SYSVAR_LONGLONG(
    max_connection_delay, max_connection_delay, PLUGIN_VAR_RQCMDARG,
    "Maximum delay to be introduced. Default is 2147483647.",
    check_max_connection_delay, update_max_connection_delay,
    connection_control::DEFAULT_MAX_DELAY, connection_control::MIN_DELAY,
    connection_control::MAX_DELAY, 1);

/** Array of system variables. Used in plugin declaration. */
static struct st_mysql_sys_var *connection_control_system_variables[3]= {
    MYSQL_SYSVAR(max_connection_delay), MYSQL_SYSVAR(min_connection_delay),
    MYSQL_SYSVAR(failed_connections_threshold)};

/**
  Function to display value for status variable :
  Connection_control_delay_generated

  @param thd  MYSQL_THD handle. Unused.
  @param var  Status variable structure
  @param buff Value buffer.

  @returns Always returns success.
*/

static int show_delay_generated(MYSQL_THD thd, SHOW_VAR *var, void *buff)
{
  var->type= SHOW_LONGLONG;
  var->value= buff;
  longlong *value= reinterpret_cast<longlong *>(buff);

  if (!data_store)
  {
    *value= 0;
    return 0;
  }
  *value= connection_delay_total_count.load();
  return 0;
}

/** Array of status variables. Used in plugin declaration. */
struct st_mysql_show_var connection_control_status_variables[]= {
    {"Connection_control_delay_generated", (char *) &show_delay_generated,
     enum_mysql_show_type::SHOW_SIMPLE_FUNC},
    {0, 0, (enum_mysql_show_type) 0}};

struct st_mysql_information_schema connection_control_failed_attempts_view= {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION};

namespace Show
{

static ST_FIELD_INFO failed_attempts_view_fields[]= {
    Column("USER_HOST", Varchar(USERNAME_LENGTH + HOSTNAME_LENGTH + 5*8),
           NOT_NULL, "User_host"),
    Column("FAILED_ATTEMPTS", ULong(), NOT_NULL, "Failed_attempts"), CEnd()};

} // namespace Show

int fill_failed_attempts_view(THD *thd, TABLE_LIST *tables, Item *cond)
{
  if (check_global_access(thd, PROCESS_ACL, true))
  {
    push_warning_printf(thd, Sql_condition::WARN_LEVEL_WARN,
                        ER_WRONG_ARGUMENTS, "Access denied!");
    return 0;
  }

  TABLE *table= tables->table;
  int64 i= 0;
  //if not install connection control plugin, view return an empty table
  if (!data_store)
  {
    return 0;
  }
  // Get all data
  data_store->foreach ([table, thd, i](std::pair<std::string, int64> pair) {
    table->field[0]->store(pair.first.c_str(), pair.first.length(),
                           system_charset_info);
    table->field[1]->store(pair.second, true);
    schema_table_store_record(thd, table);
  });

  return 0;
}

/**
  View init function

  @param [in] ptr    Handle to
                     information_schema.connection_control_failed_attempts.

  @returns Always returns 0.
*/

int connection_control_failed_attempts_view_init(void *ptr)
{
  ST_SCHEMA_TABLE *schema_table= (ST_SCHEMA_TABLE *) ptr;

  schema_table->fields_info= Show::failed_attempts_view_fields;
  schema_table->fill_table= fill_failed_attempts_view;
  return 0;
}

/**
  Plugin initialization function
  @param [in] plugin_info  MYSQL_PLUGIN information

  @returns initialization status
    @retval 0 Success
    @retval 1 Failure
*/

static int connection_control_init(MYSQL_PLUGIN plugin_info)
{
  /*
    Declare all performance schema instrumentation up front,
    so it is discoverable.
  */
  std::atomic_init(&connection_delay_total_count,0);
  if (!event_handler)
  {
    event_handler= new Connection_event_handler();
  }
  if (!data_store)
  {
    data_store= new Memory_store<std::string, int64>();
  }
  if (!coordinator)
  {
    coordinator= new connection_control::Connection_control_coordinator(
        connection_control::DEFAULT_THRESHOLD,
        connection_control::DEFAULT_MIN_DELAY,
        connection_control::DEFAULT_MAX_DELAY);
  }

  if (event_handler && data_store && coordinator)
  {
    // my_printf_error(ME_NOTE, "Connection control plugin init success",
    //                 MYF(ME_NOTE | ME_ERROR_LOG));
     init_performance_schema();
    return 0;
  }

  my_printf_error(ER_PLUGIN_INSTALLED,
                  "Connection control plugin init error",
                  MYF(ME_NOTE | ME_ERROR_LOG));
  return 1;
}

/**
  Plugin deinitialization
  @param arg  Unused

  @returns success
*/

static int connection_control_deinit(void *arg)
{
  // delete data store
  // delete coordinator
  // delete event handler
  if (event_handler)
  {
    delete event_handler;
  }
  if (coordinator)
  {
    delete coordinator;
  }
  if (data_store)
  {
    delete data_store;
  }
  my_printf_error(ME_NOTE, "Connection control plugin uninstall success",
                  MYF(ME_ERROR_LOG | ME_NOTE));
  return 0;
}

void release_thd(MYSQL_THD THD) { event_handler->release_thd(THD); }

void receive_event(MYSQL_THD THD, unsigned int event_class, const void *event)
{
  event_handler->receive_event(THD, event_class, event);
}

/*entry plugin entry point*/
struct st_mysql_audit plugin_main_handler= {
    MYSQL_AUDIT_INTERFACE_VERSION,
    release_thd,
    receive_event,
    {MYSQL_AUDIT_CONNECTION_CLASSMASK} /* Add table and general event mask*/
};

maria_declare_plugin(connection_control){MYSQL_AUDIT_PLUGIN,
                                         &plugin_main_handler,
                                         "connection_control",
                                         "KurtDing",
                                         "connection_control desc",
                                         PLUGIN_LICENSE_GPL,
                                         connection_control_init,
                                         connection_control_deinit,
                                         0x0100,
                                         connection_control_status_variables,
                                         connection_control_system_variables,
                                         "1.0",
                                         MariaDB_PLUGIN_MATURITY_STABLE},
    {MYSQL_INFORMATION_SCHEMA_PLUGIN,
     &connection_control_failed_attempts_view,
     "CONNECTION_CONTROL_FAILED_LOGIN_ATTEMPTS",
     "KurtDing",
     "I_S table providing a view into failed attempts statistics",
     PLUGIN_LICENSE_GPL,
     connection_control_failed_attempts_view_init,
     NULL,
     0x0100,
     NULL,
     NULL,
     "1.0",
     MariaDB_PLUGIN_MATURITY_STABLE} maria_declare_plugin_end;
