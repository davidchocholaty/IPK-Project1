#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#define ARGSCNT 2
#define ARGIDX 1
#define BUFFERSIZE 1024
#define HOSTCPUENDIDX 13
#define LOADENDIDX 9
#define CPUNAMESIZE 128
#define SLEEPTIME 1
#define PROC_LINE_LENGTH 4096

#define VALIDRES "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n"
#define BADRES "400 Bad Request"

//TODO print info about line 340
//TODO format of HTTP response (Content-Length ?, ...)

/*
 * Hostname retrieval function
 *
 * @param cpu_name Buffer for cpu name
 * @param size     Size of cpu_name buffer
 * @return         Status of function processing
 * 
 */
int set_cpu_name (char *cpu_name, int size)
{
    FILE *ptr_prev_stream = popen("grep \"model name\" /proc/cpuinfo | head -n 1 | awk '{ print substr($0, index($0,$4)) }'", "r");

    if (ptr_prev_stream == NULL)
    {
        return EXIT_FAILURE;
    }
    
    if (!fgets(cpu_name, size, ptr_prev_stream))
    {
        cpu_name[0] = '\0';
    }
    else
    {
        /*
         * Delete new line character at the end of cpu name
         *
         * Source: https://stackoverflow.com/questions/1472537/c-trimming-newline-character-when-reading-input-from-file
         */        
        char *new_line = strchr(cpu_name, '\n');
        if (new_line)
        *new_line = 0;
    }

    if (pclose(ptr_prev_stream) < 0)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Function for calculate usage at one
 * specific time
 *
 * @param ptr_stream Stream with raw data
 * @param total      Variable for set total value
 * @param idle_res   Variable for set idle value
 * @return           Status of function processing
 * 
 */
int calculate_usage (FILE* ptr_stream,
                     unsigned long long int *total,
                     unsigned long long int *idle_res)
{
    unsigned long long int user, nice, system, idle;
    unsigned long long int iowait, irq, softirq, steal;
    unsigned long long int non_idle;

    iowait = irq = softirq = steal = 0;

    if (ptr_stream == NULL)
    {
        return EXIT_FAILURE;
    }

    char buffer[PROC_LINE_LENGTH + 1];

    if (!fgets(buffer, PROC_LINE_LENGTH, ptr_stream))
    {
        buffer[0] = '\0';
    }
    
    sscanf(buffer, "%16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);    

    /*************** CALCULATION ***************/    

    *idle_res = idle + iowait;
    non_idle = user + nice + system + irq + softirq + steal;
    *total = *idle_res + non_idle;

    return EXIT_SUCCESS;
}

/*
 * Function for measuring values
 * after specific time period
 *
 * @param prev_total Variable for previous total value
 * @param prev_idle  Variable for previous idle value
 * @param act_total  Variable for actual total value
 * @param act_idle   Variable for actual idle value
 * @return           Status of function processing
 * 
 */
int set_measurement_vals (unsigned long long int *prev_total,
                          unsigned long long int *prev_idle,
                          unsigned long long int *act_total,
                          unsigned long long int *act_idle)
{     
    /* Previous measurement */
    FILE *prev_stream = popen("grep \"cpu\" /proc/stat | head -n 1 | awk '{ print substr($0, index($0,$2)) }'", "r");

    sleep(SLEEPTIME);

    /* Actual measurement */
    FILE *act_stream = popen("grep \"cpu\" /proc/stat | head -n 1 | awk '{ print substr($0, index($0,$2)) }'", "r");
    
    /* Usage calculation of previous values */
    if (calculate_usage(prev_stream, prev_total, prev_idle) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    
    /* Usage calculation of actual values */
    if (calculate_usage(act_stream, act_total, act_idle) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    if (pclose(prev_stream) < 0 ||
        pclose(act_stream) < 0)
    {
        return EXIT_FAILURE;
    }    

    return EXIT_SUCCESS;
}

/*
 * CPU usage calculation function
 * @param result Variable for result CPU usage
 * @return       Status of function processing
 * 
 * Source: https://github.com/htop-dev/htop/blob/15652e7b8102e86b3405254405d8ee5d2a239004/linux/LinuxProcessList.c
 * Source: https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
 * 
 */
int set_cpu_usage (unsigned int *result)
{
    unsigned long long int prev_total, prev_idle;
    unsigned long long int act_total, act_idle;
    unsigned long long int total, idle;    

    /* Set measurement values */
    if (set_measurement_vals(&prev_total, &prev_idle, &act_total, &act_idle) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }        
    
    /* CPU Usage Calculation */
    total = act_total - prev_total;
    idle = act_idle - prev_idle;

    double cpu_usage = (double)(total-idle)/total*100.0;

    *result = (unsigned int)cpu_usage;    

    return EXIT_SUCCESS;
}

/*
 * Convert unsigned int value of CPU usage
 * to string
 * 
 * @param cpu_str   Buffer for CPU usage string
 * @param cpu_usage Value of CPU usage
 * @return          Status of function processing
 *  
 */
int cpu_usg_2_str(char *cpu_str, unsigned int cpu_usage)
{
    if (sprintf(cpu_str, "%d%%", cpu_usage) < 0)
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

/*
 * Main function of server
 *
 */
int main (int argc, char *argv[])
{
    int welcome_socket;
    int optval;
    unsigned long port_number;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int con_socket;
    
    /**************** GET ARGS *****************/

    if (argc != ARGSCNT)
    {
        fprintf(stderr,"BAD COUNT OF ARGS\n"); 
        exit(EXIT_FAILURE);
    }
    else
    {
        char *ptr_end;
        port_number = strtoul(argv[ARGIDX], &ptr_end, 10);

        if (port_number == 0L || errno == ERANGE)
        {
            fprintf(stderr, "BAD ARGUMENT VALUE\n");
            exit(EXIT_FAILURE);
        }
        else if (port_number > USHRT_MAX)
        {
            fprintf(stderr, "ARGUMENT NOT IN RANGE\n");      
            exit(EXIT_FAILURE);
        }
    }
        
    /************** CREATE SOCKET **************/

    if ((welcome_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("SOCKET ERROR");
        exit(EXIT_FAILURE);
    }
    
    optval = 1;
    setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));
    
    /****************** BIND *******************/
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port_number);

    if (bind(welcome_socket, (struct sockaddr *) &server_addr,
        sizeof(server_addr)) < 0)
    {
        perror("BIND ERROR");
        exit(EXIT_FAILURE);
    }
    
    /***************** LISTEN ******************/

    if (listen(welcome_socket, 1) < 0)
    {
        perror("LISTEN ERROR");
        exit(EXIT_FAILURE);
    }    
    
    /***************** ACCEPT ******************/

    socklen_t client_addr_len = sizeof(client_addr);

    while (1)
    {
        con_socket = accept(welcome_socket, (struct sockaddr*) &client_addr, &client_addr_len);

        if (con_socket > 0)
        {
            char str[INET_ADDRSTRLEN];

            /* Set to non-blocking mode */
            int flags = fcntl(con_socket, F_GETFL, 0);

            if (fcntl(con_socket, F_SETFL, flags | O_NONBLOCK) < 0)
			{
				perror("FCNTL ERROR");
				exit(EXIT_FAILURE);								
			}

            char buffer[BUFFERSIZE];
            char response[BUFFERSIZE];

            char input[BUFFERSIZE];            

            for (;;)
            {
                fd_set fds;
				FD_ZERO(&fds);
				FD_SET(con_socket, &fds);
				FD_SET(STDIN_FILENO, &fds);	
				
                if (select(con_socket + 1, &fds, NULL, NULL, NULL) <= 0) continue; 
                
                if (FD_ISSET(con_socket, &fds))
                {
                    int res = recv(con_socket, buffer, BUFFERSIZE,0);				
                    
                    if (res > 0)
                    {
                        /* Print request */
                        buffer[res] = '\0';
                        printf("%s",buffer);

                        buffer[HOSTCPUENDIDX] = '\0';

                        if (strcmp(buffer, "GET /hostname") == 0)
                        {
                            /* Set HTTP msg */
                            strcpy(response, VALIDRES);

                            char hostname[HOST_NAME_MAX + 1];

                            if (gethostname(hostname, HOST_NAME_MAX + 1) != 0)
                            {
                                perror("HOSTNAME ERROR");
                                exit(EXIT_FAILURE);
                            }
                                                                                              
                            strcat(response, hostname);                            
                            send(con_socket, response, strlen(response), 0);                                                        

                            //TODO
                            //printf("INFO: Closing conection to %s.\n", str);
                            close(con_socket);						
                            break;
                        }
                        else if (strcmp(buffer, "GET /cpu-name") == 0)
                        {
                            /* Set HTTP msg */
                            strcpy(response, VALIDRES);
                            char cpu_name[CPUNAMESIZE];

                            if (set_cpu_name(cpu_name, CPUNAMESIZE) != EXIT_SUCCESS)
                            {
                                perror("HOSTNAME ERROR");
                                exit(EXIT_FAILURE);
                            }

                            strcat(response, cpu_name);
                            send(con_socket, response, strlen(response), 0);

                            close(con_socket);						
                            break;
                        }
                        else
                        {
                            buffer[LOADENDIDX] = '\0';

                            if (strcmp(buffer, "GET /load") == 0)
                            {
                                /* Set HTTP msg */
                                strcpy(response, VALIDRES);                                
                                
                                unsigned int cpu_usage;

                                if (set_cpu_usage(&cpu_usage) != EXIT_SUCCESS)
                                {
                                    perror("CPU USAGE ERROR");
                                    exit(EXIT_FAILURE);
                                }                                                                

                                char cpu_str[4];                                

                                if (cpu_usg_2_str(cpu_str, cpu_usage) != EXIT_SUCCESS)
                                {
                                    perror("CPU USAGE ERROR");
                                    exit(EXIT_FAILURE);
                                }

                                strcat(response, cpu_str);
                                send(con_socket, response, strlen(response), 0);

                                close(con_socket);						
                                break;                                                                                           
                            }
                            else
                            {
                                /* Set HTTP msg */
                                strcpy(response, BADRES);
                                close(con_socket);						
                                break;                        
                            }                           
                        }                    
                    }
                    else if (res == 0)
                    {
                        printf("INFO: %s closed connection.\n", str);
                        close(con_socket);						
                        break;                  
                    }
                }
				
				if (FD_ISSET(STDIN_FILENO, &fds))
				{
				    bzero(input, BUFFERSIZE);
				    fgets(input, BUFFERSIZE, stdin);
				    send(con_socket, input, strlen(input), 0);        
				}                
            }           
        }            
    }
    
    return 0;
}
