/* See LICENSE file for copyright and license details. */
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libgamma.h>

#include "arg.h"

#define t(...) do { if (__VA_ARGS__) goto fail; } while (0)

#define WELCOME_MESSAGE\
	"\033[H\033[01;31mWARNING: Do not use this if you have epilepsia.\033[00m\n"\
	"\n"\
	"It is recommended to massage the defective dots, whilst\n"\
	"running this program.\n"\
	"\n"\
	"Press C-c to quit, and Enter to start (C-c will quit).\n"\
	"\n"\
	"\n"\
	"You should not have to run this program. If the manufacture or\n"\
	"reseller of your monitor refuse to replace or repair the monitor\n"\
	"that was broken, because of manufacturing error, when it was\n"\
	"chiped to you, please consider yelling at the support, threaten\n"\
	"to report the personally because of involvement in organised\n"\
	"crime (not replacing hardware that was chipped broken when you\n"\
	"selled it is a crime, and they do this in an organised fashion\n"\
	"to make money), call their executive director persistently at\n"\
	"inconvenient times, report them applicable board for consumer\n"\
	"disputes/complaints or equivalent agency. If this program does\n"\
	"not help, and the manufacture is Packard Bell, or its\n"\
	"supersidiary Acer, tell them that you have tried a program\n"\
	"that was prompted by the complete suckness and incapability\n"\
	"to repair defects that they have agreed to repair. They suck!"

/**
 * The name of the process.
 */
char *argv0;

/**
 * Should we exit?
 */
static volatile sig_atomic_t please_exit = 0;

/**
 * The name of the site.
 */
static char *sitename = NULL;

/**
 * The site.
 */
static libgamma_site_state_t site;

/**
 * The partitions.
 */
static libgamma_partition_state_t *restrict *restrict parts = NULL;

/**
 * The CRTC:s.
 */
static libgamma_crtc_state_t *restrict *restrict crtcs = NULL;

/**
 * The ramps to use when displaying red.
 */
static libgamma_gamma_ramps16_t *restrict *restrict ramps_red = NULL;

/**
 * The ramps to use when displaying green.
 */
static libgamma_gamma_ramps16_t *restrict *restrict ramps_green = NULL;

/**
 * The ramps to use when displaying blue.
 */
static libgamma_gamma_ramps16_t *restrict *restrict ramps_blue = NULL;

/**
 * The original ramps.
 */
static libgamma_gamma_ramps16_t *restrict *restrict ramps_saved = NULL;

/**
 * 2: `site` initialised and all ramps saved.
 * 1: `site` initialised.
 * 0: nothing done.
 */
static int clut_state = 0;



/**
 * Parse interval argument.
 * 
 * @param   argv      The interval argument.
 * @param   interval  Output parameter for the interval.
 * @param   nonzero   With be set to zero if the interval is zero,
 *                    otherwise it is set to a non-zero value (1).
 * @return            0 on success, -1 on error.
 */
