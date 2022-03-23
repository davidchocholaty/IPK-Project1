/**********************************************************/
/*                                                        */
/* File: hinfosvc.c                                       */
/* Created: 2022-02-10                                    */
/* Last change: 2022-02-12                                */
/* Author: David Chocholaty <xchoch09@stud.fit.vutbr.cz>  */
/* Project: Project 1 for course IPK                      */
/* Description: Server that communicates via HTTP         */
/*                                                        */
/**********************************************************/

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

#define ARGS_CNT 2
#define ARG_IDX 1
#define BUFFER_SIZE 1024
#define HOST_CPU_END_IDX 14
#define LOAD_END_IDX 10
#define CPU_NAME_SIZE 128
#define SLEEP_TIME 1
#define PROC_LINE_LENGTH 4096

#define CPU_NAME_CMND "grep \"model name\" /proc/cpuinfo | head -n 1 | awk '{ print substr($0, index($0,$4)) }'"
#define CPU_USAGE_CMND "grep \"cpu\" /proc/stat | head -n 1 | awk '{ print substr($0, index($0,$2)) }'"

#define VALID_RES_STATUS "HTTP/1.1 200 OK\r\n"
#define BAD_RES_STATUS "HTTP/1.1 400 Bad Request\r\n"
#define NOT_FOUND_RES_STATUS "HTTP/1.1 400 Not Found\r\n"
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_TYPE "Content-Type: text/plain\r\n\r\n"

/*
 * Hostname retrieval function
 *
 * @param cpu_name Buffer for cpu name
 * @param size     Size of cpu_name buffer
 * @return         Status of function processing
 */
