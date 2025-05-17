#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "buffer.h"
#include "helpers.h"
#include "requests.h"
#include "parson.h"

int cookie_find(char *response){
    char *cookie_message = "Set-Cookie";
    size_t cookie_mes_len = strlen(cookie_message);
    size_t length = strlen(response);
    for (size_t i = 0; i < length; i++){
        size_t j;
        for(j = 0; j < cookie_mes_len; j++){
            if (response[i+j] != cookie_message[j]){
                break;
            }
        }
        if(j == cookie_mes_len){
            return i;
        }
    }
    return -1;
}

void cookie_extract(char *response, int pos, char **cookies, int *cookies_count, int *cookies_limit){
    size_t length = strlen(response);
    char *message = response + pos + strlen("Set-Cookie: ");
    int cookie_len = 0;
    for (int i = 0; i < length; i++){
        if(message[i] ==';'){
            break;
        } else {
            cookie_len++;
        }
    }
    char *new_cookie = malloc(sizeof(char) * (cookie_len + 1));
    memcpy(new_cookie, message, cookie_len);
    new_cookie[cookie_len] = '\0';
    if (*cookies_count + 1 <= *cookies_limit){
        cookies[*cookies_count] = new_cookie;
        (*cookies_count)++;
    } else {
        // realloc
    }
}
char *error_message(char *response){
    char *error = "{\"error\":\"";
    size_t error_len = strlen(error);
    size_t length = strlen(response);
    int i = 0;
    for (i = 0; i < length; i++){
        size_t j;
        for(j = 0; j < error_len; j++){
            if (response[i+j] != error[j]){
                break;
            } 
        }
        if (j == error_len){
            char *error_mes = malloc(length - i - error_len);
            memcpy(error_mes, response + i + error_len, length - i - error_len - 3);
            return error_mes;
        }
    }
    return NULL;
}

