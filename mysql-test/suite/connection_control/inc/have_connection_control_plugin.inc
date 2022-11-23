disable_query_log;
--source include/not_embedded.inc
# Check if server has support for loading plugin

if (`SELECT @@have_dynamic_loading != 'YES'`) {
  --skip The connection_control plugin requires dynamic loading
}

#
# Check if the variable CONNECTION_CONTROL_PLUGIN is set
#
if (!$CONNECTION_CONTROL_PLUGIN) {
  --skip The connection_control plugin requires the environment variable \$CONNECTION_CONTROL_PLUGIN to be set (normally done by mtr)
}

enable_query_log;