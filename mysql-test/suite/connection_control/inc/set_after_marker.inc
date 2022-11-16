# set after marker
# $SERVER_RESPONSE_TIME [IN] Expected delay
disable_query_log;
disable_result_log;
SET @after= TIMESTAMP(current_time());
SET @server_response_time= TIMESTAMPDIFF(SECOND, @before, @after);
enable_result_log;

--eval SELECT abs(@server_response_time - $SERVER_RESPONSE_TIME) <= 2 
enable_query_log;