int response_find(char *message){
    char *http_type = "HTTP/1.1 ";
    size_t http_type_len = strlen(http_type);
    size_t length = strlen(message);
    for (size_t i = 0; i < length; i++){
        size_t j;

        for (j = 0; j < http_type_len; ++j) {
            if (message[i + j] != http_type[j]) {
                break;
            }
        }
        if (j == http_type_len){
            int first = message[j] - '0';
            int second = message[j + 1] - '0';
            int third = message[j + 2] - '0';
            int code = first * 100 + second * 10 + third;
            return code;
        }
    }
    return -1;
}
void login(char *message, int sockfd, char *response, char **cookies, int *cookies_count, int *cookies_size, int login_admin){
    char *admin_username = malloc(sizeof(char) * 100);
    char *username = malloc(sizeof(char) * 100);
    char *password = malloc(sizeof(char) * 100);
    getchar();
    if (!login_admin){
        printf("admin_username=");
        fgets(admin_username, 100, stdin);
        if (strlen(admin_username) != 0)
            admin_username[strlen(admin_username) - 1] = '\0';
        // scanf("%s", admin_username);
        for (int i = 0; i < strlen(admin_username); i++){
            if(admin_username[i] == ' '){
            printf("FAIL: admin username nu are voie sa aiba spatii.\n");
            // free(admin_username);
            // free(username);
            // free(password);
            return;
            }
        }
    }
    printf("username=");
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';
    for (int i = 0; i < strlen(username); i++){
        if(username[i] == ' '){
            printf("FAIL: username nu are voie sa aiba spatii.\n");
            // free(admin_username);
            // free(username);
            // free(password);
            return;
        }
    }
    printf("password=");
    fgets(password, 100, stdin);
    password[strlen(password) - 1] = '\0';
    // printf("trec de citire\n");
    // creez json-ul
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    if (!login_admin){
        json_object_set_string(root_object, "admin_username", admin_username);
    }
    
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);
    // printf("fac serialized string\n");
    // sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    char *body_data[1];
    body_data[0] = serialized_string;
    // printf("fac body data: %s\n", body_data[0]);
    if (!login_admin){
        // printf("asta cu mesaju");
        char * message2 = compute_post_request("63.32.125.183", "/api/v1/tema/user/login", NULL, "application/json", body_data, 1, cookies, *cookies_count);
        // printf("fac message\n");
        send_to_server(sockfd, message2);
        // printf("am trimis la server\n");
        char *response2 = receive_from_server(sockfd);
        // printf("RESPONSE: %s\n", response2);
        int pos = cookie_find(response2);
        if (pos != -1){
            cookie_extract(response2, pos, cookies, cookies_count, cookies_size);
        }
        int code = response_find(response2);
        // printf("code: %d\n", code);
        if (code == 200){
            printf("SUCCES: 200 - OK\n");
        } else if(code / 100 == 4){
            printf("eroare: %s\n", error_message(response2));
        }
    } else {
        char *message2 = compute_post_request("63.32.125.183", "/api/v1/tema/admin/login", NULL, "application/json", body_data, 1, cookies, *cookies_count);
    
        // printf("fac message\n");
        send_to_server(sockfd, message2);
        // printf("am trimis la server\n");
        response = receive_from_server(sockfd);
        // printf("RESPONSE: %s\n", response);
        int pos = cookie_find(response);
        if (pos != -1){
            cookie_extract(response, pos, cookies, cookies_count, cookies_size);
        }
        int code = response_find(response);
        // printf("code: %d\n", code);
        if (code == 200){
            printf("SUCCES: 200 - OK\n");
        } else if(code / 100 == 4){
            printf("eroare: %s\n", error_message(response));
        }
    }
    // json_free_serialized_string(serialized_string);
    // json_value_free(root_value);
    // free(admin_username);
    // free(username);
    // free(password);
}
void add_user(char *message, int sockfd, char *response, char **cookies, int *cookies_count, int *cookies_size){
    char *username = malloc(sizeof(char) * 100);
    char *password = malloc(sizeof(char) * 100);
    getchar();
    printf("username=");
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';
    for (int i = 0; i < strlen(username); i++){
        if(username[i] == ' '){
            printf("FAIL: username nu are voie sa aiba spatii.\n");
            free(username);
            free(password);
            return;
        }
    }
    printf("password=");
    fgets(password, 100, stdin);
    password[strlen(password) - 1] = '\0';
    // fac json-ul
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);
    char *body_data[1];
    body_data[0] = serialized_string;
    // transmit mesajul
    message = compute_post_request("63.32.125.183", "/api/v1/tema/admin/users", NULL, "application/json", body_data, 1, cookies, *cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // caut cookie-uri
    // int pos = cookie_find(response);
    // if (pos != -1)
        // cookie_extract(response, pos, cookies, cookies_count, cookies_size);
    int code = response_find(response);
    // afisez rezultatul
    if (code == 201){
        printf("SUCCESS: Utilizator adaugat cu succes\n");
    }
    if(code / 100 == 4){
        printf("eroare: %s\n", error_message(response));
    }    
    free(username);
    free(password);
}
void get_users(char *message, int sockfd, char *response, char **cookies, int *cookies_count, int *cookies_size){
    message = compute_get_request("63.32.125.183", "/api/v1/tema/admin/users", NULL, cookies, *cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // int pos = cookie_find(response);
    // if (pos != -1)
        // cookie_extract(response, pos, cookies, cookies_count, cookies_size);
    int code = response_find(response);
    if (code == 200){
        // printf("SUCCES: Lista utilizatorilor:\n");
        // get_user_list(response)
        size_t length = strlen(response);
        int i = 0;
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        JSON_Value *root_value = json_parse_string(response + i);
        JSON_Object *root_object = json_value_get_object(root_value);
        JSON_Array *users_array = json_object_get_array(root_object, "users");

        size_t count = json_array_get_count(users_array);
        for (size_t i = 0; i < count; i++) {
            JSON_Object *user = json_array_get_object(users_array, i);
            const char *username = json_object_get_string(user, "username");
            const char *password = json_object_get_string(user, "password");
            printf("#%ld %s: %s\n",i, username, password);
        }
    }
    if(code / 100 == 4){
        printf("eroare: %s\n", error_message(response));
    }
}
void delete_user(char *message, int sockfd, char *response, char **cookies, int cookies_count){
    char *username = malloc(sizeof(char) * 100);
    getchar();
    printf("username=");
    // scanf("%s", username);
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';

    for(int i = 0; i < strlen(username); i++){
        if (username[i] == ' '){
            printf("ERROR: Username-ul nu are voie sa aiba spatii\n");
            free(username);
            return;
        }
    }

    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/admin/users/");
    strcat(path, username);
    message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Utilizator sters\n");
    }
    if(code / 100 == 4){
        printf("eroare: %s\n", error_message(response));
    }
    free(username);
}

