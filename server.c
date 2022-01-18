/**
 * Třída, která reprezentuje server.
 * @author Michaela Benešová
 * @version 1.0
 * @date 12.01.2022
 */ 

#include<stdio.h>
#include<string.h>    
#include<stdlib.h>    
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h> 
#include<pthread.h>
#include<time.h>

#define CLIENT_MSG_SIZE 512
#define REQ_ID "1"
#define CONNECT_TO_GAME "2"
#define RECONNECT_TO_GAME "3"
#define CREATE_NEW_ROOM "4"
#define START_GAME "5"
#define SEND_QUIZ_ANSWER "6"
#define PAUSE_GAME "7"
#define LEAVE_GAME "8"
#define NEXT_QUESTION "9"
#define BACK_TO_MENU "10"
#define SHOW_TABLE "11"


#define STATE_IN_LOBBY 1
#define STATE_IN_GAME 2
#define STATE_IN_PAUSE 3
#define STATE_ENDED 4

#define ERR "-1"


#define NUMBER_OF_PLAYERS 90

#define NUMBER_OF_ROOMS 30

#define NUMBER_OF_QUESTIONS 3

#define NUMBER_PLAYERS_IN_ONE_GAME 3

void *connection_handler(void *);

/**
 * Struktura která symbolizuje jednoho hráče.
 * */
typedef struct Player
{
 
    int id_p; 

    int points;

    int socket;
    
    char name[255];
   
    int end_game;

    char passwd[255];
} player;


/**
 * Struktura, která symbolizuje jednu hru.
*/

typedef struct Game
{
   
    int id_r;
    
    int availability[3];
   
    player array_p[3];
   
    int state;
  
    int number_players;
   
    int q_ids[NUMBER_OF_QUESTIONS];
    
    int round;
} game;

/**
 * Struktura, která symbolizuje jednu otázku.
*/
typedef struct Question
{
    
    char *question;
    char *ans1;
    
    char *ans2;
    
    char *ans3;
    
    char *ans4;
    
    int correct;

} question;


player* players = NULL;

game* games = NULL;

question* questions = NULL;

/**
 * @brief Funkce pro uspání hry, před začátkem dalšího kola.
 * 
 * @param game_id id hry
*/
void* sleep_time(int game_id){
    sleep(5);
    next_round(game_id);
}

/**
 * @brief Vstupni bod programu
 * 
 * @param argc pocet argumentu
 * @param argv pole argumentu
 * @return int navratova hodnota programu
 */
int main(int argc , char *argv[])
{

    setvbuf(stdout, NULL, _IOLBF, 0);
    players = calloc(NUMBER_OF_PLAYERS,sizeof(player));
    games = calloc(NUMBER_OF_ROOMS, sizeof(game));
    questions = calloc(NUMBER_OF_QUESTIONS, sizeof(question));


    questions[0].question = "Jakou barvu má sluníčko?";
    questions[0].ans1 = "modrá";
    questions[0].ans2 = "žlutá";
    questions[0].ans3 = "černá";
    questions[0].ans4 = "růžová";
    questions[0].correct = 2;

    questions[1].question = "Jakou barvu má nebe?";
    questions[1].ans1 = "modrá";
    questions[1].ans2 = "žlutá";
    questions[1].ans3 = "černá";
    questions[1].ans4 = "růžová";
    questions[1].correct = 1;


    questions[2].question = "Jakou barvu má tráva?";
    questions[2].ans1 = "modrá";
    questions[2].ans2 = "žlutá";
    questions[2].ans3 = "černá";
    questions[2].ans4 = "zelená";
    questions[2].correct = 4;


    int i;
    for(i = 0; i < NUMBER_OF_PLAYERS; i++){
        players[i].id_p=-1;
        players[i].end_game = 0;
    }


    int h;
    for(i = 0; i < NUMBER_OF_ROOMS; i++){
        int questions[NUMBER_OF_QUESTIONS];
        int q_len = 0;
        int duplicate = 0;

        int j;
        while(q_len < NUMBER_OF_QUESTIONS){
            duplicate = 0;
            int rnd = rand() % NUMBER_OF_QUESTIONS;

            for (j = 0; j < q_len; j++){
                if (questions[j] == rnd){
                    duplicate = 1;
                }
            }

            if (duplicate == 0){
                questions[q_len] = rnd;
                q_len++;
            }
        }

        games[i].id_r = -1;
        games[i].round = 0;
        memcpy(games[i].q_ids, questions , sizeof(games[i].q_ids));
        for (h = 0;h < 3; h++){
            games[i].availability[h] = 0;
        }
    }

    
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
  
    int enable = 1;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (socket_desc == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    int pom;
    pom = atoi(argv[1]);
    server.sin_port = htons(pom);
   

     
    
     
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("bind failed. Error\n");
        return 1;
    }
    printf("bind done\n");

    listen(socket_desc , 3);

    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("Connection accepted\n");
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            printf("could not create thread\n");
        }
        printf("Handler assigned\n");

       
    }
     
    if (client_sock < 0)
    {
        printf("accept failed\n");
        return 1;
    }
     
    free(players);
    free(games);
    free(questions);
    return 0;
}


