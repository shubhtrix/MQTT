#include "mqtt_client.h" 

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}

int handle_url (struct MemoryStruct *chunk, char *server, int port)
{
	char *serial = "lcr2";
	CURL *curl_handle;
	CURLcode res;
	char url[200];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

	// Please note port is hardcoded to 81
	sprintf(url, "http://%s:%d/?serialno=%s",server, 81, serial);
	fprintf(stderr,"url is %s\n",url);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl_handle);
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	}
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	return 0;
}

int gen_file(json_t *data, char *file_name)
{
	int fd = 0;
	char file[100];

	memset(file, '\0', sizeof(file));
	sprintf(file, "%s/%s", CERT_DIR,file_name);
	printf("file is %s\n", file);
	fd = open(file, O_CREAT |O_WRONLY);
	if(!fd) 
		return -1;
	printf("crt is %s\n", json_string_value(data));
	write(fd, json_string_value(data), strlen(json_string_value(data))); 
	close(fd);
	return 0;		
}

int check_certs_if_downloaded()
{
	char fname[100];

	sprintf(fname,"%s/%s",CERT_DIR,"ca-crt.pem");
	if(!access( fname, F_OK ) )
		return -1;
	sprintf(fname,"%s/%s",CERT_DIR,"client.key");
	if(!access( fname, F_OK ) )  
		return -1;
	sprintf(fname,"%s/%s",CERT_DIR,"client.crt");
	if(!access( fname, F_OK ) ) 
		return -1;
	return 0;
}

int download_client_certs( char *server, int port) 
{
	struct MemoryStruct chunk;
	json_t *data = NULL, *key = NULL, *crt = NULL, *ca_cert = NULL ;	
	json_error_t error;	

	printf("Server address :: %s and port :: %d\n", server, port);

	fprintf(stderr, "in download_client_certs\n");
	if(check_certs_if_downloaded()) /*if certs are already downloaded then return */	
		return 0;
	chunk.memory = malloc(1);   
	chunk.size = 0;    /* no data at this point */ 
	handle_url(&chunk, server, port);
	data = json_loads(chunk.memory, JSON_DISABLE_EOF_CHECK, &error);
	if(!data) 
		return -1;
	crt = json_object_get(data,"crt");
	if(!crt)
		return -1;
	gen_file(crt, "client.crt");
	key  = json_object_get(data,"key");
	if(!key)
		return -1;
	gen_file(key, "client.key");
	ca_cert  = json_object_get(data,"csr");
	if(!ca_cert)
		return -1;
	gen_file(ca_cert, "ca-crt.pem");
	free(chunk.memory);
	return 0;
}
