/* Include all the header files here. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto_internal.h>

/* For syslog logging facility. */
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Get Opt long. */
#include <getopt.h>

/* Can add options here. */
#define OPTIONS "dhp:s:"

/* Functions Declarations. */
void daemonize ( char *, int );
void on_connect (struct mosquitto *, void *, int);
void on_publish (struct mosquitto *, void *, int);
void on_message (struct mosquitto *, void *, const struct mosquitto_message *);
