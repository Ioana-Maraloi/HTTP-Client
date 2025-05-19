# modul de abordare
 - am inceput de la laboratorul 9, am copiat fisierele helpers, requests si buffer. am copiat, asa cum era mentionat in tema, fisierele parson.c si parson.h.
 - am facut in main un mare while, citesc comanda si am creat foarte multe if-uri ca sa vad ce comanda trebuie sa execut. apoi dau memset ca sa nu am probleme la citire.
 - am inceput sa creez functiile in ordinea in care erau descrise pe site-ul temei, initial nu testam cu checker-ul pt ca voiam sa afisez tot response-ul si sa vad daca e ok, dupa ce implementam mai multe functii, corectam si output-ul astfel incat sa afisez fix cum scria pe site si rulam checkerul.
 - am creat apoi get_delete_requests in requests.c si put, m-am luat dupa modelul de la post si get din laborator.



# functii
## cookie_find
 - parcurg tot string-ul primit ca raspuns de la server si caut string-ul "Set-Cookie" ca sa vad unde este pozitia de inceput a mesajului de cookie
## cookie_extract
 - cu numarul primit de la cookie_find, calculez cat de lung e cookie-ul ca sa vad cat de mult aloc si dau memcpy. caut caracterul ';' pt ca asa sunt delimitate campurile in json-ul returnat de server.
