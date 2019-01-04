#include "mqtt_client.h"

void daemonize ( char *server, int port)
{
	pid_t PID, SID;
	FILE *LOG_FILE;

    int ret, qos=1;
	char *id = "RESTCONF";

    // true to tell the boker to clean all messages on disconnect
	// false to keep them under function mosquitto_new
	bool clean_session = true;

	int major=0, minor=0, revision=0;
	struct mosquitto *clnt_inst = NULL;
	struct mosq_config *cfg = NULL;

	/* Library Versioning */	
	ret = mosquitto_lib_version( &major, &minor, &revision);

	printf("Mosquitto Library Version : %d.%d.%d\n", major, minor, revision);
	syslog (LOG_INFO, "MOSQUITTO WILL CONNECT OVER SERVER : %s, port : %d\n",
					server, port);
   
    /* Library Initialization */
	ret = mosquitto_lib_init();
	if (ret == MOSQ_ERR_SUCCESS) {

	    /* Actual CLIENT Instance */
		clnt_inst = mosquitto_new ( id, clean_session, NULL);

		mosquitto_connect_callback_set ( clnt_inst, on_connect);
		mosquitto_message_callback_set ( clnt_inst, on_message);

		ret = mosquitto_connect (clnt_inst, server, port, 100);
		if (ret == MOSQ_ERR_SUCCESS) {
			printf("Connect call SUCCESS.\n");	
			printf("Connected waiting for Published Messages \n");
			ret = mosquitto_subscribe(clnt_inst, NULL, id, qos);
			if (ret == MOSQ_ERR_SUCCESS)
				printf("Subscribe.\n");
		} else if (ret == MOSQ_ERR_INVAL) {
			printf("Connect call invalid parameters.\n");	
			return;
		} else if (ret == MOSQ_ERR_ERRNO) {
			printf("Connect call ERROR.\n");	
			return;
		}
	}

	PID = fork();

	if (PID > 0) {exit (EXIT_SUCCESS);}
	if (PID < 0) {exit (EXIT_FAILURE);}

	umask(0);
	
	if ((SID=setsid()) < 0) {
		syslog (LOG_INFO, "SHUBH: FAILED SETSID");
		exit (EXIT_FAILURE);
	}

 	if (chdir("/") < 0) {
		syslog (LOG_INFO,"SHUBH: FAILED CHDIR");
		exit (EXIT_FAILURE);
	}

	close (STDIN_FILENO);
	close (STDOUT_FILENO);
	close (STDERR_FILENO);

	syslog (LOG_INFO, "MOSQUITTO CLIENT DAEMONIZING.");
	ret = mosquitto_loop_forever (clnt_inst, -1, 1);
		
    /* Destroy the CLIENT Instance */
	mosquitto_destroy (clnt_inst);			

	/* Library Clean-up */
	ret = mosquitto_lib_cleanup();
	if (ret == MOSQ_ERR_SUCCESS) {
		return;
	}

	exit (EXIT_SUCCESS);
}
