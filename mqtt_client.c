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
		
		//post_obj = (char *)malloc(sizeof(char)*101);
		post_obj = (char *)malloc(sizeof(int)*(msg->payloadlen));
		memset( post_obj, '\0', strlen( post_obj));
		
		sprintf( post_obj, "curl -X PUT -d '%s' http://127.0.0.1/restconf/data",
			   	(char *)msg->payload);	
		printf("Data to post to clixon :Length %ld\t%s\n", strlen(post_obj), post_obj);	

		//syslog (LOG_INFO, "POSTED DATA :: %s", post_obj);
		
		memset( post_obj, '\0', strlen( post_obj));
		sprintf( post_obj, "Data receieved successfully.");	

		mosquitto_publish ( clnt_inst, NULL, topic, strlen(post_obj), post_obj, 0, false);

        free(post_obj);

	} else {
		//printf("NO MESSAGE RECEIVED.\n");
		//mosquitto_publish (clnt_inst, mid, topic, (int)strlen(post_obj), "NO MESSAGE RECEIVED!!! :(", 0, false);
	}
}

void daemonize (void )
{
	pid_t PID, SID;
	FILE *LOG_FILE;

    int ret, qos=1;
	char *id = "NETCONF";

    // true to tell the boker to clean all messages on disconnect
	// false to keep them under function mosquitto_new
	bool clean_session = true;

	int major=0, minor=0, revision=0;
	struct mosquitto *clnt_inst = NULL;
	struct mosq_config *cfg = NULL;

	/* Library Versioning */	
	ret = mosquitto_lib_version( &major, &minor, &revision);

	printf("Mosquitto Library Version : %d.%d.%d\n", major, minor, revision);
   
    /* Library Initialization */
	ret = mosquitto_lib_init();
	if (ret == MOSQ_ERR_SUCCESS) {

	    /* Actual CLIENT Instance */
		clnt_inst = mosquitto_new ( id, clean_session, NULL);

		mosquitto_connect_callback_set ( clnt_inst, on_connect);
		mosquitto_message_callback_set ( clnt_inst, on_message);

		ret = mosquitto_connect (clnt_inst, "localhost", 1883, 100);
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

void default_client (void) 
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

    /* Library Initialization */
	ret = mosquitto_lib_init();
	if (ret == MOSQ_ERR_SUCCESS) {

	    /* Actual CLIENT Instance */
		clnt_inst = mosquitto_new ( id, clean_session, NULL);

		mosquitto_connect_callback_set ( clnt_inst, on_connect);
		mosquitto_message_callback_set ( clnt_inst, on_message);
		mosquitto_publish_callback_set ( clnt_inst, on_publish);

		//ret = mosquitto_connect ( clnt_inst, "localhost", 1883, 100);
		ret = mosquitto_connect ( clnt_inst, "172.16.1.105", 1883, 100);
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

void main(int argc, char **argv) 
{
	int opt;
	char *options = "hd";     // Can add options here.

    while(opt != -1)
	{
		opt = getopt(argc, argv, options);
		switch (opt) {
		case'h': 
			printf("Subscriber MQTT client.\n");
			printf("Options ::\t-h : help.\n");
			printf("\t\t-d : Daemonize the process.\n");
			return;
		case'd':
			printf("Daemonizing this program.\n");
            daemonize ();
			return;
		case'?':
			printf("Invalid option. Need some help.\n");
			printf("Subscriber MQTT client.\n");
			printf("Options ::\t-h : help.\n");
			printf("\t\t-d : Daemonize the process.\n");
			return;
        default : 
            default_client ();
			return;
		}
	}

	return;
}