## error_message
- functie pentru a extrage mesajul de eroare. o apelez in cazul in care codul incepe cu 4
## response_find
- functie pentru a extrage codul de raspuns din mesajul transmis de server. caut textul "http/1.1" pentru ca stiu ca dupa el, se afla codul. citesc cifra cu cifra si inmultesc si returnez.
## login
- am facut o singura functie pentru login si login_admin. am adaugat un parametru la sfarsit, un int: daca e 0, fac login(citesc si parametrul "admin_username", adaug acest param in json si transmit catre alta adresa), iar daca e 1, fac login_admin(citesc doar username si password). creez json-ul si adaug parametrii si transform in char ** pt a putea sa apelez compute_post_request. deschid conexiunea, transmit mesajul catre server, inchid conexiunea si caut cookie-ul(il adaug daca il gassesc), si caut codul de raspuns, afisand mesajul SUCCESS sau ERROR.
## add_user
- citesc datele, username-ul si parola, dar verific ca username-ul sa nu aiba spatii, pentru ca nu are voie(caz in care afisez mesaj de eroare si returnez). creez json-ul, transform in char **, trimit mesajul cu post, caut codul si afisez mesjaul corespunzator: SUCCESS sau ERROR.
## get_users
- execut comanda get_users, fac mesajul get, deschid conexiunea, transmit mesajul si extrag codul din mesajul de raspuns. daca acest cod este 200, operatiunea s-a realizat cu succes si mi-a fost trimis si json-ul cu toti utilizatorii. parcurg caracter cu caracter, pentru a cauta {, deoarece de aici incepe json-ul. extrag json-ul si vad cate elemente am in users. ii parcurg pe fiecare in parte si extrag username-ul si parola is le afisez asa cum era descris in tema.
## delete_user
- citesc username-ul, verific sa nu aiba spatii(in cazul in care are spatii, afisez ERROR), creez url-ul, compun mesajul, deschid conexiunea si transmit mesajul. caut codul in raspuns, iar daca e 200, inseamna ca operatia s-a realizat cu success, daca se afla in grupa 400, inseamna ca a fost o eroare si afisez textul erorii.
## get_access
- creez mesajul get, deschid conexiunea si il transmit. caut in mesajul primit ca raspuns string-ul "token", pentru ca am observat ca asa se afiseza in json. copiez apoi tokenul in variabila jwt_token, declarata in main. caut codul de raspuns si afisez mesajul corespunzator: SUCCESS sau ERRROR.
## get_movie
- citesc de la tastatura id-ul, fac cererea get, transmit mesajul la server, adaugand si token-ul jwt. in raspunsul primit, caut codul si, in cazul in care e 200, afisez SUCCESS si parcurg raspunsul, cautand { si extragand json-ul de acolo. apoi din json, extrag titlul, rating-ul, anul si descrierea si le afisez conform cerintei temei.
## add_movie
- citesc datele: titlul, anul, descrierea si rating-ul. daca rating-ul este peste 9.9, este imposibil si afisez mesaj de eroare, eliberez memoria si ma opresc. daca e ok, continui si creez json-ul, adaug in el campurile si transmit cererea post. atunci cand primesc raspunsul, caut codul si, daca e 201, atunci au fost adaugate datele cu succes si le afisez.
## delete_movie
- am citit id-ul de la tastatura, am creat o variabila pentru url si am concatenat id-ul acolo. am creat functia compute_delete_request in fisierul requests.c, dupa modelul get. am apelat functia, dand ca parametru tokenul jwt. daca am primit codul 200, operatiunea s-a realizat cu succes.
## logout_admin
## logout
## get_movies
## update_movie
## get_collections
## get_collection
-  citesc id-ul colectiei, concatenez la url si transmit cererea. daca s-a realizat cu succes, extrag json-ul si, din acesta extrag titlul, owner-ul si le afisez. apoi extrag lista de filme si afisez fiecare film.
## add_movie_to_collection
- daca add_col e 0, atunci citesc datele de la tastatura(id-urile). daca este 1, atunci stiu ca am primit ca parametrii id-urile. creez json-ul si cererea si o transmit. caut codul si daca e successful, returnez 1(daca add_col e 0, afisez si mesajul SUCCESS, daca e 1, atunci nu afisez mesajul pt ca e parte din functia add_collection).
## delete_collection
- similar cu delete_movie
## add_collection
- citesc datele: titlul si numarul de filme care trebuie adaugate. creez un json si adaug aceste date. citesc id-urile filmelor. transmit cererea post si verific daca am primit codul 201. daca da, atunci cererea s-a realizat cu succes si am creat o colectie goala. afisez un mesaj intermediar, apoi caut in mesajul primit ca raspuns caracterul {, extrag json-ul de acolo. iau din json id-ul colectiei, il transform in string. parcurg toate filmele pe care trebuie sa le adaug in colectie. apelez functia add_movie_to_collection si trimit ca parametri id-ul colectiei, al filmului si un parametru 1, care imi indica faptul ca trebuie sa le folosesc pe acestea, nu sa citesc altele de la tastatura. daca functia se termina cu succes, returnez 1. adaug astfel toate filmele, daca unul dintre ele nu se adauga (din cauza unei erori precum inexistenta filmului), sterg toata colectia si afisez mesaj de eroare. daca am reusit sa adaug toate filmele in colectie, afisez mesaj de succes.
## delete_movie_from_collection
- similar cu delete_movie, citesc datele, creez url-ul si transmit cererea delete. caut codul de raspuns si afisez mesajul corespunzator.


# probleme intampinate
- mi-a fost mai greu la afisare, am uitat de cateva ori sa pun si # la inceputul liniei si imi dadea eroare si nu imi dadeam seama de ce. :))
- la add_collection am stat cateva ore sa ma gandesc si nu mi se adaugau filmele in colectie, apoi mi-am dat seama ca trebuie sa creez colectia, sa adaug filmele, iar daca filmele nu se adauga cum trebuie, csa sterg colectia. 
- asadar, am creat functia add_movie_to_collection si ea, comparativ cu celelalte functii facute de mine, returneaza int, nu e de tip void. daca codul returnat de acest request este favorabil, atunci returnez 1, iar daca nu e ok, daca a aparut o eroare(daca nu s-a gasit filmul), returneaza 0. fac un for in add_collection si adaug filmele pe rand, iar daca add_movie_to_collection returneaza 0, atunci nu merge creata colectia si ma opresc, sterg colectia si afisez mesajul de eroare.
- ca sa nu fac inca o functie pt delete_collection si add_movie_from_collection, am adaugat la acestea cate un parametru. la add movie, am pus un 0 sau un 1, 1 daca apelez aceasta functie din pozitia de a adauga un film in cadrul add_collection(deci nu mai afisez si citesc de la tastatura) si 0 in cazul in care trebuie sa citesc de la tastatura. similar pentru delete_collection, doar ca cu un char care reprezinta id-ul colectiei.