void get_movie(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    char *id = malloc(sizeof(char) * 10);
    getchar();
    printf("id=");
    scanf("%s", id);
    char *url = calloc(100, 1);
    strcat(url, "/api/v1/tema/library/movies/");
    strcat(url, id);
    // printf("url: %s", url);
    message = compute_get_request("63.32.125.183", url, JWT_token, cookies, cookies_count);

    int sockfd2 = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);

    send_to_server(sockfd2, message);
    response = receive_from_server(sockfd2);
    close_connection(sockfd2);
    // printf("RESPONSE: %s\n", response);
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Detalii film\n");
        size_t length = strlen(response);
        int i = 0;
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        JSON_Value *root_value = json_parse_string(response + i);
        JSON_Object *movie = json_value_get_object(root_value);

        const char *title = json_object_get_string(movie, "title");
        const char *rating = json_object_get_string(movie, "rating");
        const char *description = json_object_get_string(movie, "description");
        int year = json_object_get_number(movie, "year");
        printf("title: %s\n", title);
        printf("year: %d\n", year);
        printf("description: %s\n", description);
        printf("rating: %s\n", rating);
    } else if (code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
}
void add_movie(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    // char title[100], description[100];
    char *title = malloc(sizeof(char) * 100);
    char *description = malloc(sizeof(char) * 100);
    int year;
    float rating;
    getchar();

    printf("title=");
    fgets(title, 100, stdin);
    if (title[strlen(title) - 1] == '\n') title[strlen(title) - 1] = '\0';

    printf("year=");
    scanf("%d", &year);
    getchar();

    printf("description=");
    fgets(description, 100, stdin);
    if (description[strlen(description) - 1] == '\n') description[strlen(description) - 1] = '\0';

    printf("rating=");
    scanf("%f", &rating);
    if (rating > 9.9){
        printf("ERROR: rating-ul nu poate fi peste 9.9.\n");
        free(title);
        free(description);
        // exit(-1);
        return;
    }

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "title", title);
    json_object_set_number(root_object, "year", year);
    json_object_set_string(root_object, "description", description);
    json_object_set_number(root_object, "rating", rating);
    serialized_string = json_serialize_to_string_pretty(root_value);
    char *body_data[1];
    body_data[0] = serialized_string;
    message = compute_post_request("63.32.125.183", "/api/v1/tema/library/movies",JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // printf("RESPONSE: %s\n", response);
    int code = response_find(response);
    if (code == 201){
        printf("SUCCESS: Detalii film\n");
        printf("title: %s\n", title);
        printf("year: %d\n", year);
        printf("description: %s\n", description);
        printf("rating: %f\n", rating);
    }
    free(title);
    free(description);
}

