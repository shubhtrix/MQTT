#include "mqtt_client.h"

char *serial_no () {

	FILE *fp = NULL;
	char *line = NULL;
	size_t len=0, read;

	fp = popen("cat /etc/system_key" , "r");
	if ( fp == NULL) {
		printf("Popen failed at line :: %ld.\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		strtok( line,"\n");
		printf("SERIAL NO. : %s\n", line);
	}
	pclose ( fp);
	return line;
}

void populate_ip_addr (json_t *iface_array) {

	FILE *fptr;
	size_t index;					//For json population
	json_t *value;
	char *line = NULL;
	char *command=NULL;

	size_t len=0, read;

	command=(char *)malloc ( MAX_CMD_SIZE);
	
	if ( json_array_size ( iface_array)) {
		
		//printf( "Array size : %ld\n", json_array_size ( iface_array));
		json_array_foreach( iface_array, index, value) {

			memset ( command,'\0', MAX_CMD_SIZE);
			sprintf( command, "ifconfig %s | grep -w 'UP' | awk '{print $1}'",
								json_string_value( json_object_get( value, "intf_name")));	
			
			fptr = popen( command, "r");
			if ( fptr == NULL) {
				printf("Popen failed at line :: %ld.\n", __LINE__);
				exit (EXIT_FAILURE);
			}

			while ((read = getline(&line, &len, fptr)) != -1)
				json_object_set_new ( value, "link", json_string("up"));
			
			memset ( command,'\0', MAX_CMD_SIZE);
			sprintf( command, "ifconfig %s | grep -w 'inet' | tail -1 | awk '{print $2}' | cut -d':' -f2", 
						json_string_value( json_object_get( value, "intf_name")));	
			pclose ( fptr);

			fptr = popen( command, "r");
			if ( fptr == NULL) {
				printf("Popen failed at line :: %ld.\n", __LINE__);
				exit (EXIT_FAILURE);
			}

			while ((read = getline(&line, &len, fptr)) != -1) {

				strtok( line,"\n");
				json_object_set_new ( value, "ip", json_string( line));
			}
			
			memset ( command,'\0', MAX_CMD_SIZE);
			sprintf( command, "ifconfig %s | grep -w 'inet' | tail -1 | awk '{print $4}' | cut -d':' -f2", 
						json_string_value( json_object_get( value, "intf_name")));	
			pclose ( fptr);

			fptr = popen( command, "r");
			if ( fptr == NULL) {
				printf("Popen failed at line :: %ld.\n", __LINE__);
				exit (EXIT_FAILURE);
			}

			while (( read = getline(&line, &len, fptr)) != -1) {

				strtok( line,"\n");
				json_object_set_new ( value, "netmask", json_string( line));
			}
			
			memset ( command,'\0', MAX_CMD_SIZE);
			sprintf( command, "ethtool %s | grep -w Speed | awk '{print $2}'", 
						json_string_value( json_object_get( value, "intf_name")));	
			pclose ( fptr);

			fptr = popen( command, "r");
			if ( fptr == NULL) {
				printf("Popen failed at line :: %ld.\n", __LINE__);
				exit (EXIT_FAILURE);
			}

			while (( read = getline ( &line, &len, fptr)) != -1) {

				strtok(line,"\n");
				json_object_set_new ( value, "speed", json_string( line));
			}
			
			pclose ( fptr);
			
			if ( json_string_value ( json_object_get( value, "link")) == NULL)
				json_object_set_new ( value, "link", json_string("down"));
		}
	}

	free (line);
	free (command);
}

char *populate_status_data () {
	
	int json_type, i;
	char *data=NULL;
	char *buffer = NULL;
	FILE *fp;
	size_t len, read;
	char *iface = NULL, *ip = NULL, *mask = NULL;


	json_t *iface_array = NULL, *obj_new = NULL, *obj_new1 = NULL;

	fp = popen( "ip addr | cut -d' ' -f2 | xargs", "r");
	if (fp == NULL) {
		printf("Popen failed at line :: %ld.\n", __LINE__);
		exit (EXIT_FAILURE);
	}

	obj_new = json_object();         //  Final JSON_OBJECT for complete config.
	iface_array = json_array(); 	 //  Interface array

	while((read=getdelim(&buffer,&len,58,fp))!= -1)
	{
		if (!strcmp(buffer,"\n"))
			continue;
		buffer = strtok(buffer,":");
		buffer = strtok(buffer," ");
		buffer = strtok(buffer,"@");
		obj_new1 = json_object ();

		json_object_set_new ( obj_new1, "intf_name", json_string(buffer));
		json_array_append_new (iface_array, obj_new1);
	}

	populate_ip_addr ( iface_array);

	json_object_set_new ( obj_new, "interfaces", iface_array);

	data = json_dumps( obj_new, JSON_COMPACT);

	pclose ( fp);
	return data;
}

void *status_handler ( void *thrd_args) {

	char *device_data=NULL;
	char *pub_status_topic = "NULL";

	json_t *json;
	json_error_t error;
	struct mosquitto *thrd_inst = NULL;
	
	struct thread_args *details = (struct thread_args *)thrd_args;

	printf("IP : %s\tPORT : %d\n", details->ip, details->port);	

	pub_status_topic = (char *)malloc ((sizeof(char)*100)+(strlen(srl_no)));
	sprintf( pub_status_topic, "devices/lcr/%s/lcr-status", srl_no);	

	mosquitto_publish ( details->inst, NULL, pub_status_topic, 
					strlen( "STATUS DETAILS"), "STATUS DETAILS", 0, false);

	while (1) {
	
		// Populate the data to some buffer before actual publishing.	
		device_data = populate_status_data();
			
		mosquitto_publish( details->inst, NULL, pub_status_topic, 
							strlen(device_data), device_data, 0, false);
		free (device_data);
		sleep ( 30);
	}
	return NULL;
}
