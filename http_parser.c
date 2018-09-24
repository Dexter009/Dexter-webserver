#include <stdio.h>
#include <string.h>

#define F_DIR "Content-Type: text/directory\r\n"
#define F_GIF "Content-Type: image/gif\r\n"
#define F_HTML "Content-Type: text/html\r\n"
#define F_JPEG "Content-Type: image/jpeg\r\n"
#define F_JPG "Content-Type: image/jpg\r\n"
#define F_TXT "Content-Type: text/plain\r\n"
#define F_CSS "Content-Type: text/css\r\n"


#define STAT_200 " 200 OK\r\n"
#define STAT_404 " 404 Not Found\r\n"


const char* get_ext(char *file) {
    if (strstr(file, ".gif") != NULL)
        return F_GIF;
    if (strstr(file, ".html") != NULL)
        return F_HTML;
    if (strstr(file, ".jpeg") != NULL)
        return F_JPEG;
    if (strstr(file, ".jpg") != NULL)
        return F_JPG;
    if (strstr(file, ".txt") != NULL)
        return F_TXT;
    if (strstr(file, ".css") != NULL)
        return F_CSS;
}


int recognize_request(const char* req){
	const char req1[4] = "GET";
	const char req2[8] = "POST";
	const char req3[8] = "PUT";
	const char req4[8] = "DELETE";
	char* request;
	request = strstr(req,req1);
	if(request == NULL){
		request = strstr(req,req2);
		if(request == NULL){
			request = strstr(req,req3);
			if(request == NULL){
				return 4;
			}
			else{
				return 3;
			}
		}
		else{
			return 2;
		}
	}
	else{
		return 1;
	}
}

char* recognize_resource(const char* line){
	
	char* version = malloc(sizeof *version);
	version = "HTTP/1.1";
	char* res_status =  malloc(sizeof *res_status);
	char* cont_length = malloc(sizeof *cont_length);
	char* response = malloc(sizeof *response);



	const char *start_of_path = strchr(line, ' ') + 1;


    const char *start_of_query = strchr(start_of_path, '?');
    if(start_of_query == NULL){
    	const char *end_of_query = strchr(start_of_path, ' ');
    	char path[end_of_query - start_of_path];
    	strncpy(path, start_of_path,  end_of_query - start_of_path);
    	path[sizeof(path)] = 0;

    	//lets open the file
    	char url[sizeof(path) + 1];
    	sprintf(url,".%s",path);

	    FILE *fp = fopen(("%s",url), "r");
	    

		if (fp == NULL){
	        printf("Couldn't open file\n");
	        res_status = STAT_404;
	    }
	    else{
	    res_status = STAT_200;
	    
		}

	    fseek(fp, 0, SEEK_END);
		int content_length = ftell(fp);

		const char* ext = malloc(sizeof *ext);
		ext = get_ext(url);
		sprintf(cont_length,"Content-Length: %d\r\n",content_length);

		fseek(fp,0,SEEK_SET);
		char* res = malloc(content_length + 3);
		fread(res, content_length, 1, fp);
		res[content_length+1] = 0;
		char* result = malloc(content_length + 3);
		sprintf(result,"\n\n%s",res);
				fclose(fp);


		printf("starting to debug %s\n",response);
		// strncat(response,version,strlen(version));
		// strncat(response,res_status,strlen(res_status));
		
		// strncat(response,ext,strlen(ext));
		
		// strncat(response,cont_length,strlen(cont_length));
		
		// strncat(response,result,strlen(result));
		sprintf(response,"%s%s%s%s%s", version, res_status, ext, cont_length, result);

		
		return response;

    }
    else{
    	const char *end_of_query = strchr(start_of_query, ' ');
    	char path[start_of_query - start_of_path];
	    char query[end_of_query - start_of_query];

	    /* Copy the strings into our memory */
	    strncpy(path, start_of_path,  start_of_query - start_of_path);
	    strncpy(query, start_of_query, end_of_query - start_of_query);

	    /* Null terminators (because strncpy does not provide them) */
	    path[sizeof(path)] = 0;
	    query[sizeof(query)] = 0;

	    /*Print */
	    printf("%s\n", query, sizeof(query));
	    printf("%s\n", path, sizeof(path));
	    return response;
	}
}