void delete_movie(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    char *id = malloc(sizeof(char) * 20);
    printf("id=");
    scanf("%s", id);

    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/library/movies/");
    strcat(path, id);

    message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Movie sters\n");
    }  else if(code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(id);
    free(path);
}
void logout_admin(char *message, int sockfd, char *response, char **cookies, int *cookies_count, int *cookies_size, char *JWT_token){
    message = compute_get_request("63.32.125.183", "/api/v1/tema/admin/logout", NULL, cookies, *cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 200){
        free(cookies[*cookies_count]);
        (*cookies_count)--;
        memset(JWT_token, 0, 200);
        printf("SUCCES: 200 - OK\n");
    }
    else if (code == 403){
        // printf("eroare: %s\n", error_message(response));
    }
}
void logout(char *message, int sockfd, char *response, char **cookies, int *cookies_count, int *cookies_size, char *JWT_token){
    message = compute_get_request("63.32.125.183", "/api/v1/tema/user/logout", NULL, cookies, *cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 200){
        free(cookies[*cookies_count]);
        (*cookies_count)--;
        memset(JWT_token, 0, 200);
        printf("SUCCES: 200 - OK\n");
    } else if(code == 403){
        printf("eroare: %s\n", error_message(response));
    }
}
void JWT_token_extract(char *response, char *JWT_token){
    size_t length = strlen(response);
    char *token = "token";
    size_t token_size = strlen(token);
    int i = 0;
    for (i = 0; i < length; i++){
        size_t j;
        for(j = 0; j < token_size; j++){
            if (response[i+j] != token[j]){
                break;
            }   
        }
        if (j == token_size){
            memcpy(JWT_token, response + i + token_size + 3, length - i - token_size - 6);
            JWT_token[length - i - token_size - 6] = '\0';
            // printf("TOKENUL:%s\n", JWT_token);
            return;
        }
    }
}
void get_access(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    message = compute_get_request("63.32.125.183", "/api/v1/tema/library/access", NULL, cookies, cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    size_t length = strlen(response);
    char *token = "token";
    size_t token_size = strlen(token);
    int i = 0;
    for (i = 0; i < length; i++){
        size_t j;
        for(j = 0; j < token_size; j++){
            if (response[i+j] != token[j]){
                break;
            }   
        }
        if (j == token_size){
            memcpy(JWT_token, response + i + token_size + 3, length - i - token_size - 6);
            JWT_token[length - i - token_size - 6] = '\0';
            break;
        }
    }
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Token JWT primit\n");
    } else if(code == 403){
        printf("eroare: %s\n", error_message(response));
    }
}
void get_movies(char *message, int sockfd, char *response, char *JWT_token){
    message = compute_get_request("63.32.125.183", "/api/v1/tema/library/movies", JWT_token, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 200){
        size_t length = strlen(response);
        int i = 0;
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        JSON_Value *root_value = json_parse_string(response + i);
        JSON_Object *root_object = json_value_get_object(root_value);
        JSON_Array *users_array = json_object_get_array(root_object, "movies");

        size_t count = json_array_get_count(users_array);
        printf("SUCCESS: Lista filmelor\n");
        for (size_t i = 0; i < count; i++) {
            JSON_Object *user = json_array_get_object(users_array, i);
            const char *title = json_object_get_string(user, "title");
            int id = json_object_get_number(user, "id");
            printf("#%d %s\n",id, title);
        }
    } else if(code == 403){
        printf("ERROR: %s\n", error_message(response));
    }
}

void update_movie(char *message,int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    // char title[100], description[100];
    char *title = malloc(sizeof(char) * 100);
    char *description = malloc(sizeof(char) * 100);
    int year;
    float rating;
    getchar();
    char *id = malloc(sizeof(char) * 10);
    printf("id=");
    scanf("%s", id);
    getchar();

    printf("title=");
    fgets(title, 100, stdin);
    if (title[strlen(title) - 1] == '\n') title[strlen(title) - 1] = '\0';

    printf("year=");
    scanf("%d", &year);
    getchar();

    printf("description=");
    fgets(description, 100, stdin);
    if (description[strlen(description) - 1] == '\n') description[strlen(description) - 1] = '\0';

    printf("rating=");
    scanf("%f", &rating);
    if (rating > 9.9){
        printf("ERROR: Rating-ul nu poate fi mai mare decat 9.9.\n");
        return;
    }

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "title", title);
    json_object_set_number(root_object, "year", year);
    json_object_set_string(root_object, "description", description);
    json_object_set_number(root_object, "rating", rating);
    serialized_string = json_serialize_to_string_pretty(root_value);
    char *body_data[1];
    body_data[0] = serialized_string;

    char *url = calloc(100, 1);
    strcat(url, "/api/v1/tema/library/movies/");
    strcat(url, id);
    message = compute_put_request("63.32.125.183", url,JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    // printf("message: %s\n", message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // printf("RESPONSE: %s\n", response);
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Film actualizat\n");
    }
    if (code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(id);
    free(url);
    free(title);
    free(description);
}
void get_collections(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    message = compute_get_request("63.32.125.183", "/api/v1/tema/library/collections", JWT_token, cookies, cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    // printf(response);
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Lista colectiilor\n");
        size_t length = strlen(response);
        int i = 0;
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        JSON_Value *root_value = json_parse_string(response + i);
        JSON_Object *root_object = json_value_get_object(root_value);
        JSON_Array *collections_array = json_object_get_array(root_object, "collections");

        size_t count = json_array_get_count(collections_array);
        for (size_t i = 0; i < count; i++) {
            JSON_Object *collection = json_array_get_object(collections_array, i);
            const char *title = json_object_get_string(collection, "title");
            int id = json_object_get_number(collection, "id");
            printf("#%d: %s\n",id, title);
        }
    } else if (code / 100 == 4){
        printf("eroare: %s\n", error_message(response));
    }
}
void get_collection(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    char *id = malloc(sizeof(char) * 10);
    printf("id=");
    scanf("%s", id);

    char *url = calloc(100, 1);
    strcat(url, "/api/v1/tema/library/collections/");
    strcat(url, id);

    message = compute_get_request("63.32.125.183", url, JWT_token, cookies, cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Detalii colectie\n");
        char *json = strchr(response, '{');
        JSON_Value *root = json_parse_string(json);
        JSON_Object *root_obj = json_value_get_object(root);
        // int id = (int)json_object_get_number(root_obj, "id");
        const char *title = json_object_get_string(root_obj, "title");
        const char *owner = json_object_get_string(root_obj, "owner");
        printf("title: %s\n", title);
        printf("owner:%s\n", owner);
        printf("movies:\n");

        JSON_Array *movies = json_object_get_array(root_obj, "movies");
        int cnt = json_array_get_count(movies);
        for (int i = 0; i < cnt; i++){
            JSON_Object *movie_obj = json_array_get_object(movies, i);
            int id = (int)json_object_get_number(movie_obj, "id");
            const char *movie_title = json_object_get_string(movie_obj, "title");
            printf("#%d: %s\n", id, movie_title);
        }
    } else if (code / 100 == 4){    
        printf("ERROR: %s\n", error_message(response));
    }
}