int set_cpu_name (char *cpu_name, int size)
{
    FILE *ptr_stream = popen(CPU_NAME_CMND, "r");

    if (ptr_stream == NULL)
    {
        return EXIT_FAILURE;
    }
    
    if (!fgets(cpu_name, size, ptr_stream))
    {
        cpu_name[0] = '\0';
    }
    else
    {
        /*
         * Delete new line character at the end of cpu name     
         */
        char *new_line = strchr(cpu_name, '\n');

        if (new_line) 
        {
            *new_line = '\0';
        }
    }

    if (pclose(ptr_stream) < 0)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Function for usage calculation at one
 * specific time
 *
 * @param ptr_stream Stream with raw data
 * @param total      Variable for set total value
 * @param idle_res   Variable for set idle value
 * @return           Status of function processing
 * 
 * Source: https://github.com/htop-dev/htop/blob/15652e7b8102e86b3405254405d8ee5d2a239004/linux/LinuxProcessList.c
 * Source: https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
 */
int calculate_usage (FILE* ptr_stream,
                     unsigned long long int *total,
                     unsigned long long int *idle_res)
{
    unsigned long long int user, nice, system, idle;
    unsigned long long int iowait, irq, softirq, steal;
    unsigned long long int non_idle;
    char buffer[PROC_LINE_LENGTH + 1];

    iowait = irq = softirq = steal = 0;

    if (ptr_stream == NULL)
    {
        return EXIT_FAILURE;
    }    

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
 */
int set_measurement_vals (unsigned long long int *prev_total,
                          unsigned long long int *prev_idle,
                          unsigned long long int *act_total,
                          unsigned long long int *act_idle)
{
    /* Previous measurement */
    FILE *prev_stream = popen(CPU_USAGE_CMND, "r");

    /* Sleep for specific time period */
    sleep(SLEEP_TIME);

    /* Actual measurement */
    FILE *act_stream = popen(CPU_USAGE_CMND, "r");
    
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
 */
int set_cpu_usage (unsigned int *result)
{
    unsigned long long int prev_total, prev_idle;
    unsigned long long int act_total, act_idle;
    unsigned long long int total, idle;
    double cpu_usage;   

    /* Set measurement values */
    if (set_measurement_vals(&prev_total, &prev_idle, &act_total, &act_idle) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }        
    
    /* CPU Usage Calculation */
    total = act_total - prev_total;
    idle = act_idle - prev_idle;

    cpu_usage = (double)(total-idle)/total*100.0;

    *result = (unsigned int)cpu_usage;    

    return EXIT_SUCCESS;
}

/*
 * Convert unsigned int value of CPU usage
 * to string with percent character
 * 
 * @param cpu_str   Buffer for CPU usage string
 * @param cpu_usage Value of CPU usage
 * @return          Status of function processing
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
 * Function for create response
 * with hostname query
 * 
 * @param response   Buffer for response message
 * @param con_socket Connection socket
 * @return           Status of function processing
 */
int hostname_response (char *response, int con_socket)
{
    char hostname[HOST_NAME_MAX + 1];        

    if (gethostname(hostname, HOST_NAME_MAX + 1) != 0)
    {
        return EXIT_FAILURE;
    }

    /* Set HTTP response */
    sprintf(response,"%s%s%ld\r\n%s%s", VALID_RES_STATUS, CONTENT_LENGTH, strlen(hostname), CONTENT_TYPE, hostname);               
    send(con_socket, response, strlen(response), 0);    

    return EXIT_SUCCESS;
}

/*
 * Function for create response
 * with cpu-name query
 * 
 * @param response   Buffer for response message
 * @param con_socket Connection socket
 * @return           Status of function processing
 */
int cpu_name_response (char *response, int con_socket)
{
    char cpu_name[CPU_NAME_SIZE];        

    if (set_cpu_name(cpu_name, CPU_NAME_SIZE) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    /* Set HTTP response */
    sprintf(response,"%s%s%ld\r\n%s%s", VALID_RES_STATUS, CONTENT_LENGTH, strlen(cpu_name), CONTENT_TYPE, cpu_name);
    send(con_socket, response, strlen(response), 0);    

    return EXIT_SUCCESS;
}

/*
 * Function for create response
 * with load query
 * 
 * @param response   Buffer for response message
 * @param con_socket Connection socket
 * @return           Status of function processing
 */
int load_response (char *response, int con_socket)
{
    unsigned int cpu_usage;
    char cpu_str[4];    

    if (set_cpu_usage(&cpu_usage) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    if (cpu_usg_2_str(cpu_str, cpu_usage) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;        
    }

    /* Set HTTP response */
    sprintf(response,"%s%s%ld\r\n%s%s", VALID_RES_STATUS, CONTENT_LENGTH, strlen(cpu_str), CONTENT_TYPE, cpu_str);    
    send(con_socket, response, strlen(response), 0);    

    return EXIT_SUCCESS;
}

/*
 * Main function of server
 */
int main (int argc, char *argv[])
{
    int welcome_socket;
    int con_socket;
    int optval;
    int flags;
    unsigned long port_number;    
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];    
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    
    /**************** GET ARGS *****************/

    if (argc != ARGS_CNT)
    {
        fprintf(stderr,"BAD COUNT OF ARGS\nusage: %s <port>\n", argv[0]); 
        exit(EXIT_FAILURE);
    }
    else
    {
        char *ptr_end;
        port_number = strtoul(argv[ARG_IDX], &ptr_end, 10);

        if (port_number == 0L)
        {
            fprintf(stderr, "BAD ARGUMENT VALUE\n");
            exit(EXIT_FAILURE);
        }
        else if (errno == ERANGE || port_number > USHRT_MAX)
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
            /* Set to non-blocking mode */
            flags = fcntl(con_socket, F_GETFL, 0);

            if (fcntl(con_socket, F_SETFL, flags | O_NONBLOCK) < 0)
			{
				perror("FCNTL ERROR");
				exit(EXIT_FAILURE);								
			}                       

            for (;;)
            {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(con_socket, &fds);
                FD_SET(STDIN_FILENO, &fds);	
				
                if (select(con_socket + 1, &fds, NULL, NULL, NULL) <= 0)
                {
                    continue;
                }
                
                if (FD_ISSET(con_socket, &fds))
                {
                    int res = recv(con_socket, buffer, BUFFER_SIZE, 0);                    

                    if (res > 0)
                    {
                        /* Set end of string for following comparing */
                        buffer[HOST_CPU_END_IDX] = '\0';
                        
                        /**************** HOSTNAME *****************/
                        if (strcmp(buffer, "GET /hostname ") == 0)
                        {
                            if (hostname_response(response, con_socket) != EXIT_SUCCESS)
                            {
                                perror("HOSTNAME ERROR");
                                exit(EXIT_FAILURE);
                            }

                            /* Read the remaining data from socket */
                            while (recv(con_socket, buffer, BUFFER_SIZE, 0) > 0);

                            break;
                        }                      
                        /**************** CPU NAME *****************/
                        else if (strcmp(buffer, "GET /cpu-name ") == 0)
                        {
                            if (cpu_name_response(response, con_socket) != EXIT_SUCCESS)
                            {
                                perror("HOSTNAME ERROR");
                                exit(EXIT_FAILURE);
                            }

                            /* Read the remaining data from socket */
                            while (recv(con_socket, buffer, BUFFER_SIZE, 0) > 0);
                            				
                            break;
                        }
                        else
                        {
                            /* Set end of string for following comparing */
                            buffer[LOAD_END_IDX] = '\0';
                            
                            /****************** LOAD *******************/
                            if (strcmp(buffer, "GET /load ") == 0)
                            {
                                if (load_response(response, con_socket) != EXIT_SUCCESS)
                                {
                                    perror("CPU USAGE ERROR");
                                    exit(EXIT_FAILURE);
                                }

                                /* Read the remaining data from socket */
                                while (recv(con_socket, buffer, BUFFER_SIZE, 0) > 0);

                                break;
                            }
                            else
                            {
                                /* Set HTTP msg */
                                sprintf(response,"%s%s", BAD_RES_STATUS, CONTENT_TYPE);
                                send(con_socket, response, strlen(response), 0);

                                /* Read the remaining data */
                                while (recv(con_socket, buffer, BUFFER_SIZE, 0) > 0);

                                break;
                            }                           
                        }
                    }
                    else if (res <= 0)
                    {
                        sprintf(response,"%s%s", NOT_FOUND_RES_STATUS, CONTENT_TYPE);
                        send(con_socket, response, strlen(response), 0);
                        break;
                    } 
                }
            }

            close(con_socket);           
        }      
    }
    
    return EXIT_SUCCESS;
}