/**
 * @brief Funkce je spustena ve vlastnim vlakne a obstarava jednoho klienta, zpracovava prijate zpravy
 * 
 * @param socket_desc socket uzivatele
 * @return void* 
 */
void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[CLIENT_MSG_SIZE];
    FILE *file_socket;
    char id[10], command[30], params[30];
 
    file_socket = fdopen(sock);
  
    while( (read_size = fscanf(file_socket, "%s,%s,%s", id, command, params)))
    {
 
        if (strcmp(command, CREATE_NEW_ROOM) == 0){
            int i;

            for(i = 0; i < NUMBER_OF_ROOMS; i++){
                if(games[i].id_r == -1){
                    games[i].id_r = i;
                    games[i].state = STATE_IN_LOBBY;
                    games[i].array_p[0] = players[atoi(id)];
                    games[i].number_players = 1;
                    games[i].availability[0] = 1;
                    break;
                }
            }


            char text_number[8];
            sprintf(text_number, "%d", i);

            char *msg = malloc(strlen("4,") + strlen(text_number) + 1);
            memcpy(msg, "4,", strlen("4,"));
            memcpy(msg + strlen("4,"), text_number, strlen(text_number));
            memcpy(msg + strlen("4,") + strlen(text_number), ";1", strlen(";1") + 1);
            write(sock, msg, sizeof(msg));
            free(msg);
        }


        else if (strcmp(command, START_GAME) == 0){
            int player_id = atoi(id);
            int g;
            int game_id;

            char text_number[8];
            sprintf(text_number, "%d", player_id);

            for (g = 0; g < NUMBER_OF_ROOMS;g++){
                if(games[g].array_p[0].id_p == player_id){
                    game_id = g;
                    break;
                }
            }

            games[g].state = STATE_IN_GAME;

            question q = questions[games[game_id].q_ids[games[game_id].round]];

            char *msg = malloc(strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-")+ strlen(q.ans2) + strlen("-") + strlen(q.ans3) + strlen("-")+ strlen(q.ans4) + 1);
            memcpy(msg, "5,", strlen("5,"));
            memcpy(msg + strlen("5,"), q.question, strlen(q.question)); 
            memcpy(msg + strlen("5,") + strlen(q.question), ";", strlen(";")); 
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";"), q.ans1, strlen(q.ans1)); 
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1), "-", strlen("-"));
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-"), q.ans2, strlen(q.ans2)); 
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2), "-", strlen("-")); 
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-"), q.ans3, strlen(q.ans3)); 
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3), "-", strlen("-")); 
            memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3)+ strlen("-"), q.ans4, strlen(q.ans4) + 1); 

            int p;
            for (p = 0; p < 3; p++){
                if (games[game_id].availability[p] == 1){
                    write(games[game_id].array_p[p].socket, msg, strlen(msg));
                }       
            }
            free(msg);
        }

        else if (strcmp(command, CONNECT_TO_GAME) == 0){
            int game_id = atoi(params);
            
            if(game_id > 30){
     		send_error("spatne id", sock);
                return;
                
            } else if(games[game_id].id_r == -1){
     		send_error("spatne id", sock);
                return;
            }
            else if(games[game_id].number_players == 3){
                send_error("Plná místnost", sock);
                return;
            } 
            else if(games[game_id].state == STATE_IN_GAME) {
                send_error("Hra už jede", sock);
                return;
            }
            
            int i;
            for (i = 0; i < 3; i++){
                if (games[game_id].availability[i] == 0){
                    games[game_id].array_p[i] = players[atoi(id)];
                    games[game_id].number_players++;
                    games[game_id].availability[i] = 1;
                    break;
                }  
            } 

            char text_number[8];
            sprintf(text_number, "%d", games[game_id].number_players);

            char *msg = malloc(strlen("2,") + strlen(text_number) + 1);
            memcpy(msg, "2,", strlen("2,"));
            memcpy(msg + strlen("2,"), text_number, strlen(text_number)+1);

            int p;
            for (p = 1; p < 3; p++){
                if (games[game_id].availability[p] == 1){
                    write(games[game_id].array_p[p].socket, msg, sizeof(msg));
                }       
            }
            free(msg);


            char game_id_text[8];
            sprintf(game_id_text, "%d", game_id);

            char *msg_for_admin = malloc(strlen("4,") + strlen(game_id_text) + strlen(text_number) + 1);
            memcpy(msg_for_admin, "4,", strlen("4,"));
            memcpy(msg_for_admin + strlen("4,"), game_id_text, strlen(game_id_text));
            memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text), ";", strlen(";"));
            memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text) + strlen(";"), text_number, strlen(text_number)+1);

            write(games[game_id].array_p[0].socket, msg_for_admin, sizeof(msg_for_admin));
            free(msg_for_admin);

        } 

        else if (strcmp(command, SEND_QUIZ_ANSWER) == 0){
            int player_id = atoi(id);
            int answer = atoi(params);
            int game_id;

            int g,t;
            for (g = 0; g < NUMBER_OF_ROOMS;g++){
                for (t = 0; t < NUMBER_PLAYERS_IN_ONE_GAME; t++) {
                    if(games[g].availability[t] == 1) {
                        if(games[g].array_p[t].id_p == player_id) {
                        	game_id = g;
                        	break;
                        }
                    }
                }  
            }

            if (answer == questions[games[game_id].q_ids[games[game_id].round]].correct){

                players[player_id].points += 1;


                char correct[8];
                sprintf(correct, "%d", questions[games[game_id].q_ids[games[game_id].round]].correct);

                int p;
                for (p = 0; p < 3; p++){

		        char points[8];
		        sprintf(points, "%d", players[games[game_id].array_p[p].id_p].points);
		        
		        
		        char *msg = malloc(strlen("6,") + strlen(points) + strlen(";") + strlen(correct) + 1);
		        memcpy(msg, "6,", strlen("6,"));
		        memcpy(msg + strlen("6,"), points, strlen(points));
		        memcpy(msg + strlen("6,") + strlen(points), ";", strlen(";"));
		        memcpy(msg + strlen("6,") + strlen(points) + strlen(";"), correct, strlen(correct) + 1);
		        write(games[game_id].array_p[p].socket, msg, sizeof(msg));
		        free(msg);
                       
                }
                pthread_t thread;
                pthread_create(&thread, NULL, sleep_time, game_id);    
            }


            
        }

        else if (strcmp(command, LEAVE_GAME) == 0){
            int player_id = atoi(id);
            int g, i, game_id;
            int is_admin = 0;

            for (g = 0; g < NUMBER_OF_ROOMS;g++){
                for(i = 0; i < 3; i++){
                    if(games[g].array_p[i].id_p == player_id && games[g].array_p[i].end_game == 1){
                        game_id = g;
                        games[game_id].availability[i] = 0;
                        games[game_id].number_players -= 1;
                        players[player_id].id_p = -1;
                        players[player_id].end_game = 0;
                        players[player_id].points = 0;
                        if (i == 0){
                            is_admin = 1;
                        }
                        break;
                    }
                }   
                break;
            }

            


            if(games[game_id].state == STATE_IN_LOBBY){

                char text_number[8];
                sprintf(text_number, "%d", games[game_id].number_players);

                char *msg = malloc(strlen("2,") + strlen(text_number) + 1);
                memcpy(msg, "2,", strlen("2,"));
                memcpy(msg + strlen("2,"), text_number, strlen(text_number)+1);

                int p;
                for (p = 0; p < 3; p++){
                    if (games[game_id].availability[p] == 1){
                        printf("Zprava: %s", msg);
                        write(games[game_id].array_p[p].socket, msg, sizeof(msg));
                    }       
                }
                free(msg);
                

                char text_number_2[8];
                sprintf(text_number_2, "%d", games[game_id].number_players);


                char game_id_text[8];
                sprintf(game_id_text, "%d", game_id);

                char *msg_for_admin = malloc(strlen("4,") + strlen(game_id_text) + strlen(text_number_2) + 1);
                memcpy(msg_for_admin, "4,", strlen("4,"));
                memcpy(msg_for_admin + strlen("4,"), game_id_text, strlen(game_id_text));
                memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text), ";", strlen(";"));
                memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text) + strlen(";"), text_number_2, strlen(text_number_2)+1);

                write(games[game_id].array_p[0].socket, msg_for_admin, sizeof(msg_for_admin));
                free(msg_for_admin);
            } 
           

        } 

        else if (strcmp(command, REQ_ID) == 0){
            int i;
            int rec = 0;
            
            char *l = strtok(params, "+");
            char *larams[2];
            while (l != NULL)
                {
                    larams[i++] = l;
                    l = strtok(NULL, "+");
                }



            for(i = 0; i < NUMBER_OF_PLAYERS; i++){
                if(players[i].id_p != -1) {

                    if(strcmp(players[i].name, params) == 0 && players[i].end_game == 0){
                        if( strcmp(players[i].passwd,larams[1]) == 0) {
                            reconnect(i, sock);
                            rec = 1;
                            break;
                        } 
                    } 
                }
            }


            if (rec == 0){
                for(i = 1; i < NUMBER_OF_PLAYERS; i++){
                    if(players[i].id_p == -1){
                        players[i].id_p = i;
                        if(sock>=0){
                            players[i].socket = sock;
                        }
                        players[i].points = 0;
                        memcpy(players[i].name, larams[0], strlen(larams[0]));
                        memcpy(players[i].passwd, larams[1], strlen(larams[1]));
                        puts(players[i].passwd);
                        break;
                    }
                }


                char text_number[8];
                sprintf(text_number, "%d", i);

                char *msg = malloc(strlen("1,") + strlen(text_number) + 1);
                memcpy(msg, "1,", strlen("1,"));
                memcpy(msg + strlen("1,"), text_number, strlen(text_number)+1);
                write(sock, msg, sizeof(msg));
                free(msg);
            }
        }
      
	    memset(client_message, 0, CLIENT_MSG_SIZE);
    }
     
    if(read_size == 0)
    {

        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    return 0;
} 

