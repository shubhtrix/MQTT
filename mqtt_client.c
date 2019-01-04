#include "mqtt_client.h"

/* Callback for connection. */
void on_connect (struct mosquitto *clnt_inst, void *obj, int ret) {

	printf ("\n");
	printf ("Connect call-back result : %d\n", ret);
}

/* Callback for publish. */
void on_publish (struct mosquitto *clnt_inst, void *obj, int ret) {

	printf ("\n");
	printf ("Publish call-back result : %d\n", ret);

}

/* Callback for messages. */
void on_message (struct mosquitto *clnt_inst, void *obj, const struct mosquitto_message *msg) {

	char *post_obj = NULL;
	int *mid = NULL;
	char *topic = "RESULT";
	char *string = "OOPS!!!";

	printf("Topic :: %s Message Length :: %d Received Message :: %s\n", 
			 msg->topic, msg->payloadlen, (char*) msg->payload);

	if (msg->payloadlen) {
		
		post_obj = (char *)malloc(sizeof(int)*(msg->payloadlen));
		memset( post_obj, '\0', strlen( post_obj));
		
		sprintf( post_obj, "curl -X PUT -d '%s' http://127.0.0.1/restconf/data",
			   	(char *)msg->payload);	
		printf("Data to post to clixon :Length %ld\t%s\n", strlen(post_obj), post_obj);	

		syslog (LOG_INFO, "POSTED DATA :: %s", post_obj);
		
		memset( post_obj, '\0', strlen( post_obj));
		sprintf( post_obj, "Data receieved successfully.");	

		mosquitto_publish ( clnt_inst, NULL, topic, strlen(post_obj), post_obj, 0, false);

        free(post_obj);

	} else {
		//printf("NO MESSAGE RECEIVED.\n");
		//mosquitto_publish (clnt_inst, mid, topic, (int)strlen(post_obj), "NO MESSAGE RECEIVED!!! :(", 0, false);
	}
}

void default_client ( char *server, int port) 
{
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
	printf("Will connect over server : %s , port : %d\n", server, port);

    /* Library Initialization */
	ret = mosquitto_lib_init();
	if (ret == MOSQ_ERR_SUCCESS) {

	    /* Actual CLIENT Instance */
		clnt_inst = mosquitto_new ( id, clean_session, NULL);

		mosquitto_connect_callback_set ( clnt_inst, on_connect);
		mosquitto_message_callback_set ( clnt_inst, on_message);
		mosquitto_publish_callback_set ( clnt_inst, on_publish);

		//ret = mosquitto_connect ( clnt_inst, "localhost", 1883, 100);
		ret = mosquitto_connect ( clnt_inst, server, port, 100);
		if (ret == MOSQ_ERR_SUCCESS) {
			printf ("Connect call SUCCESS.\n");
			printf ("Connected waiting for Published Messages \n");
			ret = mosquitto_subscribe( clnt_inst, NULL, id, qos);
			if (ret == MOSQ_ERR_SUCCESS) {

				printf("mosquitto_connect SUCCESS.\n");	
				ret = mosquitto_loop_forever ( clnt_inst, -1, 1);
			}
		} else if ( ret == MOSQ_ERR_INVAL) {
			printf("Connect call invalid parameters.\n");	
			return;
		} else if ( ret == MOSQ_ERR_ERRNO) {
			printf("Connect call ERROR.\n");	
			return;
		}

		/* Destroy the CLIENT Instance */
		mosquitto_destroy (clnt_inst);			

	}
	
	/* Destroy the CLIENT Instance */
	mosquitto_destroy (clnt_inst);			

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
	int port=0;
	char *server = NULL;

    //while((opt) != -1)
    while((opt = getopt(argc, argv, OPTIONS)) != -1)
	{
		//opt = getopt(argc, argv, options);
		switch (opt) {
		case'd':
			printf("Daemonizing this program.\n");
			daemon_flag=1;
            //daemonize ();
			break;
		case'h':
			usage ();
			exit (EXIT_FAILURE);
		case'p':
			port = atoi(optarg);
			if ( port < 1024) {
				printf ("Port Should not be less than 1024. Exiting...\n");	
				exit (EXIT_FAILURE);
			}
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

	if ( (server != NULL) && (port != 0)) {
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
