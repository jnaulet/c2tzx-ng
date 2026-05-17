#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define TZX_ZXTAPE "ZXTape!\x1a\x01\x14"

/* TODO: make start_addr a parameter */
static const char program[] = {
  "\x00\x0a\x0e\x00 \xfd""32768\x0e\x00\x00\x00\x80\x00\r"     /* 10 CLEAR 32768 */
  "\x00\x14\x07\x00 \xef\"\" \xaf\r"                           /* 20 LOAD "" CODE */
  "\x00\x1e\x03\x00 \xfb\r"                                    /* 30 CLS */
  "\x00\x28\x0f\x00 \xf9\xc0""32768\x0e\x00\x00\x00\x80\x00\r" /* 40 RANDOMIZE USR 32768 */
};

/* fix terminating zeroes */
#define SIZEOF_TZX_ZXTAPE (sizeof(TZX_ZXTAPE) - 1)
#define SIZEOF_PROGRAM (sizeof(program) - 1)

struct tzx_std_speed_data_block {
  uint8_t id; /* 0x10 */
  uint8_t pause_ms_l;
  uint8_t pause_ms_h;
  uint8_t len_l;
  uint8_t len_h;
} __attribute__ ((packed));

struct tap_block_header {
  uint8_t flag;        /* 0x00 for headers, 0xff for data blocks */
  uint8_t type;        /* 0 (program), 1 (num array), 2 (char array) or 3 (code) */
  char filename[10];
  uint8_t block_len_l; /* len without flag & csum */
  uint8_t block_len_h;
  uint8_t param1_l;
  uint8_t param1_h;
  uint8_t param2_l;
  uint8_t param2_h;
  uint8_t csum;
} __attribute__ ((packed));

static uint8_t xcsum(uint8_t sum, const uint8_t *buf, size_t n)
{
  while(n-- != 0)
    sum ^= *buf++;

  return sum;
}

static int tzx_file_header(int fd)
{
  return (int)write(fd, TZX_ZXTAPE, SIZEOF_TZX_ZXTAPE);
}

static int tzx_std_speed_block(int fd, unsigned pause_ms, size_t len)
{
  struct tzx_std_speed_data_block tzx;
  
  tzx.id = (uint8_t)0x10; /* std speed data block */
  tzx.pause_ms_l = (uint8_t)pause_ms;
  tzx.pause_ms_h = (uint8_t)(pause_ms >> 8);
  tzx.len_l = (uint8_t)len;
  tzx.len_h = (uint8_t)(len >> 8);

  /* FIXME: make a write_safe() function */
  return (int)write(fd, &tzx, sizeof(tzx));
}

static int tap_program_block_header(int fd)
{
  struct tap_block_header tap;
  
  tap.flag = (uint8_t)0x0; /* header */
  tap.type = (uint8_t)0x0; /* program */
  memcpy(tap.filename, " c2tzx-ng ", sizeof(tap.filename));
  tap.block_len_l = (uint8_t)SIZEOF_PROGRAM;
  tap.block_len_h = (uint8_t)(SIZEOF_PROGRAM >> 8);
  /* autostart line nb */
  tap.param1_l = (uint8_t)0xa; /* LINE 10 */
  tap.param1_h = (uint8_t)0x0;
  /* start of variable area */
  tap.param2_l = (uint8_t)SIZEOF_PROGRAM;
  tap.param2_h = (uint8_t)(SIZEOF_PROGRAM >> 8);
  tap.csum = xcsum(0, (uint8_t*)&tap, sizeof(tap) - 1);
  
  return (int)write(fd, &tap, sizeof(tap));
}

static int tap_code_block_header(int fd, unsigned start_addr, size_t block_len)
{
  struct tap_block_header tap;

  /* tap */
  tap.flag = 0x0; /* header */
  tap.type = 0x3; /* code */
  memcpy(tap.filename, ".code     ", sizeof(tap.filename));
  tap.block_len_l = (uint8_t)block_len;
  tap.block_len_h = (uint8_t)(block_len >> 8);
  tap.param1_l = (uint8_t)start_addr;
  tap.param1_h = (uint8_t)(start_addr >> 8);
  /* mandatory for code block */
  tap.param2_l = (uint8_t)32768;
  tap.param2_h = (uint8_t)(32768 >> 8);
  tap.csum = xcsum(0, (uint8_t*)&tap, sizeof(tap) - 1);

  return (int)write(fd, &tap, sizeof(tap));
}

static int tap_block_data(int fd, const void *data, size_t n)
{
  int ret = 0;
  uint8_t flag = (uint8_t)0xff; /* data */
  uint8_t csum = xcsum(0xff, data, n);
  
  /* FIXME */
  ret += (int)write(fd, &flag, sizeof(flag));
  ret += (int)write(fd, data, n);
  ret += (int)write(fd, &csum, sizeof(csum));
  
  return ret;
}

static void print_usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [options]\n", argv0);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -s/--start start_addr: specify code start addr, (defalult: 32768)\n");
  fprintf(stderr, "  -i/--input file.bin: specify input file, (defalult: stdin)\n");
  fprintf(stderr, "  -o/--output file.tzx: specify output file, (defalult: stdout)\n");
  fprintf(stderr, "  -h/--help: display this message\n");
  fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
  /* defaults */
  unsigned start = 0x8000u;
  int fd_in = fileno(stdin);
  int fd_out = fileno(stdout);

  /* max load is 32k for the moment */
  ssize_t n;
  static char data[32768];
  
  int c;
  int option_index = 0;
  static struct option long_options[] = {
    {"start",  required_argument, 0, 's' },
    {"input",  required_argument, 0, 'i' },
    {"output", required_argument, 0, 'o' },
    {"help",   no_argument,       0, 'h' },
    { 0, 0, 0, 0 }
  };

  while((c = getopt_long(argc, argv, "s:i:o:h",
                         long_options, &option_index)) != EOF){
    switch(c){
    case 's': start = (unsigned)strtoul(optarg, NULL, 10); break;
    case 'i': fd_in = open(optarg, O_RDONLY); break;
    case 'o': fd_out = open(optarg, O_CREAT | O_WRONLY | O_TRUNC, 0644); break;
    case 'h': print_usage(argv[0]); return 1;
    default:
      fprintf(stderr, "Unknown parameter %c\n", (char)c);
      print_usage(argv[0]);
      return 1;
    }
  }

  /* check */
  if(fd_in < 0 || fd_out < 0 || start > 0xffffu){
    fprintf(stderr, "Bad parameter !\n");
    print_usage(argv[0]);
    return 1;
  }

  /* TODO: check min start */
  /* TODO: check start + file size */
  
  /* main conversion */
  (void) tzx_file_header(fd_out);
  
  (void) tzx_std_speed_block(fd_out, 1000u, sizeof(struct tap_block_header));
  (void) tap_program_block_header(fd_out);
  (void) tzx_std_speed_block(fd_out, 0u, SIZEOF_PROGRAM + 2); /* +flag +csum */
  (void) tap_block_data(fd_out, program, SIZEOF_PROGRAM);

  if((n = read(fd_in, data, sizeof(data))) < 0){
    perror("read");
    return 1;
  }
  
  (void) tzx_std_speed_block(fd_out, 1000u, sizeof(struct tap_block_header));
  (void) tap_code_block_header(fd_out, start, (size_t)n);
  (void) tzx_std_speed_block(fd_out, 1000u, (size_t)n + 2); /* +flag +csum */
  (void) tap_block_data(fd_out, data, (int) n);

  (void) close(fd_out);
  (void) close(fd_in);
  return 0;
}
