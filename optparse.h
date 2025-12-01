/* Optparse --- portable, reentrant, embeddable, getopt-like option parser
 *
 * This is free and unencumbered software released into the public domain.
 *
 * To get the implementation, define OPTPARSE_IMPLEMENTATION.
 * Optionally define OPTPARSE_API to control the API's visibility
 * and/or linkage (static, __attribute__, __declspec).
 *
 * The POSIX getopt() option parser has three fatal flaws. These flaws
 * are solved by Optparse.
 *
 * 1) Parser state is stored entirely in global variables, some of
 * which are static and inaccessible. This means only one thread can
 * use getopt(). It also means it's not possible to recursively parse
 * nested sub-arguments while in the middle of argument parsing.
 * Optparse fixes this by storing all state on a local struct.
 *
 * 2) The POSIX standard provides no way to properly reset the parser.
 * This means for portable code that getopt() is only good for one
 * run, over one argv with one option string. It also means subcommand
 * options cannot be processed with getopt(). Most implementations
 * provide a method to reset the parser, but it's not portable.
 * Optparse provides an optparse_arg() function for stepping over
 * subcommands and continuing parsing of options with another option
 * string. The Optparse struct itself can be passed around to
 * subcommand handlers for additional subcommand option parsing. A
 * full reset can be achieved by with an additional optparse_init().
 *
 * 3) Error messages are printed to stderr. This can be disabled with
 * opterr, but the messages themselves are still inaccessible.
 * Optparse solves this by writing an error message in its errmsg
 * field. The downside to Optparse is that this error message will
 * always be in English rather than the current locale.
 *
 * Optparse should be familiar with anyone accustomed to getopt(), and
 * it could be a nearly drop-in replacement. The option string is the
 * same and the fields have the same names as the getopt() global
 * variables (optarg, optind, optopt).
 *
 * Optparse also supports GNU-style long options with optparse_long().
 * The interface is slightly different and simpler than getopt_long().
 *
 * By default, argv is permuted as it is parsed, moving non-option
 * arguments to the end. This can be disabled by setting the `permute`
 * field to 0 after initialization.
 */
#ifndef OPTPARSE_H
#define OPTPARSE_H

#ifndef OPTPARSE_API
#  define OPTPARSE_API
#endif

struct optparse
{
	char **argv;
	int permute;
	int optind;
	int optopt;
	char *optarg;
	char errmsg[64];
	int subopt;
};

enum optparse_argtype
{
	OPTPARSE_NONE,
	OPTPARSE_REQUIRED,
	OPTPARSE_OPTIONAL
};

struct optparse_option
{
	const char *longname;
	int shortname;
	enum optparse_argtype argtype;
};

#include <string.h>
#include <ctype.h>

/**
 * Initializes the parser state.
 */
OPTPARSE_API
void optparse_init(struct optparse *options, char **argv);

/**
 * Handles GNU-style long options in addition to getopt() options.
 * This works a lot like GNU's getopt_long(). The last option in
 * longopts must be all zeros, marking the end of the array. The
 * longindex argument may be NULL.
 *
 * @return the zero-based index in the longopts array,
 *         OPTPARSE_DONE for done, or OPTPARSE_ERR for error.
 */
OPTPARSE_API
int optparse(struct optparse *options,
			 const struct optparse_option *longopts,
			 int *longindex);

/**
 * Used for stepping over non-option arguments.
 * @return the next non-option argument, or NULL for no more arguments
 *
 * Argument parsing can continue with optparse() after using this
 * function. That would be used to parse the options for the
 * subcommand returned by optparse_arg(). This function allows you to
 * ignore the value of optind.
 */
OPTPARSE_API
char *optparse_arg(struct optparse *options);

/* Implementation */
#ifdef OPTPARSE_IMPLEMENTATION

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"
#define OPTPARSE_ERR (-2)
#define OPTPARSE_DONE (-1)

static int
optparse_error(struct optparse *options, const char *msg, const char *data)
{
	unsigned p = 0;
	const char *sep = " -- '";
	while (*msg)
		options->errmsg[p++] = *msg++;
	while (*sep)
		options->errmsg[p++] = *sep++;
	while (p < sizeof(options->errmsg) - 2 && *data)
		options->errmsg[p++] = *data++;
	options->errmsg[p++] = '\'';
	options->errmsg[p++] = '\0';
	return OPTPARSE_ERR;
}

OPTPARSE_API
void
optparse_init(struct optparse *options, char **argv)
{
	options->argv = argv;
	options->permute = 1;
	options->optind = argv[0] != 0;
	options->subopt = 0;
	options->optarg = 0;
	options->errmsg[0] = '\0';
}

static int
optparse_is_dashdash(const char *arg)
{
	// "--\0"
	return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] == '\0';
}

