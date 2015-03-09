#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <stdio.h>
#include <string.h>
#include "ST7735.h"
#include "ADC.h"
#include "Perf.h"
#define COMMAND(NAME) { #NAME, Command_ ## NAME }
#define DECL_COMMAND(NAME) int Command_ ## NAME ## ( char* );

// #define DEF_COMMAND(NAME) int Command_ ## NAME ## ( char * param_str ) { \
//   char id[16]; \
//   char args[64]; \
//   const char* ptr = param_str; \
//   sscanf(ptr, "%s %63s", id, args); \

int Command_disp_message(char * args);
int Command_list(char * args);
int Command_adc_open(char *args);
int Command_adc_in(char *args);
int Command_perf(char* args);
int Command_x(char* args);
int Command_y(char* args);

struct command
{
  char *label;
  int (*function) (char *);
};

struct command commands[] =
{
  COMMAND(list),
  COMMAND(disp_message),
  COMMAND(adc_open),
  COMMAND(adc_in),       //{"adc_in", Command_adc_in}
  COMMAND(perf),
  COMMAND(x),
  COMMAND(y)
  //COMMAND(not_found)

};



/*
 Want support for get, set, route, and list
*/

char buffer[64];
  char m_command[16];
void interpreter(void){
  char *buff_ptr = buffer;
  struct command* command_ptr = commands;
  struct command* end_ptr = command_ptr + sizeof(commands)/sizeof(commands[0]);
  int (*function)(char *);
	int n;
  printf(">> ");
  //scanf("%s\n", buffer); //, buffer);
	fgets(buffer, 64, stdin);
	sscanf(buff_ptr, "%s%n", m_command, &n);
	printf("m_command: %s\n", m_command);
    buff_ptr += n;
	printf("%s\n", buff_ptr);
    //printf("Buffer: %s\n", buff_ptr);
  while (command_ptr < end_ptr){
    function = command_ptr->function;
    if(strcmp(m_command, command_ptr->label) == 0){
//			printf("Doing Command: %s\n", command_ptr->label);
      function(buff_ptr);
			
      return;
    }

    command_ptr++;
  }
  printf("[Warning] Interpreter Command not found\n");
  return;
}


// ====================================================
int Command_list(char * args)
{
  struct command* command_ptr = commands;
  struct command* end_ptr = command_ptr + sizeof(commands)/sizeof(commands[0]);

  while(command_ptr < end_ptr){
    printf("%s\n", command_ptr->label);
    command_ptr++;
  }
  return 0;
}

int Command_disp_message(char * args)
{
  //printf("Not implemented yet\n");
  int disp, line;//, n;
  char str[64];
  sscanf(args, "%d %d %[^\t\r\n]", &disp, &line, str);
	//str = args + n;
	printf("\n Got disp=%d\tline=%d, %s\n", disp, line, str);
  ST7735_Message(disp, line, str, 0);    

  return -1;
}

int Command_adc_open(char *args)
{
  unsigned int channel;
  sscanf(args, "%u", &channel);
  return ADC_Open(channel);
}

int Command_adc_in(char *args)
{
  unsigned short result;
  result = ADC_In();
  printf("ADC value: %f\n", ((float) result) *3.3/4096.0);
  return result;
}


int Command_perf(char* args){
  printf("\nJitter: %d\nNumSamples: %d\nNumCreated: %d\nDataLost: %d\n", jitter, NumSamples, NumCreated, DataLost  );
	return 1;
}
int Command_x(char* args){
	int i;
  for(i = 0; i < 64; i++){
    printf("\n");
    printf("%ld\t", (3000*x[i])/4096);
  }
  return 1;
}
int Command_y(char* args){
	int i;
  for( i = 0; i < 64; i++){
    printf("\n");
    printf("%ld\t", y[i]);
  }
  return 1;
}

#endif /* __INTERPRETER_H__ */