/**
 * @brief funkce pro reconnect
 * @param player_id id hráče
*/
void reconnect(int player_id, int sock){
    int g, i, game_id;
    
    for (g = 0; g < NUMBER_OF_ROOMS;g++){
        for(i = 0; i < 3; i++){
            if(players[player_id].id_p == games[g].array_p[i].id_p){
                game_id = g;
                games[g].array_p[i].socket = sock;
                break;
            }
        }   
    }
    
    char text_number[8];
    sprintf(text_number, "%d", games[game_id].state);
    puts("state");
    if(games[game_id].state == STATE_IN_LOBBY){
    	char text_player_id[8];
        sprintf(text_player_id, "%d", player_id);
        
        char *msg_id = malloc(strlen("1,") + strlen(text_player_id) + 1);
        memcpy(msg_id, "1,", strlen("1,"));
        memcpy(msg_id + strlen("1,"), text_player_id, strlen(text_player_id)+1);
        write(sock, msg_id, sizeof(msg_id));
    
        char text_number[8];
        sprintf(text_number, "%d", games[game_id].number_players);

        char *msg = malloc(strlen("2,") + strlen(text_number) + 1);
        memcpy(msg, "2,", strlen("2,"));
        memcpy(msg + strlen("2,"), text_number, strlen(text_number)+1);

        write(players[player_id].socket, msg, strlen(msg));
        free(msg);
    }
    else if (games[game_id].state == STATE_IN_GAME){
    	char text_player_id[8];
        sprintf(text_player_id, "%d", player_id);
        
        char *msg_id = malloc(strlen("1,") + strlen(text_player_id) + 1);
        memcpy(msg_id, "1,", strlen("1,"));
        memcpy(msg_id + strlen("1,"), text_player_id, strlen(text_player_id)+1);
        write(sock, msg_id, sizeof(msg_id));
    
        question q = questions[games[game_id].q_ids[games[game_id].round]];
        char *msg = malloc(strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-")+ strlen(q.ans2) + strlen("-") + strlen(q.ans3) + strlen("-")+ strlen(q.ans4) + 1);
        memcpy(msg, "5,", strlen("5,"));
        memcpy(msg + strlen("5,"), q.question, strlen(q.question)); 
        memcpy(msg + strlen("5,") + strlen(q.question), ";", strlen(";")); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";"), q.ans1, strlen(q.ans1));
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1), "-", strlen("-"));
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-"), q.ans2, strlen(q.ans2)); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2), "-", strlen("-")); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-"), q.ans3, strlen(q.ans3)); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3), "-", strlen("-")); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3)+ strlen("-"), q.ans4, strlen(q.ans4) + 1); 

        write(sock, msg, strlen(msg));
        free(msg);
   } 
}