static int
optparse_is_shortopt(const char *arg)
{
	// "-X"
	return arg != 0 && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static int
optparse_is_longopt(const char *arg)
{
	// "--XX" or "/XX"
	return arg != 0
		&& ((arg[0] == '-' && arg[1] == '-' && arg[2] != '\0')
			|| (arg[0] == '/' && arg[1] != '\0'));
}

static void
optparse_permute(struct optparse *options, int index)
{
	char *nonoption = options->argv[index];
	int i;
	for (i = index; i < options->optind - 1; i++)
		options->argv[i] = options->argv[i + 1];
	options->argv[options->optind - 1] = nonoption;
}

static int
optparse_opts_end(const struct optparse_option *options, int i)
{
	return !options[i].longname && !options[i].shortname;
}

/* Unlike strcmp(), handles options containing "=". */
static int
optparse_opts_match(const char *longname, const char *option)
{
	if (longname == 0)
		return 0;
	{
		size_t name_len = strlen(longname);
		if (_strnicmp(longname, option, name_len) != 0)
			return 0;
		return option[name_len] == '\0' || option[name_len] == '=';
	}
}

/* Return the part after "=", or NULL. */
static char *
optparse_opts_arg(char *option)
{
	for (; *option && *option != '='; option++);
	if (*option == '=')
		return option + 1;
	else
		return 0;
}

static int
optparse_handle_shortopt(struct optparse *options,
						 const struct optparse_option *longopts)
{
	int i;
	char *option = options->argv[options->optind];
	char namebuf[2] = {0, 0};
	int matched = -1;
	for (i = 0; !optparse_opts_end(longopts, i); i++)
	{
		if (longopts[i].shortname &&
			tolower((unsigned char)longopts[i].shortname) == tolower((unsigned char)option[1]))
		{
			matched = i;
			break;
		}
	}
	if (matched < 0)
		return optparse_error(options, OPTPARSE_MSG_INVALID, option);

	namebuf[0] = (char)longopts[matched].shortname;
	options->errmsg[0] = '\0';
	options->optopt = matched;
	options->optarg = 0;

	switch (longopts[matched].argtype)
	{
	case OPTPARSE_NONE:
		if (option[2] != '\0')
			return optparse_error(options, OPTPARSE_MSG_TOOMANY, option);
		options->optind++;
		return matched;
	case OPTPARSE_REQUIRED:
		if (option[2] != '\0')
		{
			options->optarg = option + 2;
			options->optind++;
			return matched;
		}
		else if (options->argv[options->optind + 1] != 0)
		{
			options->optarg = options->argv[options->optind + 1];
			options->optind += 2;
			return matched;
		}
		else
		{
			const char *name = longopts[matched].longname ? longopts[matched].longname : namebuf;
			return optparse_error(options, OPTPARSE_MSG_MISSING, name);
		}
	case OPTPARSE_OPTIONAL:
		if (option[2] != '\0')
			options->optarg = option + 2;
		options->optind++;
		return matched;
	default:
		break;
	}
	return OPTPARSE_ERR;
}

static int
optparse_handle_longopt(struct optparse *options,
						const struct optparse_option *longopts,
						int *longindex,
						char *option)
{
	int i;
	int prefix = (option[0] == '/' ? 1 : 2);
	options->errmsg[0] = '\0';
	options->optopt = 0;
	options->optarg = 0;
	option += prefix; /* skip "--" or "/" */
	options->optind++;
	for (i = 0; !optparse_opts_end(longopts, i); i++)
	{
		const char *name = longopts[i].longname;
		if (optparse_opts_match(name, option))
		{
			char *arg;
			if (longindex)
				*longindex = i;
			options->optopt = i;
			arg = optparse_opts_arg(option);
			if (longopts[i].argtype == OPTPARSE_NONE && arg != 0)
			{
				return optparse_error(options, OPTPARSE_MSG_TOOMANY, name);
			}
			if (arg != 0)
			{
				options->optarg = arg;
			}
			else if (longopts[i].argtype == OPTPARSE_REQUIRED)
			{
				options->optarg = options->argv[options->optind];
				if (options->optarg == 0)
					return optparse_error(options, OPTPARSE_MSG_MISSING, name);
				else
					options->optind++;
			}
			return i;
		}
	}
	return optparse_error(options, OPTPARSE_MSG_INVALID, option);
}

OPTPARSE_API
int
optparse(struct optparse *options,
		 const struct optparse_option *longopts,
		 int *longindex)
{
	char *option = options->argv[options->optind];
	if (option == 0)
	{
		return -1;
	}
	else if (optparse_is_dashdash(option))
	{
		options->optind++; /* consume "--" */
		return OPTPARSE_DONE;
	}
	else if (optparse_is_shortopt(option))
	{
		return optparse_handle_shortopt(options, longopts);
	}
	else if (optparse_is_longopt(option))
	{
		return optparse_handle_longopt(options, longopts, longindex, option);
	}

	if (options->permute)
	{
		int index = options->optind++;
		int r = optparse(options, longopts, longindex);
		optparse_permute(options, index);
		options->optind--;
		return r;
	}
	else
	{
		return OPTPARSE_DONE;
	}
}

OPTPARSE_API
char *
optparse_arg(struct optparse *options)
{
	char *option = options->argv[options->optind];
	options->subopt = 0;
	if (option != 0)
		options->optind++;
	return option;
}

#endif /* OPTPARSE_IMPLEMENTATION */
#endif /* OPTPARSE_H */
