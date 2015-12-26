/**
 * unstickpixels – screen loop to try to unstick stuck pixels
 * 
 * Copyright © 2013, 2015  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>



#define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)


#define WELCOME_MESSAGE								\
  "\033[?25l\033[H\033[2J"							\
  "\033[H\033[01;31mWARNING: Do not used this if you have epilepsia.\033[00m\n"	\
  "\n"										\
  "It is recommended to massage the defective dots, whilst\n"			\
  "running this program.\n"							\
  "\n"										\
  "Press C-c to quit, and Enter to start (C-c will quit).\n"			\
  "\n"										\
  "\n"										\
  "You should not have to run this program. If the manufacture or\n"		\
  "reseller of your monitor refuse to replace or repair the monitor\n"  	\
  "that was broken, because of manufacturing error, when it was\n"		\
  "chiped to you, please consider yelling at the support, threaten\n"		\
  "to report the personally because of involvement in organised\n"		\
  "crime (not replacing hardware that was chipped broken when you\n"		\
  "selled it is a crime, and they do this in an organised fashion\n"		\
  "to make money), call their executive director persistently at\n"		\
  "inconvenient times, report them applicable board for consumer\n"		\
  "disputes/complaints or equivalent agency. If this program does\n"		\
  "not help, and the manufacture is Packard Bell, or its\n"			\
  "supersidiary Acer, tell them that you have tried a program\n"		\
  "that was prompted by the complete suckness and incapability\n"		\
  "to repair defects that they have agreed to repair. They suck!\n"		\
  "\n"										\
  "\n"										\
  "\n"										\
  "Copyright © 2013, 2015  Mattias Andrée (maandree@member.fsf.org)\n"  	\
  "\n"										\
  "This program is free software: you can redistribute it and/or modify\n"	\
  "it under the terms of the GNU General Public License as published by\n"	\
  "the Free Software Foundation, either version 3 of the License, or\n"		\
  "(at your option) any later version.\n"					\
  "\n"										\
  "This program is distributed in the hope that it will be useful,\n"		\
  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"		\
  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"		\
  "GNU General Public License for more details.\n"				\
  "\n"										\
  "You should have received a copy of the GNU General Public License\n"		\
  "along with this program.  If not, see <http://www.gnu.org/licenses/>."


/**
 * The name of the process.
 */
const char* argv0;



/**
 * Parse interval argument.
 * 
 * @param   argv      The interval argument.
 * @param   interval  Output parameter for the interval.
 * @param   nonzero   With be set to zero if the interval is zero,
 *                    otherwise it is set to a non-zero value (1).
 * @return            0 on success, -1 on error.
 */
static int get_interval(const char* arg, struct timespec* interval, int* nonzero)
{
  char* end;
  *nonzero = 1;
  t (!isdigit(*arg));
  interval->tv_sec = (time_t)(errno = 0, strtol)(arg, &end, 10);
  t (errno || *end);
  interval->tv_nsec = interval->tv_sec % 1000;
  interval->tv_sec /= 1000;
  interval->tv_nsec *= 1000000L;
  if (interval->tv_sec == 0)
    if (interval->tv_nsec == 0)
      *nonzero = 0;
  return 0;
 fail:
  return -1;
}


/**
 * Wrapper for fork(3) that clocks all signals in the parent.
 * 
 * @return  See fork(3).
 */
static pid_t xfork(void)
{
  sigset_t set;
  pid_t pid;
  sigfillset(&set);
  sigprocmask(SIG_BLOCK, &set, 0);
  pid = fork();
  if (pid != -1)
    sigprocmask(SIG_UNBLOCK, &set, 0);
  return pid;
}


int main(int argc, char* argv[])
{
  int started = 0;
  int with_sleep = 0;
  struct timespec interval;
  int _status;
  pid_t pid;
  
  argv0 = (argc ? "unstickpixels" : *argv);
  
  if (argc > 1)
    t (argv[1], &interval, &with_sleep);
  
  printf(WELCOME_MESSAGE);
  fflush(stdout);
  
  if (read(STDIN_FILENO, &started, 1) < 0)
    goto done;
  started = 1;
  
  pid = xfork();
  if (pid == -1)
    return perror(argv0), 1;
  else if (pid)
    goto parent;
  
  t (printf("\033[H") == -1);
  t (fflush(stdout));
  
  for (;;)
    {
      t (printf("\033]P0FF0000\033[2J") == -1);
      t (fflush(stdout));
      if (with_sleep)
	clock_nanosleep(CLOCK_MONOTONIC, 0, &interval, NULL);
      
      t (printf("\033]P000FF00\033[2J") == -1);
      t (fflush(stdout));
      if (with_sleep)
	clock_nanosleep(CLOCK_MONOTONIC, 0, &interval, NULL);
      
      t (printf("\033]P00000FF\033[2J") == -1);
      t (fflush(stdout));
      if (with_sleep)
	clock_nanosleep(CLOCK_MONOTONIC, 0, &interval, NULL);
    }
  
 fail:
  return 0;
  
 parent:
  waitpid(pid, &_status, 0);
 done:
  printf("%s\033[?25h\033[H\033[2J", started ? "\033]P0000000" : "");
  fflush(stdout);
  return 0;
}