/**
 * @brief odeslání erroru na klienta
 * @param msg zpráva která se odešle ->info o erroru
 * @param sock na jaký socket se posílá zpráva
*/
void send_error(char* msg, int sock){
    char* m = malloc(strlen("-1,") + strlen(msg) + 1);
    memcpy(m, "-1,", strlen("-1,"));
    memcpy(m + strlen("-1,"), msg, strlen(msg) + 1);
    write(sock, m, strlen(m));
    free(m);
}

/**
 * @brief funkce pro tvorbu dalšího kola
 * @param room id místnosti
*/
void next_round(int room){
    games[room].round += 1;
    if (games[room].round == NUMBER_OF_QUESTIONS){
        send_points(room);
        return;
    }

    question q = questions[games[room].q_ids[games[room].round]];

    char *msg = malloc(strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-")+ strlen(q.ans2) + strlen("-") + strlen(q.ans3) + strlen("-")+ strlen(q.ans4) + 1);
    memcpy(msg, "5,", strlen("5,"));
    memcpy(msg + strlen("5,"), q.question, strlen(q.question)); 
    memcpy(msg + strlen("5,") + strlen(q.question), ";", strlen(";")); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";"), q.ans1, strlen(q.ans1)); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1), "-", strlen("-"));
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-"), q.ans2, strlen(q.ans2)); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2), "-", strlen("-")); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-"), q.ans3, strlen(q.ans3)); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3), "-", strlen("-")); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3)+ strlen("-"), q.ans4, strlen(q.ans4) + 1); 


    int p;
    for (p = 0; p < 3; p++){
        if (games[room].availability[p] == 1){
            write(games[room].array_p[p].socket, msg, strlen(msg));
        }       
    }
    free(msg);
}

