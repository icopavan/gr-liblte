#include <iostream>
#include <signal.h>
#include "pbch_ue_wrapper.h"
using namespace std;

#define INSIZE 9600
char *input_file_name = NULL;
filesource_t fsrc;
int go_exit=0;

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "i")) != -1) {
    switch(opt) {
    case 'i':
      input_file_name = argv[optind];
      break;
    default:
      exit(-1);
    }
  }
}

void sigintHandler(int sig_num)
{
  go_exit=1;
}

int main(int argc,char **argv)
{
  parse_args(argc,argv);

  int n;
  pbch_ue_wrapper ue;
  cf_t* input_buffer = (cf_t*) malloc(INSIZE * sizeof(cf_t));

  if (filesource_init(&fsrc, input_file_name, COMPLEX_FLOAT_BIN)) {
    return -1;
  }

  printf("\n --- Press Ctrl+C to exit --- \n");
  signal(SIGINT, sigintHandler);

  while(!go_exit) {
    if (input_file_name) {
      n = filesource_read(&fsrc, input_buffer, INSIZE);
      if (n == -1) {
        fprintf(stderr, "Error reading file\n");
        exit(-1);
      } else if (n < INSIZE) {
        filesource_seek(&fsrc, 0);
        filesource_read(&fsrc, input_buffer, INSIZE);
      }
    }

    ue.process(input_buffer, INSIZE);
    usleep(5000);
  }

  filesource_free(&fsrc);
  free(input_buffer);
  return 0;
}
