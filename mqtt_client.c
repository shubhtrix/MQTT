#include "mqtt_client.h"

char *srl_no;

/* Callback for connection. */
void on_connect (struct mosquitto *clnt_inst, void *obj, int ret) {

	printf ("Connect call-back result : %d\n", ret);
	printf ("\n");
}

/* Callback for publish. */
void on_publish (struct mosquitto *clnt_inst, void *obj, int ret) {

	printf ("Publish call-back result : %d\n", ret);
	printf ("\n");

}

void default_client ( char *server, int port) 
{
	int thread_ret=0;
    int ret, qos=1;
	pthread_t tid;
	json_error_t error;
	json_t *boot_json = NULL;
	char *bootup = NULL;
	char *sub_topic = NULL;
	char *pub_bootconfig = NULL;

	sub_topic = (char *)malloc ((sizeof(char)*100)+(strlen(srl_no)));
	sprintf( sub_topic, "devices/lcr/%s/lcr-config/+", srl_no);

    // true to tell the boker to clean all messages on disconnect
	// false to keep them under function mosquitto_new
	bool clean_session = true;

	int major=0, minor=0, revision=0;
	struct mosquitto *clnt_inst = NULL;
	struct mosq_config *cfg = NULL;

	struct thread_args *thrd_args;

	/* Library Versioning */	
	ret = mosquitto_lib_version( &major, &minor, &revision);

	printf ("Mosquitto Library Version : %d.%d.%d\n", major, minor, revision);
	printf ("Will connect over server : %s, port : %d\n", server, port);

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

		ret = mosquitto_connect_async( clnt_inst, server, port, 100);
		if ( ret == MOSQ_ERR_SUCCESS) {
			printf ("Connect call SUCCESS.\n");
			printf ("Connected waiting for Published Messages \n");
		
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

			ret = mosquitto_subscribe ( clnt_inst, NULL, sub_topic, qos);
			if (ret == MOSQ_ERR_SUCCESS) {

				printf("mosquitto_connect SUCCESS.\n");	
				ret = mosquitto_loop_forever ( clnt_inst, 0, 1);
			}

		} else if ( ret == MOSQ_ERR_INVAL) {
			printf("Connect call invalid parameters.\n");	
			return;
		} else if ( ret == MOSQ_ERR_ERRNO) {
			printf("Connect call ERROR.\n");	
			return;
		}
	}
	
	/* Destroy the CLIENT Instance */
	mosquitto_destroy ( clnt_inst);

	/* Library Clean-up */
	ret = mosquitto_lib_cleanup();
	if (ret == MOSQ_ERR_SUCCESS) {
		return;
	}
}

void usage () {

    printf("This is mosquitto based MQTT client including puablisher & subscriber.\n"); 
    printf("Usage :\n\t-d -\tDaemonize the process\n"
                   "\t-h -\tprint this help\n"
                   "\t-p -\tPort to connect to, default is 1883\n"
                   "\t-s -\tServer to connect to\n");
}

void main(int argc, char **argv) 
{
	int opt, daemon_flag=0;
	int port=1883;			//1883 is defined at IANA as MQTT over TCP
	char *server = NULL;

    while((opt = getopt(argc, argv, OPTIONS)) != -1)
	{
		switch (opt) {
		case'd':
			daemon_flag=1;
			break;
		case'h':
			usage ();
			exit (EXIT_FAILURE);
		case'p':
			port = atoi(optarg);
			if ( port < 1024) {
				port=1883;
			} else if ( port > 65534) {
				port=1883;
			} else
				printf("Provided port for broker : %d", port);
			break;
		case's':
			server = strdup(optarg);
			break;
		case'?':
			printf("Invalid option, check usage...\n");
			usage ();
			exit (EXIT_FAILURE);
        default : 
			printf("Default case, check usage...\n");
			usage ();
			exit (EXIT_FAILURE);
		}
	}

	/* Get serial no of the device. */
	srl_no = serial_no();
	
	if ( (server != NULL) && (port != 0)) {
#ifdef WITH_TLS
		download_client_certs ( server, port);
#endif
		if (!daemon_flag) {
			default_client ( server, port);
		} else {
			daemonize ( server, port);
		}
	} else {
		printf("No server address provided. Exiting...");
		exit (EXIT_FAILURE);
	}

	return;
}
