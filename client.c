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
// functie ca sa gasesc pozitia la care se afla cookie-ul in response
int cookie_find(char *response){
    char *cookie_message = "Set-Cookie";
    size_t cookie_mes_len = strlen(cookie_message);
    // parcurg tot response-ul si verific daca mesajul corespunde cu mesajul "Set-cookie"
    // ca sa vad daca am ajuns la partea cu json si ca
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
// functie ca sa extrag cookie-ul si sa il pun in vector
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
    }
}
// functie ca sa extrag mesajul de eroare, caut string-ul "{"error":" ca sa 
// vad pozitia de inceput si sa pot sa dau malloc si memcpy
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
// caut codul de raspuns in mesajul returnat
// caut string-ul "HTTP/1.1" pt ca pe linia aia se afla codul
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
            // extrag caracter cu caracter si transform in int
            // adun numerele si returnez codul
            int first = message[j] - '0';
            int second = message[j + 1] - '0';
            int third = message[j + 2] - '0';
            int code = first * 100 + second * 10 + third;
            return code;
        }
    }
    return -1;
}
// am creat o singura functie pentru login si login_admin, mai trimit 
// un extra parametru pentru a vedea ce functie apelez, daca e 0, fac login,
// iar daca e 1, fac login_admin si nu mai afisez si partea cu 
// "admin_username="
void login(char **cookies, int *cookies_count, int *cookies_size, int login_admin){
    char *admin_username = malloc(sizeof(char) * 100);
    char *username = malloc(sizeof(char) * 100);
    char *password = malloc(sizeof(char) * 100);
    getchar();
    // citesc datele:
    if (!login_admin){
        printf("admin_username=");
        fgets(admin_username, 100, stdin);
        if (strlen(admin_username) != 0)
            admin_username[strlen(admin_username) - 1] = '\0';
        for (int i = 0; i < strlen(admin_username); i++){
            if(admin_username[i] == ' '){
                printf("FAIL: admin username nu are voie sa aiba spatii.\n");
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
            return;
        }
    }
    printf("password=");
    fgets(password, 100, stdin);
    password[strlen(password) - 1] = '\0';
    // creez json-ul:
    JSON_Value *json_val = json_value_init_object();
    JSON_Object *json_obj = json_value_get_object(json_val);
    char *serialized_string = NULL;
    // adaug datele in json
    if (!login_admin){
        json_object_set_string(json_obj, "admin_username", admin_username);
    }
    
    json_object_set_string(json_obj, "username", username);
    json_object_set_string(json_obj, "password", password);
    serialized_string = json_serialize_to_string_pretty(json_val);
    char *body_data[1];
    body_data[0] = serialized_string;
    // fac request-ul si transmit cererea
    char *message;
    if (!login_admin){
        message = compute_post_request("63.32.125.183", "/api/v1/tema/user/login", NULL, "application/json", body_data, 1, cookies, *cookies_count);
    } else {
        message = compute_post_request("63.32.125.183", "/api/v1/tema/admin/login", NULL, "application/json", body_data, 1, cookies, *cookies_count);
    }
    // deschid conexiunea
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    // caut cookie-ul si daca il gasesc, adaug in vectorul de cookie-uri
    int pos = cookie_find(response);
    if (pos != -1){
        cookie_extract(response, pos, cookies, cookies_count, cookies_size);
    }
    // caut codul si afisez succes sau eroare
    int code = response_find(response);
    if (code == 200){
        printf("SUCCES: 200 - OK\n");
    } else if(code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(admin_username);
    free(username);
    free(password);
    free(response);
    free(message);
}
void add_user(char **cookies, int cookies_count){
    // citesc datele
    char *username = malloc(sizeof(char) * 100);
    char *password = malloc(sizeof(char) * 100);
    getchar();
    printf("username=");
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';
    // verific daca username-ul are spatii pt ca nu are voie 
    // afisez fail si dau return daca nu e bine
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
    JSON_Value *json_val = json_value_init_object();
    JSON_Object *json_obj = json_value_get_object(json_val);
    char *serialized_string = NULL;
    json_object_set_string(json_obj, "username", username);
    json_object_set_string(json_obj, "password", password);
    serialized_string = json_serialize_to_string_pretty(json_val);
    char *body_data[1];
    body_data[0] = serialized_string;
    // transmit mesajul
    char *message = compute_post_request("63.32.125.183", "/api/v1/tema/admin/users", NULL, "application/json", body_data, 1, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close(sockfd);
    int code = response_find(response);
    // afisez rezultatul
    if (code == 201){
        printf("SUCCESS: Utilizator adaugat cu succes\n");
    }
    if(code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }    
    free(username);
    free(password);
    free(message);
    free(response);
}
// execut comanda get_users, creez mesajul get, deschid conexiunea, transmit
// mesajul, inchid conesiunea si extrag codul.
void get_users(char **cookies, int cookies_count){
    char *message = compute_get_request("63.32.125.183", "/api/v1/tema/admin/users", NULL, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close(sockfd);
    int code = response_find(response);
    if (code == 200){
        // daca operatiunea s-a realizat cu succes, afisez lista de utilizatori
        printf("SUCCES: Lista utilizatorilor:\n");
        size_t length = strlen(response);
        int i = 0;
        // parcurg tot textul in cautarea inceperii json-ului
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        // extrag json-ul si numarul de users pentru a ii parcurge pe rand
        JSON_Value *json_val = json_parse_string(response + i);
        JSON_Object *json_obj = json_value_get_object(json_val);
        JSON_Array *users_array = json_object_get_array(json_obj, "users");

        size_t count = json_array_get_count(users_array);
        // extrag fiecare user din json, iau din el cele 2 string-uri: username
        // si password si afisez
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
    free(message);
    free(response);;
}

void delete_user(char **cookies, int cookies_count){
    // citesc username-ul
    char *username = malloc(sizeof(char) * 100);
    getchar();
    printf("username=");
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';
    // ma asigur ca nu are spatii
    for(int i = 0; i < strlen(username); i++){
        if (username[i] == ' '){
            // daca am gasit un spatiu, afisez mesajul de eroare si ma opresc
            printf("ERROR: Username-ul nu are voie sa aiba spatii\n");
            free(username);
            return;
        }
    }
    // creez url-ul, concatenez username-ul
    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/admin/users/");
    strcat(path, username);
    // creez mesajul si il transmit
    char *message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, NULL);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // daca am primit inapoi codul 200, inseamna ca stergerea s-a realizat cu succes
    // daca am primit un cod 400, a aparut o eroare si afisez mesajul
    if (code == 200){
        printf("SUCCESS: Utilizator sters\n");
    }
    if(code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(path);
    free(message);
    free(response);
    free(username);
}

void get_access(char **cookies, int cookies_count, char *JWT_token){
    // creez mesajul get, deschid conexiunea si il transmit
    char *message = compute_get_request("63.32.125.183", "/api/v1/tema/library/access", NULL, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    // parcurg tot mesajul in cautarea string-ului "token"
    // similar cu get error
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
        // atunci cand gasesc string-ul token, copiez token-ul in variabila jwt_token
        if (j == token_size){
            memcpy(JWT_token, response + i + token_size + 3, length - i - token_size - 6);
            JWT_token[length - i - token_size - 6] = '\0';
            break;
        }
    }
    // caut codul primit ca raspuns si afisez mesajul corespunzator
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Token JWT primit\n");
    } else if(code == 403){
        printf("ERROR: %s\n", error_message(response));
    }
    free(message);
    free(response);
}
void get_movie(char **cookies, int cookies_count, char *JWT_token){
    // citesc id-ul de la tastatura
    char *id = malloc(sizeof(char) * 10);
    getchar();
    printf("id=");
    scanf("%s", id);
    for (int i = 0; i < strlen(id); i++){
        if (id[i] <= '9' && id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return;
        }
    }
    char *url = calloc(100, 1);
    strcat(url, "/api/v1/tema/library/movies/");
    strcat(url, id);
    // alcatuiesc mesajul si adaug si token-ul jwt
    char *message = compute_get_request("63.32.125.183", url, JWT_token, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // caut codul de raspuns si daca e 200, afisez datele asa cum e mentionat
    // in cerinta temei
    if (code == 200){
        printf("SUCCESS: Detalii film\n");
        size_t length = strlen(response);
        int i = 0;
        // parcurg raspunsul in cautarea inceputului json-ului
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        // extrag campurile title rating description si year si le afisez
        JSON_Value *json_val = json_parse_string(response + i);
        JSON_Object *movie = json_value_get_object(json_val);

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
    // eliberez memoria
    free(message);
    free(response);
    free(url);
}

void add_movie(char **cookies, int cookies_count, char *JWT_token){
    // citesc titlul, anul, descrierea si rating-ul.
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
    // daca rating-ul e mai mare decat 9.9, afisez eroarea
    if (rating > 9.9){
        printf("ERROR: rating-ul nu poate fi peste 9.9.\n");
        free(title);
        free(description);
        return;
    }
    // creez json-ul ci datele citite
    JSON_Value *json_val = json_value_init_object();
    JSON_Object *json_obj = json_value_get_object(json_val);
    char *serialized_string = NULL;
    json_object_set_string(json_obj, "title", title);
    json_object_set_number(json_obj, "year", year);
    json_object_set_string(json_obj, "description", description);
    json_object_set_number(json_obj, "rating", rating);
    serialized_string = json_serialize_to_string_pretty(json_val);
    char *body_data[1];
    body_data[0] = serialized_string;
    // fac cererea post si o trimit
    char *message = compute_post_request("63.32.125.183", "/api/v1/tema/library/movies",JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // daca primesc codul 201, a fost  adaugat cu succes si afisez datele
    if (code == 201) {
        printf("SUCCESS: Detalii film\n");
        printf("title: %s\n", title);
        printf("year: %d\n", year);
        printf("description: %s\n", description);
        printf("rating: %f\n", rating);
    } else if (code / 100 >= 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(message);
    free(response);
    free(title);
    free(description);
}
void delete_movie(char **cookies, int cookies_count, char *JWT_token){
    // citesc id-ul
    char *id = malloc(sizeof(char) * 20);
    printf("id=");
    scanf("%s", id);
    for (int i = 0; i < strlen(id); i++){
        if (id[i] <= '9' && id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return;
        }
    }
    // creez url-ul si concatenez id-ul filmului
    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/library/movies/");
    strcat(path, id);
    // 
    char *message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, JWT_token);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    if (code == 200){
        printf("SUCCESS: Film sters.\n");
    }  else if(code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(message);
    free(response);
    free(id);
    free(path);
}
void logout_admin(char **cookies, int *cookies_count, char *JWT_token){
    // transmit cererea GET
    char *message = compute_get_request("63.32.125.183", "/api/v1/tema/admin/logout", NULL, cookies, *cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // verific codul primit
    if (code == 200){
        // daca m-am delogat cu succes, sterg cookie-ul si token-ul jwt
        free(cookies[*cookies_count]);
        (*cookies_count)--;
        memset(JWT_token, 0, 200);
        printf("SUCCES: 200 - OK\n");
    }
    else if (code / 100 == 4){
        printf("ERROR: %s\n", error_message(response));
    }
    free(message);
    free(response);
}
// similar logout_admin
void logout(char **cookies, int *cookies_count, char *JWT_token){
    char *message = compute_get_request("63.32.125.183", "/api/v1/tema/user/logout", NULL, cookies, *cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    if (code == 200){
        free(cookies[*cookies_count]);
        (*cookies_count)--;
        memset(JWT_token, 0, 200);
        printf("SUCCES: 200 - OK\n");
    } else if(code == 403){
        printf("eroare: %s\n", error_message(response));
    }
    free(message);
    free(response);
}
// similar get_users, doar ca mai transmit si token-ul jwt
void get_movies(char *JWT_token){
    char *message = compute_get_request("63.32.125.183", "/api/v1/tema/library/movies", JWT_token, NULL, 0);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    if (code == 200){
        size_t length = strlen(response);
        int i = 0;
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        JSON_Value *json_val = json_parse_string(response + i);
        JSON_Object *json_obj = json_value_get_object(json_val);
        JSON_Array *users_array = json_object_get_array(json_obj, "movies");

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
    free(message);
    free(response);
}

void update_movie(char **cookies, int cookies_count, char *JWT_token){
    // citesc datele: id, titlu, an, descriere si rating
    char *title = malloc(sizeof(char) * 100);
    char *description = malloc(sizeof(char) * 100);
    int year;
    float rating;
    getchar();
    char *id = malloc(sizeof(char) * 10);
    printf("id=");
    scanf("%s", id);
    for (int i = 0; i < strlen(id); i++){
        if (id[i] <= '9' && id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return;
        }
    }
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
    // ma asigur ca rating-ul este mai mic decat 9.9
    printf("rating=");
    scanf("%f", &rating);
    if (rating > 9.9){
        printf("ERROR: Rating-ul nu poate fi mai mare decat 9.9.\n");
        return;
    }
    // creez json-ul si adaug parametrii
    JSON_Value *json_val = json_value_init_object();
    JSON_Object *json_obj = json_value_get_object(json_val);
    char *serialized_string = NULL;
    json_object_set_string(json_obj, "title", title);
    json_object_set_number(json_obj, "year", year);
    json_object_set_string(json_obj, "description", description);
    json_object_set_number(json_obj, "rating", rating);
    serialized_string = json_serialize_to_string_pretty(json_val);
    char *body_data[1];
    body_data[0] = serialized_string;
    // creez string-ul pt url si concatenez id-ul
    char *url = calloc(100, 1);
    strcat(url, "/api/v1/tema/library/movies/");
    strcat(url, id);
    // transmit mesajul
    char *message = compute_put_request("63.32.125.183", url,JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // afisez SUCCESS sau FAIL, in functie de cod
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
    free(message);
    free(response);
}
void get_collections(char **cookies, int cookies_count, char *JWT_token){
    char *message = compute_get_request("63.32.125.183", "/api/v1/tema/library/collections", JWT_token, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
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
        JSON_Value *json_val = json_parse_string(response + i);
        JSON_Object *json_obj = json_value_get_object(json_val);
        JSON_Array *collections_array = json_object_get_array(json_obj, "collections");

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
    free(message);
    free(response);
}
void get_collection(char **cookies, int cookies_count, char *JWT_token){
    // citesc id-ul si il concatenez la url
    char *id = malloc(sizeof(char) * 10);
    printf("id=");
    scanf("%s", id);
    for (int i = 0; i < strlen(id); i++){
        if (id[i] <= '9' && id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return;
        }
    }
    char *url = calloc(100, 1);
    strcat(url, "/api/v1/tema/library/collections/");
    strcat(url, id);
    // transmit cererea la server
    char *message = compute_get_request("63.32.125.183", url, JWT_token, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    if (code == 200){
        // daca s-a realizat cu succes, extrag json-ul
        printf("SUCCESS: Detalii colectie\n");
        size_t length = strlen(response);
        int i = 0;
        for ( i = 0; i < length; i++){
            if (response[i] == '{'){
                break;
            }
        }
        // din json, extrag titlul, owner-ul si filmele
        JSON_Value *root = json_parse_string(response + i);
        JSON_Object *root_obj = json_value_get_object(root);
        const char *title = json_object_get_string(root_obj, "title");
        const char *owner = json_object_get_string(root_obj, "owner");
        printf("title: %s\n", title);
        printf("owner:%s\n", owner);
        printf("movies:\n");
        // extrag fiecare film din lista si il afisez 
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
    free(url);
    free(message);
    free(response);
}

int add_movie_to_collection(char **cookies, int cookies_count, char *JWT_token, char *collection, int id_movie, int add_col){
    // daca add_col este 0, atunci eu primesc aceasta comanda explicit
    // voi citi datele, collection_id si movie_id
    // daca e 1, atunci functia e apelata din cadrul add_collection
    // si am deja id-urile
    int id;
    if (!add_col){
        collection = malloc(sizeof(char) * 100);
        printf("collection_id=");
        scanf("%s", collection);
        for (int i = 0; i < strlen(collection); i++){
        if (collection[i] <= '9' && collection[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return 0;
        }
    }
        printf("movie_id=");
        scanf("%d", &id);
    
        getchar();
    } else {
        id = id_movie;
    }
    // creez json-ul
    JSON_Value *json_val = json_value_init_object();
    JSON_Object *json_obj = json_value_get_object(json_val);
    char *serialized_string = NULL;
    json_object_set_number(json_obj, "id", id);
    serialized_string = json_serialize_to_string_pretty(json_val);
    char *body_data[1];
    body_data[0] = serialized_string;
    char *url = calloc(100, 1);
    // realizez adresa url si transmit mesajul
    strcat(url, "/api/v1/tema/library/collections/");
    strcat(url, collection);
    strcat(url, "/movies");

    char *message = compute_post_request("63.32.125.183", url, JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    // caut codul primit ca raspuns si afisez mesajul
    // returnez 1 daca s-a produs cu succes

    int code = response_find(response);
    free(url);
    free(message);
    free(response);
    if (code == 201){
        if (!add_col){
            printf("SUCCESS: Film adaugat in colectie\n");
        }
        return 1;
    } else if(code / 100 >= 4){
        if (!add_col){
            printf("ERROR: %s\n", error_message(message));
        }
    }
    return 0;
}
// similar cu delete_movie
int delete_collection(char **cookies, int cookies_count, char *JWT_token, char *col){
    // daca nu primesc ca param id-ul colectiei, il citesc
    // il concatenez la url
    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/library/collections/");
    if (col == NULL){
        char *id = malloc(sizeof(char) * 20);
        printf("id=");
        scanf("%s", id);
        for (int i = 0; i < strlen(id); i++){
        if (id[i] <= '9' && id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return 0;
        }
    }
        strcat(path, id);
    } else {
        strcat(path, col);
    }
    // transmit mesajul
    char *message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, JWT_token);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // caut codul de raspuns si afisez mesajul
    if (code == 200){
        if (col == NULL){
            printf("SUCCESS: Colectie stearsa.\n");
        }
        free(message);
        free(response);
        return 1;
    } else {
        printf("ERROR: %s\n", error_message(response));
        free(message);
        free(response);
        return 0;
    }
    return 0;
}
void add_collection(char **cookies, int cookies_count, char *JWT_token){
    // citesc datele :titlul si numarul de filme
    char *title = malloc(sizeof(char) * 100);
    getchar();
    printf("title=");
    fgets(title, 100, stdin);
    if (title[strlen(title) - 1] == '\n') title[strlen(title) - 1] = '\0';
    int num_movies;
    printf("num_movies=");
    scanf("%d", &num_movies);
    // creez json-ul, adaug titlul si numarul de filme
    JSON_Value *json_val = json_value_init_object();
    JSON_Object *json_obj = json_value_get_object(json_val);
    char *serialized_string = NULL;
    json_object_set_string(json_obj, "title", title);
    json_object_set_number(json_obj, "num_movies", num_movies);
    // citesc filmele pe care trebuie sa le adaug
    int *num_id = malloc(sizeof(int) * num_movies);
    for (int i = 0; i < num_movies; i++){
        printf("movie_id[%d]=", i);
        scanf("%d", &num_id[i]);
    }
    serialized_string = json_serialize_to_string_pretty(json_val);
    char *body_data[1];
    body_data[0] = serialized_string;
    // fac cererea post, o trimit (si cu token-ul jwt)
    char *message = compute_post_request("63.32.125.183", "/api/v1/tema/library/collections", JWT_token, "application/json", body_data, 1, cookies, cookies_count);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // daca s-a realizat cu succes, afisez un mesaj intermediar
    if (code == 201){
        printf("Colectie adaugata. acum adaug filmele\n");
        // caut in mesajul primit ca raspuns, json-ul
        int i = 0;
        for(i = 0; i < strlen(response); i++){
            if (response[i] == '{'){
                break;
            }
        }
        JSON_Value *root = json_parse_string(response + i);
        JSON_Object *root_obj = json_value_get_object(root);
        // extrag id-ul colectiei pe  care doar ce am creat-o, colectia e goala
        int id = (int)json_object_get_number(root_obj, "id");
        char *collection_id_string = malloc(sizeof(char) * 10);
        sprintf(collection_id_string, "%d", id);
        // adaug fiecare film citit de la tastatura
        for(int i = 0; i < num_movies; i++){
            // transmit un parametru string-ul ce reprezinta id-ul colrectiei, 
            // id-ul filmului si un 1, ceea ce imi indica faptul ca trebuie
            // sa le folosesc pe acestea
            // nu sa citesc de la tastatura alte date
            int success = add_movie_to_collection(cookies, cookies_count, JWT_token, collection_id_string, num_id[i], 1);
            // returnez 1 daca se realizeaza cu succes. daca nu, sterg toata colectia
            // transmit ca parametru id-ul colectiei, pentru a nu citi de la tastatura
            if (success != 1){
                int del = delete_collection(cookies, cookies_count, JWT_token, collection_id_string);
                // daca se realizeaza stergerea, afisez mesaj de eroare
                if (del == 1){
                    free(message);
                    free(response);
                    printf("ERROR: eroare la adaugare.\n");
                }
                return;
            }
        }
        // daca nu intampin nicio eroare, toate filmele au fost adaugate cu succes si afisez mesajul
        free(collection_id_string);
        printf("SUCCESS: Colectie adaugata\n");
    }
    free(title);
    free(message);
    free(response);
}
// similar cu delete_movie si user
void delete_movie_from_collection(char **cookies, int cookies_count, char *JWT_token){
    // citesc id-ul colectiei
    char *collection_id = malloc(sizeof(char) * 20);
    printf("collection_id=");
    scanf("%s", collection_id);
    for (int i = 0; i < strlen(collection_id); i++){
        if (collection_id[i] <= '9' && collection_id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return;
        }
    }
    // citesc id-ul filmului
    char *movie_id = malloc(sizeof(char) * 20);
    printf("movie_id=");
    scanf("%s", movie_id);
    for (int i = 0; i < strlen(movie_id); i++){
        if (movie_id[i] <= '9' && movie_id[i]>= '0'){
            continue;
        }
        else{
            printf("FAIL: id-ul trebuie sa fie un numar.\n");
            return;
        }
    }
    // creez url-ul
    char *path = calloc(100, 1);
    strcat(path,"/api/v1/tema/library/collections/");
    strcat(path, collection_id);
    strcat(path, "/movies/");
    strcat(path, movie_id);
    // fac cererea delete, cu token-ul jwt
    char *message = compute_delete_request("63.32.125.183", path, cookies, cookies_count, JWT_token);
    int sockfd = open_connection("63.32.125.183", 8081, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int code = response_find(response);
    // caut codul de raspuns si afisez mesajul
    if (code == 200){
        printf("SUCCESS: Film sters din colectie.\n");
    } else {
        printf("ERROR: %s\n", error_message(response));
    }
    free(message);
    free(response);
    free(collection_id);
    free(movie_id);
}
int main(int argc, char *argv[])
{
    // aloc memorie pt comanda, vectorul de cookies si token-ul jwt
    char *command = malloc(sizeof(char) * 100);
    int running = 1;
    int cookies_size = 2;
    char **cookies = malloc(2 * sizeof(char *));
    int cookies_count = 0;
    char *JWT_token = calloc(200, sizeof(char));

    while(running){
        scanf("%s", command);
        // citesc comanda si caut ce comanda reprezinta
        // daca este exit, atunci opresc bucla while
        // dupa ce execut comanda, setez tot string-ul pe 0 cu memeset
        // ca sa nu am erori la citire
        if (strcmp(command, "exit") == 0){
            memset(JWT_token, 0, 200);
            running = 0;
        } else if (strcmp(command, "login") == 0){
            login(cookies, &cookies_count, &cookies_size, 0);
        } else if (strcmp(command, "login_admin") == 0){
            login(cookies, &cookies_count, &cookies_size, 1);
        } else if(strcmp(command, "add_user") == 0){
            add_user(cookies, cookies_count);
        } else if(strcmp(command, "get_users") == 0){
            get_users(cookies, cookies_count);
        } else if (strcmp(command, "delete_user") == 0){
            delete_user(cookies, cookies_count);
        } else if (strcmp(command, "logout") == 0){
            logout(cookies, &cookies_count, JWT_token);
        } else if(strcmp(command, "logout_admin") == 0){
            logout_admin(cookies, &cookies_count, JWT_token);
        } else if(strcmp(command, "get_access") == 0){
            get_access(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_movie") == 0){
            get_movie(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_movies") == 0){
            get_movies(JWT_token);
        } else if(strcmp(command, "add_movie") == 0){
            add_movie(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "delete_movie") == 0){
            delete_movie(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "update_movie") == 0){
            update_movie(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_collections") == 0){
            get_collections(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "get_collection") == 0){
            get_collection(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "add_collection") == 0){
            add_collection(cookies, cookies_count, JWT_token);
        } else if(strcmp(command, "delete_collection") == 0){
            delete_collection(cookies, cookies_count, JWT_token, NULL);
        } else if(strcmp(command, "add_movie_to_collection") == 0){
            add_movie_to_collection(cookies, cookies_count, JWT_token, NULL, 0, 0);
        } else if(strcmp(command, "delete_movie_from_collection") == 0){
            delete_movie_from_collection(cookies, cookies_count, JWT_token);
        } else {
            printf("Invalid command!\n");
        }
        memset(command, 0, 100);
    }
    return 0;
}