static int
get_interval(const char *restrict arg, struct timespec *restrict interval, int *restrict nonzero)
{
	char *end;
	*nonzero = 1;
	errno = EINVAL;
	t (!isdigit(*arg));
	interval->tv_sec = (time_t)(errno = 0, strtol)(arg, &end, 10);
	t (errno || (errno = EINVAL, *end));
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
static pid_t
xfork(void)
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


/**
 * Print usage information and exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: %s [-v] [milliseconds]\n", argv0);
	exit(1);
}


/**
 * Uninitialise the CLUT backend.
 * 
 * @return  0 on success, -1 on error.
 */
static void
term_clut(void)
{
	size_t i, j;
	if (clut_state == 2)
		for (j = 0; j < 2; j++, usleep(1000000L / 10L))
			for (i = 0; crtcs && crtcs[i]; i++, usleep(1000000L / 100L))
				libgamma_crtc_set_gamma_ramps16(crtcs[i], *(ramps_saved[i]));
	for (i = 0; crtcs && crtcs[i]; i++) {
		libgamma_crtc_free(crtcs[i]);
		libgamma_gamma_ramps16_free(ramps_red[i]);
		libgamma_gamma_ramps16_free(ramps_green[i]);
		libgamma_gamma_ramps16_free(ramps_blue[i]);
		libgamma_gamma_ramps16_free(ramps_saved[i]);
	}
#if defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif
	free(crtcs);
	free(ramps_red);
	free(ramps_green);
	free(ramps_blue);
	free(ramps_saved);
	for (i = 0; parts && parts[i]; i++)
		libgamma_partition_free(parts[i]);
	free(parts);
	free(sitename);
#if defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
	if (clut_state)
		libgamma_site_destroy(&site);
	sitename = NULL;
	parts = NULL;
	crtcs = NULL;
	ramps_red = ramps_green = ramps_blue = ramps_saved = NULL;
	clut_state = 0;
}


/**
 * Initialise the CLUT backend.
 * 
 * @return  0 on success, -1 on error.
 */
static int
init_clut(void)
{
	int method, error = 0;
	size_t i, j, k, n = 0;
	libgamma_crtc_information_t info;
  
	if (libgamma_list_methods(&method, 1, 0) < 1) {
		fprintf(stderr, "%s: could not get adjustment method, try '%s -v'.\n", argv0, argv0);
		return -1;
	}

	if ((sitename = libgamma_method_default_site(method)))
		t (!(sitename = strdup(sitename)));
	t ((error = libgamma_site_initialise(&site, method, sitename)));
	sitename = NULL;

	clut_state++;

	if (!site.partitions_available) {
		fprintf(stderr, "%s: could find any graphics card/screen, try '%s -v'.\n", argv0, argv0);
		errno = 0;
		goto fail;
	}

	t (!(parts = calloc(site.partitions_available + 1, sizeof(*parts))));
	for (i = 0; i < site.partitions_available; i++) {
		t (!(parts[i] = calloc(1, sizeof(**parts))));
		t ((errno = libgamma_partition_initialise(parts[i], &site, i)));
		n += parts[i]->crtcs_available;
	}

	if (!n) {
		fprintf(stderr, "%s: could find any CRTC, try '%s -v'.\n", argv0, argv0);
		errno = 0;
		goto fail;
	}

	t (!(crtcs       = calloc(n + 1, sizeof(*crtcs))));
	t (!(ramps_red   = calloc(n + 1, sizeof(*ramps_red))));
	t (!(ramps_green = calloc(n + 1, sizeof(*ramps_green))));
	t (!(ramps_blue  = calloc(n + 1, sizeof(*ramps_blue))));
	t (!(ramps_saved = calloc(n + 1, sizeof(*ramps_saved))));
	for (i = k = 0; i < site.partitions_available; i++) {
		for (j = 0; j < parts[i]->crtcs_available; j++, k++) {
			t (!(crtcs[k]       = calloc(1, sizeof(**crtcs))));
			t (!(ramps_red[k]   = calloc(1, sizeof(**ramps_red))));
			t (!(ramps_green[k] = calloc(1, sizeof(**ramps_green))));
			t (!(ramps_blue[k]  = calloc(1, sizeof(**ramps_blue))));
			t (!(ramps_saved[k] = calloc(1, sizeof(**ramps_saved))));
			t ((error = libgamma_crtc_initialise(crtcs[k], parts[i], j)));
			if (libgamma_get_crtc_information(&info, crtcs[k], LIBGAMMA_CRTC_INFO_GAMMA_SIZE))
				t ((error = info.gamma_size_error));
			ramps_red[k]->red_size   = info.red_gamma_size;
			ramps_red[k]->green_size = info.green_gamma_size;
			ramps_red[k]->blue_size  = info.blue_gamma_size;
			*(ramps_saved[k]) = *(ramps_blue[k]) = *(ramps_green[k]) = *(ramps_red[k]);
			t (libgamma_gamma_ramps16_initialise(ramps_red[k]));
			t (libgamma_gamma_ramps16_initialise(ramps_green[k]));
			t (libgamma_gamma_ramps16_initialise(ramps_blue[k]));
			t (libgamma_gamma_ramps16_initialise(ramps_saved[k]));
			memset(ramps_red[k]->red,     ~0, ramps_red[k]->red_size     * sizeof(*(ramps_red[k]->red)));
			memset(ramps_red[k]->green,    0, ramps_red[k]->green_size   * sizeof(*(ramps_red[k]->green)));
			memset(ramps_red[k]->blue,     0, ramps_red[k]->blue_size    * sizeof(*(ramps_red[k]->blue)));
			memset(ramps_green[k]->red,    0, ramps_green[k]->red_size   * sizeof(*(ramps_green[k]->red)));
			memset(ramps_green[k]->green, ~0, ramps_green[k]->green_size * sizeof(*(ramps_green[k]->green)));
			memset(ramps_green[k]->blue,   0, ramps_green[k]->blue_size  * sizeof(*(ramps_green[k]->blue)));
			memset(ramps_blue[k]->red,     0, ramps_blue[k]->red_size    * sizeof(*(ramps_blue[k]->red)));
			memset(ramps_blue[k]->green,   0, ramps_blue[k]->green_size  * sizeof(*(ramps_blue[k]->green)));
			memset(ramps_blue[k]->blue,   ~0, ramps_blue[k]->blue_size   * sizeof(*(ramps_blue[k]->blue)));
			t ((error = libgamma_crtc_get_gamma_ramps16(crtcs[k], ramps_saved[k])));
		}
	}

	clut_state++;

	return 0;
fail:
	if (error)
		libgamma_perror(argv0, error);
	else if (errno)
		perror(argv0);
	term_clut();
	errno = 0;
	return -1;
}


/**
 * The loop, using the graphic cards' colour lookup tables.
 * 
 * @param  interval  The sleep interval, `NULL` for no sleep.
 */
static void
loop_clut(const struct timespec *restrict interval)
{
	size_t i;
	int error;

	while (!please_exit) {
		for (i = 0; crtcs[i]; i++)
			t ((error = libgamma_crtc_set_gamma_ramps16(crtcs[i], *(ramps_red[i]))));
		if (interval)
			t ((errno = clock_nanosleep(CLOCK_MONOTONIC, 0, interval, NULL)));

		for (i = 0; crtcs[i]; i++)
			t ((error = libgamma_crtc_set_gamma_ramps16(crtcs[i], *(ramps_green[i]))));
		if (interval)
			t ((errno = clock_nanosleep(CLOCK_MONOTONIC, 0, interval, NULL)));

		for (i = 0; crtcs[i]; i++)
			t ((error = libgamma_crtc_set_gamma_ramps16(crtcs[i], *(ramps_blue[i]))));
		if (interval)
			t ((errno = clock_nanosleep(CLOCK_MONOTONIC, 0, interval, NULL)));
	}
	return;
fail:
	if (error)
		libgamma_perror(argv0, error), errno = 0;
}


/**
 * The loop, using Linux VT.
 * 
 * @param  interval  The sleep interval, `NULL` for no sleep.
 */
static void
loop_vt(const struct timespec *restrict interval)
{
	while (!please_exit) {
		t (printf("\033]P0FF0000\033[2J") == -1);
		t (fflush(stdout));
		if (interval)
			t ((errno = clock_nanosleep(CLOCK_MONOTONIC, 0, interval, NULL)));

		t (printf("\033]P000FF00\033[2J") == -1);
		t (fflush(stdout));
		if (interval)
			t ((errno = clock_nanosleep(CLOCK_MONOTONIC, 0, interval, NULL)));

		t (printf("\033]P00000FF\033[2J") == -1);
		t (fflush(stdout));
		if (interval)
			t ((errno = clock_nanosleep(CLOCK_MONOTONIC, 0, interval, NULL)));
	}
	return;
fail:
	return;
}


/**
 * Signal handler for signals that exit the program.
 * 
 * @param  signo  The caught signal.
 */
static void
sigexit(int signo)
{
	(void) signo;
	please_exit = 1;
}


int
main(int argc, char *argv[])
{
	int started = 0;
	int with_sleep = 0;
	struct timespec interval;
	int status, vt = 0;
	pid_t pid = -1;
	struct sigaction sa;
	int saved_errno;

	ARGBEGIN {
	case 'v':
		vt = 1;
		break;
	default:
		usage();
		break;
	} ARGEND;

	if (optind < argc)
		t (get_interval(argv[optind++], &interval, &with_sleep));
	if (optind < argc)
		usage();

	printf("\033[?25l\033[H\033[2J");
	printf(WELCOME_MESSAGE);
	fflush(stdout);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigexit;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGHUP,  &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	if (read(STDIN_FILENO, &started, 1) < 0)
		goto done;
	started = 1;

	if (vt)
		t (printf("\033[H") == -1);
	else
		t (printf("\033[H\033[2JIf you can read this, try '%s -v'.\n", argv0) == -1);
	t (fflush(stdout));

	t (vt ? 0 : init_clut());

	t ((pid = xfork()) == -1);
	if (pid)
		goto parent;

	(vt ? loop_vt : loop_clut)(with_sleep ? &interval : NULL);

fail:
	saved_errno = errno;
	printf("%s\033[?25h\033[H\033[2J", (started && vt) ? "\033]P0000000" : "");
	fflush(stdout);
	if (!vt)
		term_clut();
	errno = saved_errno;
	if (pid == -1) {
		if (errno == EINVAL)
			usage();
		return (errno && (errno != EINTR)) ? (perror(argv0), 1) : 1;
	}
	return 0;

parent:
	waitpid(pid, &status, 0);
done:
	if (!vt)
		term_clut();
	printf("%s\033[?25h\033[H\033[2J", (started && vt) ? "\033]P0000000" : "");
	fflush(stdout);
	return WIFEXITED(status) ? WEXITSTATUS(status) : 0;
}
