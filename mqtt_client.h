/* Include all the header files here. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mosquitto.h>
#include <curl/curl.h>

/* For syslog logging facility. */
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Get Opt long. */
#include <getopt.h>

/* Thread for continuous status update. */
#include <pthread.h>
#include <jansson.h>

/* Support fot TLS based cloud authentication */
#define WITH_TLS

/* Can add options here. */
#define OPTIONS "dhp:s:"

/* For POPULATING IP AND DETAILS */
#define MAX_CMD_SIZE 300
#define MAX_BUF_SIZE 40 

/* For CERTIFICATES */
#define CERT_DIR "/www-data/certs"

/* JINJA command to convert text config to json */
#define TEXT_TO_JSON "python /etc/netconf/text_parser/regular_expression_example.py %s %s"

extern char *srl_no;

/* Structure for thread arguments. */
struct thread_args {

	char *ip;
	int port;
	struct mosquitto *inst;
};
/* For downloading client certificates for TLS connection. */
extern int download_client_certs(char *, int);

/* Will return the serial no of the device. */
char *serial_no(void);

/* Functions Declarations. */
json_t *running_json ( void);
void *status_handler ( void *); 
void daemonize ( char *, int);
char *populate_status_data ( void);
void on_connect (struct mosquitto *, void *, int);
void on_publish (struct mosquitto *, void *, int);
void on_message (struct mosquitto *, void *, const struct mosquitto_message *);