int add_movie_to_collection(char *message, int sockfd,char * response, char **cookies, int cookies_count, char *JWT_token, char *collection, int id_movie, int add_col){
    int id;
    if (!add_col){
        collection = malloc(sizeof(char) * 100);
        printf("collection_id=");
        scanf("%s", collection);
        printf("movie_id=");
        scanf("%d", &id);
        getchar();
    } else {
        id = id_movie;
    }
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_number(root_object, "id", id);
    serialized_string = json_serialize_to_string_pretty(root_value);
    char *body_data[1];
    body_data[0] = serialized_string;
    char *url = calloc(100, 1);

    strcat(url, "/api/v1/tema/library/collections/");
    strcat(url, collection);
    strcat(url, "/movies");

    message = compute_post_request("63.32.125.183", url, JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 201){
        if (!add_col){
            printf("SUCCESS: FIlm adaugat in colectie\n");
        }
        return 1;
    }
    return 0;
}
int delete_collection(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token, char *col){
    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/library/collections/");
    if (col == NULL){
        char *id = malloc(sizeof(char) * 20);
        printf("id=");
        scanf("%s", id);
        strcat(path, id);
    } else {
        strcat(path, col);
    }
    message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, JWT_token);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    
    if (code == 200){
        if (col == NULL){
            printf("SUCCESS: colectie stearsa\n");
        }
        return 1;
    } else {
        // if (col == NULL){
            printf("ERROR: %s\n", error_message(response));
        // }
        return 0;
    }
    return 0;
}
void add_collection(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    // char title[100];
        char *title = malloc(sizeof(char) * 100);

    getchar();
    printf("title=");
    fgets(title, 100, stdin);
    if (title[strlen(title) - 1] == '\n') title[strlen(title) - 1] = '\0';
    int num_movies;
    printf("num_movies=");
    scanf("%d", &num_movies);
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "title", title);
    json_object_set_number(root_object, "num_movies", num_movies);

    int *num_id = malloc(sizeof(int) * num_movies);
    for (int i = 0; i < num_movies; i++){
        printf("movie_id[%d]=", i);
        scanf("%d", &num_id[i]);
    }
    message = NULL;
    response = NULL;

    serialized_string = json_serialize_to_string_pretty(root_value);
    char *body_data[1];
    body_data[0] = serialized_string;
    message = compute_post_request("63.32.125.183", "/api/v1/tema/library/collections", JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    int code = response_find(response);
    if (code == 201){
        printf("SUCCESS: Colectie adaugata. acum adaug filmele\n");
        char *json = strchr(response, '{');
        JSON_Value *root = json_parse_string(json);
        JSON_Object *root_obj = json_value_get_object(root);
        int id = (int)json_object_get_number(root_obj, "id");
        char *collection_id_string = malloc(sizeof(char) * 10);
        sprintf(collection_id_string, "%d", id);
        for(int i = 0; i < num_movies; i++){
            int success = add_movie_to_collection(message, sockfd, response, cookies, cookies_count, JWT_token, collection_id_string, num_id[i], 1);
            if (success != 1){
                int del = delete_collection(message, sockfd, response, cookies, cookies_count, JWT_token, collection_id_string);
                if (del == 1){
                    printf("ERROR: eroare la adaugare \n");
                }
                return;
            }
        }
        free(collection_id_string);
        printf("SUCCESS: Colectie adaugata\n");
    }
    free(title);
}

