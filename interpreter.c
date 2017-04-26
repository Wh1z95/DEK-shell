#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <locale.h>
#include <ctype.h>

int flag_backgroud = 0;
int flag_script = 0;
int flag_output = 0;
char output[30];
//formatuje polecenie i rozbija je na oddzielne potoki. Parametrem jest polecenie przechwycone fgetsem i liczna wczytanych potokow 

char **sepstrs(char *cmd_line, int *strs_num) 
{
    if(cmd_line == NULL) return NULL;
    else if(cmd_line[0] == 0) return NULL;

    int i = 0, j = 0, c = 0;
    (*strs_num) = 0;

    int cmd_line_lenght = strlen(cmd_line);
    i = cmd_line_lenght - 1;
    while(cmd_line[i] == 32) --i;
    if(cmd_line[i] == '&' && cmd_line[i-1] == 32)
    {
        flag_backgroud = 1;
        --i;
    }
    while(cmd_line[i] == 32 || cmd_line[i] == '|' || cmd_line[i] == '>') --i; 
    cmd_line_lenght = i + 1;

    i = 0;
    while(cmd_line[i]) //pobieranie ze standardowego wejscia pojedynczy wiersz
    {
        if(cmd_line[i] == '>')
        {
            cmd_line[i] = 32;
            j = i;
            while(cmd_line[j] == 32 && j > 0) --j;
            cmd_line_lenght = j+1;

            ++i;
            if(cmd_line[i] == '>')
            {
                flag_output = 1;
                ++i;

                while(cmd_line[i] == 32 || cmd_line[i] == '>') ++i;
                j = 0;
                while(cmd_line[i] && j < 29)
                {
                    output[j] = cmd_line[i];
                    ++i;
                    ++j;
                }
                output[j] = 0;
                break;
            }
            else break;
        }

        ++i;
    }

    i = 0;
    while(cmd_line[i] == 32) ++i;
    while(i < cmd_line_lenght)
    {
        if(cmd_line[i] == 32)
        {
            while(cmd_line[i] == 32) ++i;
            if(cmd_line[i] == '|') //mozliwosc twoerzenia potokow o dowolnej dlugości 
            {
                ++c;
                while(cmd_line[i] == 32 || cmd_line[i] == '|') ++i;
            }
            else
            {
                c += 2;
                ++i;
            }
        }
        else
        {
            if(cmd_line[i] == '|')
            {
                ++c;
                while(cmd_line[i] == 32 || cmd_line[i] == '|') ++i;
            }
            else
            {
                ++i;
                ++c;
            }
        }
    }

    if(c == 0) return NULL;
    char *fcmd_line = (char*)malloc((c+1) * sizeof(char));

    i = 0; j = 0;
    ++(*strs_num);
    while(cmd_line[i] == 32) ++i;
    while(i < cmd_line_lenght)
    {
        if(cmd_line[i] == 32)
        {
            while(cmd_line[i] == 32) ++i;
            if(cmd_line[i] == '|')
            {
                fcmd_line[j++] = cmd_line[i];
                while(cmd_line[i] == 32 || cmd_line[i] == '|') ++i;
            }
            else
            {
                fcmd_line[j++] = 32;
                fcmd_line[j++] = cmd_line[i++];
            }
        }
        else
        {
            if(cmd_line[i] == '|')
            {
                fcmd_line[j++] = cmd_line[i];
                while(cmd_line[i] == 32 || cmd_line[i] == '|') ++i;
            }
            else
            {
                fcmd_line[j] = cmd_line[i];
                ++j;
                ++i;
            }
        }
    }
    fcmd_line[j] = 0;
    i = 0;

    while(fcmd_line[i])
    {
        if(fcmd_line[i] == '|') ++(*strs_num);
        ++i;
    }

    //printf("fcmd_line = \"%s\"\n", fcmd_line);
    //printf("strs_num = %i\n", (*strs_num));
    //printf("fcmd_line_lenght = %i\n", c);

    char **strs = (char**)malloc((*strs_num) * sizeof(char*));
    int s = (*strs_num);

    int n = 0;;
    i = 0;
    for(; n < s - 1; ++n)
    {
        c = 0;
        while(fcmd_line[i] != '|')
        {
            ++i;
            ++c;
        }
        ++i;
        strs[n] = (char*)malloc(c * sizeof(char));
    }
    c = 0;
    while(fcmd_line[i])
    {
        ++i;
        ++c;
    }

    strs[n] = (char*)malloc(c * sizeof(char));

    i = 0;
    for(n = 0; n < s - 1; ++n)
    {
        j = 0;
        while(fcmd_line[i] != '|')
        {
            strs[n][j] = fcmd_line[i];
            ++i;
            ++j;
        }
        strs[n][j] = 0;
        ++i;
    }
    j = 0;
    while(fcmd_line[i])
    {
        strs[n][j] = fcmd_line[i];
        ++i;
        ++j;
    }
    strs[n][j] = 0;

    //puts("");
    //for(i=0;i<(*strs_num); ++i) printf("\"%s\"\n", strs[i]);
    strcpy(cmd_line, fcmd_line);
    free(fcmd_line);
    return strs;
}

