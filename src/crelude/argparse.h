//! @file argparse.h
//! Parsing command line arguments.

#pragma once
#include "common.h"

#define ARGP_MAGIC_INIT_CONST 0x61726770
                             /* a r g p */

enum {
    ARGPARSE_OK = OK,
    ARGPARSE_INVALID,  //< invalid argument structure.
    ARGPARSE_UNEXPECTED,  //< unexpected argument while parsing.
    ARGPARSE_EXPECTED_ARGUMENT,  //< expected value to be given to parameter.
    ARGPARSE_UNKNOWN,  // < unknown argument/option given.
};

record(Arg) {
    string value;  //< slice into ARGV string, nil string if no argument.
    bool is_on;  //< true if argument toggled on, false if not (-a/+a).
};

newtype(ArgID, u16);
newmap(Args, ArgID, Arg);

record(ArgTemplate) {
    ArgID id;
    string short_form; //< may be nil.
    string long_form;  //< may be nil.
    bool takes_value;
    string help; //< may be nil, but morally shouldn't be.
};

record(ArgParser) {
    u32 magic;
    arrayof(ArgTemplate) templates;
    mapof(byte,   usize) index_of_short_form;
    mapof(string, usize) index_of_long_form;
    bool awaiting_value;
    ArgID awaiting_value_id;
    usize string_count;  //< current index into ARGV.
    string error_message;  //< error if encountered.
};

/// Register a command-line argument.
/// @example
///     ArgParser ctx;
///     mapof(ArgID, Arg) args;
///     ArgID arg_m, arg_l;
///     // --
///     arginit(&ctx);
///     arg_m = argreg(&ctx, "m", "message", true, "Write your message.");
///     arg_l = argreg(&ctx, "-l", "--list",   false, "List values");
///     for (usize i = 0; i < argc; ++i)
///        argparse(&ctx, &args, argv[i]);   // TODO: check returned error code.
ArgID argreg(ArgParser *ctx, char *short_form, char *long_form, bool takes_value, char *help);

/// Initialise argument parser.
u0 arginit(ArgParser *ctx);

/// Parse arguments one value of ARGV at the time.
ierr argparse(ArgParser *ctx, void *map_id_to_arg, char *arg);

/// Parse arguments from argc and argv.
ierr argparseall(ArgParser *ctx, void *map_id_to_arg, usize argc, char **argv);