void delete_movie_from_collection(char *message, int sockfd, char *response, char **cookies, int cookies_count, char *JWT_token){
    char *collection_id = malloc(sizeof(char) * 20);
    printf("collection_id=");
    scanf("%s", collection_id);
    // getchar();

    char *movie_id = malloc(sizeof(char) * 20);
    printf("movie_id=");
    scanf("%s", movie_id);
    // getchar();

    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/library/collections/");
    strcat(path, collection_id);

    strcat(path, "/movies/");
    strcat(path, movie_id);
    message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    free(collection_id);
    free(movie_id);
}
int main(int argc, char *argv[])
{
    char *message = NULL;
    char *response = NULL;
    int sockfd;

    char *command = malloc(sizeof(char) * 100);
    int running = 1;
    int cookies_size = 10;
    char **cookies = malloc(10 * sizeof(char *));

    int cookies_count = 0;
    sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    char *JWT_token = calloc(200, sizeof(char));

    while(running){
        scanf("%s", command);
        if (strcmp(command, "exit") == 0){
            memset(JWT_token, 0, 200);
            running = 0;
        } else if (strcmp(command, "login") == 0){
            login(message, sockfd, response, cookies, &cookies_count, &cookies_size, 0);
        }
        else if (strcmp(command, "login_admin") == 0){
            login(message, sockfd, response, cookies, &cookies_count, &cookies_size, 1);
        }
        else if(strcmp(command, "add_user") == 0){
            add_user(message, sockfd, response, cookies, &cookies_count, &cookies_size);

        }
        else if(strcmp(command, "get_users") == 0){
            get_users(message, sockfd, response, cookies, &cookies_count, &cookies_size);
        }
        else if (strcmp(command, "delete_user") == 0){
            delete_user(message, sockfd, response, cookies, cookies_count);
        }
        else if (strcmp(command, "logout") == 0){
            logout(message, sockfd, response, cookies, &cookies_count, &cookies_size, JWT_token);
        } else if(strcmp(command, "logout_admin") == 0){
            logout_admin(message, sockfd, response, cookies, &cookies_count, &cookies_size, JWT_token);
        } else if(strcmp(command, "get_access") == 0){
            get_access(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_movie") == 0){
            get_movie(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_movies") == 0){
            get_movies(message, sockfd, response, JWT_token);
            
        } else if(strcmp(command, "add_movie") == 0){
            add_movie(message, sockfd,response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "delete_movie") == 0){
            delete_movie(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "update_movie") == 0){
            update_movie(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_collections") == 0){
            get_collections(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_collection") == 0){
            get_collection(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "add_collection") == 0){
            add_collection(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "delete_collection") == 0){
            delete_collection(message, sockfd, response, cookies, cookies_count, JWT_token, NULL);
        } else if(strcmp(command, "add_movie_to_collection") == 0){
            add_movie_to_collection(message, sockfd, response, cookies, cookies_count, JWT_token, NULL, 0, 0);
        } else if(strcmp(command, "delete_movie_from_collection") == 0){
            delete_movie_from_collection(message, sockfd, response, cookies, cookies_count, JWT_token);
        } else {
            printf("Invalid command!\n");
        }
        memset(command, 0, 100);
    }
    return 0;
}