char **getargs(const char *cmd_line, int *args_num) //wyciaga z potokow pojedyncze instrukcje i zapisuje je do tablicy stringow
						   // argumentami jest string odsepaatowany przez sepstrs i liczba zapisanych argumentow 
{
    if(cmd_line == NULL) return NULL;
    else if(cmd_line[0] == 0) return NULL;

    int i = 0;
    (*args_num) = 0;

    while(cmd_line[i] != 0)
    {
        if(cmd_line[i] == 32) ++i;
        else
        {
            ++(*args_num);
            while(cmd_line[i] != 32 && cmd_line[i] != 0) ++i;
        }
    }

    if((*args_num) == 0) return NULL;

    char **args = (char**)malloc((*args_num+1) * sizeof(char*));

    i = 0;
    int j = 0, n = 0;
    while(cmd_line[i] != 0 )
    {
        while(cmd_line[i] == 32) ++i;
        while(cmd_line[i+j] != 32 && cmd_line[i+j] != 0) ++j;
        args[n] = (char*)malloc((j+1) * sizeof(char));
        //printf("%i\n", j);
        j = 0;

        while(cmd_line[i] != 32 && cmd_line[i] != 0)
        {
            args[n][j] = cmd_line[i];
            ++j;
            ++i;
        }

        args[n][j] = 0;
        j = 0;
        ++n;
    }
    args[n] = NULL;
    return args;
}

unsigned long int hash(char *str) //funkcja przypisuje poleceniom inikalne liczby. jako arg przekazywana jest tablica commands, w ktorej sa zhasowane wszystkie komendy
{
    unsigned long int hashval = 0, G;
    while (*str)
    {
        hashval = ( hashval << 4 ) + *str++;
        if (G = hashval & 0xF0000000L)
        {
            hashval ^= G >> 24;
        }
        hashval &= ~G;
    }
    return hashval;
}

void show_history(int sig)//obsluguje sygnal Sigusr. jezeli wykryje to wypisuje historie z pliku 
{
    if(sig == SIGUSR1)
    {
        puts("");
        int f = open("bash_history.txt", O_RDONLY);
        char buf[100];
        while(read(f, buf, 100)) write(1, buf, strlen(buf));
        puts("");
    }
}


