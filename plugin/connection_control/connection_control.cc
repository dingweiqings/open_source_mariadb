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
#include <my_global.h>
#include "connection_control.h"
#include <stddef.h>
#include<stdio.h>
#include"event_handler.h"
#include <mysql/plugin.h>
#include "table.h"
#include "field.h"
#include"data.h"

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

static int check_max_connection_delay(MYSQL_THD thd ,
                                       struct st_mysql_sys_var *var ,
                                      void *save ,
                                      struct st_mysql_value *value) {
  long long new_value;
  uint64 existing_value = coordinator.g_variables.getMaxConnectionDelay();
  if (value->val_int(value, &new_value)) return 1; /* NULL value */
   uint64 curValue=(uint64)new_value;
  if (curValue >= connection_control::MIN_DELAY &&
      curValue <= connection_control::MAX_DELAY &&
      curValue >= existing_value) {
    *(reinterpret_cast<longlong *>(save)) = new_value;
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

static void update_max_connection_delay(MYSQL_THD thd ,
                                        struct st_mysql_sys_var *var ,
                                        void *var_ptr ,
                                        const void *save) {
  long long new_value = *(reinterpret_cast<const longlong *>(save));
  coordinator.g_variables.setMaxDelay((uint64)new_value);
  return;
}

struct st_mysql_information_schema connection_control_failed_attempts_view = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION};

namespace Show {

static ST_FIELD_INFO failed_attempts_view_fields[]=
{
  Column("USER_HOST",Varchar(USERNAME_LENGTH+HOSTNAME_LENGTH+6),NOT_NULL,"User_host"),
 Column("FAILED_ATTEMPTS",ULong(),NOT_NULL,"Failed_attempts"),
  CEnd()
};

} // namespace Show


int fill_failed_attempts_view(THD *thd, TABLE_LIST *tables, Item *cond) {
      TABLE *table = tables->table;
      table->field[0]->store("127.0.0.1",strlen("127.0.0.1") ,
                             system_charset_info);
      table->field[1]->store(1, true);
      schema_table_store_record(thd, table);
  return false;
}

/**
  View init function

  @param [in] ptr    Handle to
                     information_schema.connection_control_failed_attempts.

  @returns Always returns 0.
*/

int connection_control_failed_attempts_view_init(void *ptr) {
  ST_SCHEMA_TABLE *schema_table = (ST_SCHEMA_TABLE *)ptr;

  schema_table->fields_info = Show::failed_attempts_view_fields;
  schema_table->fill_table = fill_failed_attempts_view;
  return 0;
}


static long long max_connection_delay;

/** Declaration of connection_control_max_connection_delay */
static MYSQL_SYSVAR_LONGLONG(
    max_connection_delay, max_connection_delay, PLUGIN_VAR_RQCMDARG,
    "Maximum delay to be introduced. Default is 2147483647.",
    check_max_connection_delay, update_max_connection_delay,
    1, 1,  1, 1);
 /** Array of system variables. Used in plugin declaration. */
static struct st_mysql_sys_var*  connection_control_system_variables[1] = {
        MYSQL_SYSVAR(max_connection_delay)};

/**
  Function to display value for status variable :
  Connection_control_delay_generated

  @param thd  MYSQL_THD handle. Unused.
  @param var  Status variable structure
  @param buff Value buffer.

  @returns Always returns success.
*/

// static int show_delay_generated(MYSQL_THD thd [[maybe_unused]], SHOW_VAR *var,
//                                 char *buff) {
//   var->type = SHOW_LONGLONG;
//   var->value = buff;
//   longlong *value = reinterpret_cast<longlong *>(buff);
//   int64 current_val =
//       g_statistics.stats_array[STAT_CONNECTION_DELAY_TRIGGERED].load();
//   *value = static_cast<longlong>(current_val);
//   return 0;
// }

static volatile int dwq;
/** Array of status variables. Used in plugin declaration. */
struct st_mysql_show_var simple_status[]=
{
  { "dwq_called", (char *) &dwq, SHOW_INT },
  { 0, 0, SHOW_INT}
};



Connection_event_handler event_handler=Connection_event_handler();

 void release_thd(MYSQL_THD THD){
        event_handler.release_thd(THD);
 }

void receive_event(MYSQL_THD THD, unsigned int event_class,
                          const void *event){
                            event_handler.receive_event(THD,event_class,event);
                          }


/*entry plugin entry point*/
struct st_mysql_audit plugin_main_handler=
{
  MYSQL_AUDIT_INTERFACE_VERSION,
release_thd,receive_event,
   {MYSQL_AUDIT_CONNECTION_CLASSMASK} /* Add table and general event mask*/
};



maria_declare_plugin(connection_control)
{
  MYSQL_AUDIT_PLUGIN,
  &plugin_main_handler,
  "demo_plugin",
  "KurtDing",
  "Demo plugin desc",
  PLUGIN_LICENSE_GPL,
  NULL,
  NULL,
  0x0100,
  simple_status,
  connection_control_system_variables,
  "1.0",
  MariaDB_PLUGIN_MATURITY_STABLE
}
,
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
     MariaDB_PLUGIN_MATURITY_STABLE}
maria_declare_plugin_end;
