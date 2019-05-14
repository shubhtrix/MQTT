#include "mqtt_client.h"

/* Will return json object of current running config. */
json_t *running_json () {

	int ret;
	char *command;
	json_t *run_config;
	json_error_t error;

	command = (char *)malloc(500);
	memset(command,'\0',500);

	ret = system("konf -- -d -r > /tmp/.running-config");
	if (ret) {
		printf("Running config writing failed :: %ld.\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	sprintf( command, TEXT_TO_JSON, "/tmp/.running-config", "/tmp/.running.json");
	ret = system( command);
	if (ret) {
		printf("Running config JSON creation failed :: %ld.\n", __LINE__);
		exit (EXIT_FAILURE);
	}
	
	run_config=json_load_file ("/tmp/.running.json", 0, &error);
	if (!run_config) {
		printf("Json creation failed at %ld.\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	free(command);
	
	return run_config;
}

/* Callback for messages. Actual JSON configuration response.
 * On GET, PUT 													*/
void on_message (struct mosquitto *clnt_inst, void *obj, const struct mosquitto_message *msg) {

	char *post_obj = NULL;
	int *mid = NULL;
	int ret;
	json_t *run_json;
	FILE *fptr;
	char *line = NULL;
	char *running = NULL;
	char *pub_topic = "NULL";
	size_t len=0, read;

	printf("Topic :: %s Message Length :: %d Received Message :: %s\n", 
			 msg->topic, msg->payloadlen, (char*) msg->payload);

	pub_topic = (char *)malloc ((sizeof(char)*100)+(strlen(srl_no)));
	sprintf( pub_topic, "devices/lcr/%s/run-config", srl_no);	

	if (strstr(msg->topic, "/get")) {

		run_json = running_json();
		running = json_dumps(run_json, JSON_COMPACT);
		mosquitto_publish ( clnt_inst, NULL, pub_topic, strlen(running), running, 0, false);

	} else if (strstr(msg->topic, "/put")) {

		if (msg->payloadlen) {

			post_obj = (char *)malloc((sizeof(char)*500)+(strlen((char *)msg->payload)));
			sprintf( post_obj, "curl -X PUT -d '%s' http://127.0.0.1/restconf/data",
							   	(char *)msg->payload);	

			syslog (LOG_INFO, "POSTED DATA :: %s", post_obj);
			mosquitto_publish ( clnt_inst, NULL, pub_topic, strlen(post_obj), post_obj, 0, false);

			ret = system( post_obj);
			if(ret) {
				printf("Failed at system call for curl request to system.\n");
				exit ( EXIT_FAILURE);
			}

			free(post_obj);

		} else {

			mosquitto_publish (clnt_inst, mid, pub_topic, (int)strlen(post_obj), "EMPTY OBJECT!!!", 0, false);
		}
	}
	free (pub_topic);
}