int main(int argc, char *argv[]) //pobieranie polecenia, rozdzielanie do na członym wywolanie polecenia, zapisanie w historii 
{
    setlocale(LC_ALL, "Polish");
    const unsigned long int command[] =
    {
        hash("exit"),
        hash("quit"),
        hash("help"),
        hash("history"),
        hash("about"),
        hash("fifo")
    };

    char cmd[100];
    char **sstrs = NULL, **args = NULL;

    char hisloc[] = "bash_history.txt";
    char tmpname[] = "histmp";

    int strs_num = 0, args_num = 0, proc, i, j;

    int hisdes, tmpdes;
    int customout = 0;

    unsigned long int hashed_command;

    signal(SIGUSR1, show_history); //SIGUSR1- sygnał- wyświetli odpowiednia informację i zakończy działanie 
    int sigusr_handler = fork(); //obsluga sygnału , fork()-słuzy do tworzenia nowego procesu będącego kopią procesu wywołującego daną funkcję 
    if(sigusr_handler == 0) while(1) sleep(1);

    puts("Nasz shell.\n");
    if(argc >= 3)
    {
        for(i=0; i<100; ++i) cmd[i]=0;
        strcat(cmd, argv[1]);// sklejenie tablic, cmd- linia skryptu, argv[1]- !
        flag_script = 1;// mozna uruchomic 
    }
    else puts("Type \"help\" to see a list of commands.\n");

    while(1)
    {
        if(!flag_script)
        {
            //printf(">> ");
            fgets(cmd, 100, stdin); //czyta kolejne znaki z stdin i umieszcza je w tablicy wskazywanej przez cmd 
            cmd[strlen(cmd)-1] = 0;//ustawia dlugosc łancucha
        }

        flag_output = 0; //output- wydajnosc 
        sstrs = sepstrs(cmd, &strs_num);
        if(flag_script) printf("%s\n", cmd);
        if(flag_output) printf("Saved in %s\n", output);

        if(sstrs != NULL)
        {
            char c[1];
            hisdes = open(hisloc, O_RDONLY);
            if(hisdes != -1)
            {
                tmpdes = open(tmpname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                i = 0;
                while(read(hisdes, c, 1) && i < 19) //czytanie 20 poleceń w historii
                {
                    if(c[0] == '\n') ++i;
                    write(tmpdes, c, 1); // i zapisywanie 
                }
                close(hisdes);
                close(tmpdes);

                hisdes = open(hisloc, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                write(hisdes, cmd, strlen(cmd));
                write(hisdes, "\n", 1);
                tmpdes = open(tmpname, O_RDONLY);
                i = 0;
                while(read(tmpdes, c, 1))
                {
                    write(hisdes, c, 1);
                }
                close(tmpdes);
                close(hisdes);
                remove(tmpname);
            }
            else
            {
                hisdes = open(hisloc, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                write(hisdes, cmd, strlen(cmd));
                write(hisdes, "\n", 1);
                close(hisdes);
            }

            args = getargs(sstrs[0], &args_num);
            for(i=0; i<strlen(args[0]); ++i) args[0][i] = tolower(args[0][i]);
            hashed_command = hash(args[0]);
            if((hashed_command == command[0] || hashed_command == command[1]) && args_num == 1) return 0;
            else if(hashed_command == command[2] && args_num == 1)
            {
                puts("\n   exit, quit - closes bash");
                puts("   help - displays this help message");
                puts("   about - displays information about bash");
                puts("");
            }
		//wysyła sygnał procesu okreslony przez sigusr, jezeli SIGUSR =0 wykonywane sprawdzanie bledow 
            else if(hashed_command == command[3] && args_num == 1) kill(sigusr_handler, SIGUSR1); 

            else if(hashed_command == command[4] && args_num == 1)
                puts("\nBash developed by Karolina Grodzka, Ewa Kulesza, Damian Kiolbasa\n");
            else if(hashed_command == command[5] && args_num == 1)
            {
                mkfifo("fifoexample", 0666);//utworzy nowy plik FIFO
                if(fork() == 0) // jak dziecko jest kopia rodzica - czyli kopiowane sa obszary pamieci, wartosci zmiennych i czesc srodowiska 
                {
                    int p = open("fifoexample", O_WRONLY);//
                    write(p, "THIS IS FIFO!\n\0", 16);
                    exit(0);
                }
                int p = open("fifoexample", O_RDONLY);
                wait(NULL);
                char tmpbuf[16];
                read(p, tmpbuf, 16);
                write(1, tmpbuf, 16);
                unlink("fifoexample");
            }
            else
            {
                for(i=0; i<args_num; ++i) free(args[i]);
                free(args);
                args = NULL;

                proc = fork();
                if(proc == 0)
                {
                    customout = flag_output;
                    int p1[2], p2[2];
                    pipe(p1); //umieszcza dwa nowe deskryptory plikow - do odczytu i zapisu 
                    pipe(p2);
                    for(j = 0; j < strs_num; ++j)
                    {
                        proc = fork();
                        if(proc == 0)
                        {
                            args = getargs(sstrs[j], &args_num);

                            if(j < strs_num-1)
                            {
                                if(j%2 == 0)
                                {
                                    dup2(p1[1],1); //potoki- duplikacja deskryptora i zamkiecie poprzedniego 
                                    dup2(p2[0],0);
                                    close(p1[0]);
                                    close(p2[1]);
                                }
                                else
                                {
                                    dup2(p1[0],0);
                                    dup2(p2[1],1);
                                    close(p1[1]);
                                    close(p2[0]);
                                }
                            }
                            else
                            {
                                if(customout)
                                {
                                    int out = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                    dup2(out, 1);
                                    close(out);
                                }

                                if(j%2 == 0)
                                {
                                    dup2(p2[0],0);
                                    close(p1[0]);
                                    close(p1[1]);
                                    close(p2[1]);
                                }
                                else
                                {
                                    dup2(p1[0],0);
                                    close(p1[1]);
                                    close(p2[0]);
                                    close(p2[1]);
                                }
                            }

                            execvp(args[0], args); //udostepniaja tablice wskaznikow do ciagow zakonczonych znakiem null, pierwszy agreument- plik ? 
                            printf("%s\n", strerror(errno));
                            exit(0);
                        }
                    }
                    exit(0);
                }
                if(flag_backgroud == 0) wait(NULL);
                else flag_backgroud = 0;
            }
            for(i=0; i<strs_num; ++i) free(sstrs[i]);
            free(sstrs);
            sstrs = NULL;
            strs_num = 0;
            args_num = 0;
        }

        if(flag_script)
        {
            getchar();//czyta znak ze standardowego wejscia i go stamtad usuwa 
            return 0;
        }
    }
}
