#include "mqtt_client.h"

void daemonize ( char *server, int port)
{
	int thread_ret=0;
	pid_t PID, SID;
	pthread_t tid;
	char *bootup = NULL;
	json_error_t error;
	json_t *boot_json = NULL;
	char *sub_topic = NULL;
	char *pub_bootconfig = NULL;
	FILE *LOG_FILE;

    int ret, qos=1;

	sub_topic = (char *)malloc ((sizeof(char)*100)+(strlen(srl_no)));
	sprintf( sub_topic, "devices/lcr/%s/lcr-config/+", srl_no);

    // true to tell the boker to clean all messages on disconnect
	// false to keep them under function mosquitto_new
	bool clean_session = true;

	int major=0, minor=0, revision=0;
	struct mosquitto *clnt_inst = NULL;
	struct mosq_config *cfg = NULL;

	struct thread_args *thrd_args;

	PID = fork();

	if (PID > 0) {exit (EXIT_SUCCESS);}
	if (PID < 0) {exit (EXIT_FAILURE);}

	if ((SID=setsid()) < 0) {
		syslog (LOG_INFO, "SHUBH: FAILED SETSID");
		exit (EXIT_FAILURE);
	}

	close (STDIN_FILENO);
	close (STDOUT_FILENO);
	close (STDERR_FILENO);

	/* Library Versioning */	
	ret = mosquitto_lib_version( &major, &minor, &revision);

	printf("Mosquitto Library Version : %d.%d.%d\n", major, minor, revision);
	syslog (LOG_INFO, "MOSQUITTO WILL CONNECT OVER SERVER : %s, port : %d\n",
					server, port);
   
    /* Library Initialization */
	ret = mosquitto_lib_init();
	if (ret == MOSQ_ERR_SUCCESS) {

	    /* Actual CLIENT Instance */
		clnt_inst = mosquitto_new ( sub_topic, clean_session, NULL);

		/* Tell Mosquitto library about threaded environment. */
		mosquitto_threaded_set (clnt_inst, true);

#ifdef WITH_TLS
		
		/* Setting-up the TLS connection authentication if defined. */
		ret = mosquitto_tls_opts_set(clnt_inst, 0, NULL, NULL);
		if(ret != 0) {
			printf("Mosquitto TLS OPTS set failed.\n");
			exit(EXIT_FAILURE);
	    }	
		
		ret = mosquitto_tls_set(clnt_inst, "/www-data/certs/ca-crt.pem", 
								"/www-data/certs/", "/www-data/certs/client.crt", 
								"/www-data/certs/client.key", NULL);
		if(ret != 0) {
			printf("Mosquitto TLS set failed.\n");
			exit(EXIT_FAILURE);
		}

#endif

		mosquitto_connect_callback_set ( clnt_inst, on_connect);
		mosquitto_message_callback_set ( clnt_inst, on_message);
		mosquitto_publish_callback_set ( clnt_inst, on_publish);

		ret = mosquitto_connect_async ( clnt_inst, server, port, 100);
		if (ret == MOSQ_ERR_SUCCESS) {
			printf("Connect call SUCCESS.\n");	
			printf("Connected waiting for Published Messages \n");

			/* First time publishement of boot-up configuration from LCR. */
			pub_bootconfig = (char *)malloc ((sizeof(char)*100)+(strlen(srl_no)));
			sprintf( pub_bootconfig, "devices/lcr/%s/bootup-config", srl_no);	
			boot_json=json_load_file ("/www-data/.boot_json", 0, &error);
			if (!boot_json) {
				printf("Bootup json creation failed.\n");
				exit (EXIT_FAILURE);
			} else {
				bootup = json_dumps(boot_json, JSON_COMPACT);
				mosquitto_publish( clnt_inst, NULL, pub_bootconfig, 
									strlen(bootup), bootup, 0, false);
				free (bootup);
			}
			free (pub_bootconfig);

			/* Fill thread argumetns to thread_args to send to status thread. */
			thrd_args=(struct thread_args *)malloc( sizeof (struct thread_args));
			thrd_args->ip = strdup(server);
			thrd_args->port = port;
			thrd_args->inst = clnt_inst;

			/* Seperate thread for status-updates after every 30 sec. */
			thread_ret=pthread_create( &tid, NULL, status_handler, (void  *)thrd_args);
			if (thread_ret != 0) {

				printf("Thread for status publishing creation failed. Exiting.\t");
				printf("Exit status : %d\n", thread_ret);
				exit (EXIT_FAILURE);
	
			} else
				pthread_detach ( tid);

			ret = mosquitto_subscribe(clnt_inst, NULL, sub_topic, qos);
			if (ret == MOSQ_ERR_SUCCESS) {
				printf("Subscribe.\n");
				syslog (LOG_INFO, "MOSQUITTO CLIENT DAEMONIZING.");
				ret = mosquitto_loop_forever (clnt_inst, 0, 1);
			}
		} else if (ret == MOSQ_ERR_INVAL) {
			printf("Connect call invalid parameters.\n");	
			return;
		} else if (ret == MOSQ_ERR_ERRNO) {
			printf("Connect call ERROR.\n");	
			return;
		}
	}
		
    /* Destroy the CLIENT Instance */
	mosquitto_destroy (clnt_inst);			

	/* Library Clean-up */
	ret = mosquitto_lib_cleanup();
	if (ret == MOSQ_ERR_SUCCESS) {
		return;
	}

	exit (EXIT_SUCCESS);
}