/**
 * @brief kontrola zda někdo dosáhl maximální počtu bodů - byly zodpovězeny všechny otázky, poté odeslání na klienta zda vyhrál či prohál
 * @param room id místnosti
*/
void send_points(int room){
    int max_points, i, id_max_points;
    max_points = 0;
    id_max_points = 0;
    for(i = 0; i < 3; i++){
        if(games[room].availability[i] != 0 && max_points < players[games[room].array_p[i].id_p].points){
            max_points = players[games[room].array_p[i].id_p].points;
            id_max_points = i;
        }
    }

    int p;
    for (p = 0; p < 3; p++){
        if (games[room].availability[p] != 0){
            if(p != id_max_points){
                char *msg = malloc(strlen("11,") + strlen("Prohrál jsi") + 1);
                memcpy(msg, "11,", strlen("11,"));
                memcpy(msg + strlen("11,"), "Prohrál jsi", strlen("Prohrál jsi"));
                write(games[room].array_p[p].socket, msg, strlen(msg));
                free(msg);
                games[room].array_p[p].end_game = 1;
            }
            else{
                char *msg = malloc(strlen("11,") + strlen("Vyhrál jsi") + 1);
                memcpy(msg, "11,", strlen("11,"));
                memcpy(msg + strlen("11,"), "Vyhrál jsi", strlen("Vyhrál jsi"));
                write(games[room].array_p[id_max_points].socket, msg, strlen(msg));
                free(msg);
                games[room].array_p[id_max_points].end_game = 1;
            }
        } 
        
    }

    clean_room(room);
}

/**
 * @brief připraví místnost pro další hráče
 * @param room id místnosti
*/
void clean_room(int room){
    int i;
    for(i = 0; i < 3; i++){
        games[room].availability[i] = 0; 
    }
    games[room].number_players = 0;
    games[room].round = 0;
    games[room].state = -1;
    games[room].id_r = -1;
}
