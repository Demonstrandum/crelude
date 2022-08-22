#include "argparse.h"
#include "common.h"
#include "utf.h"
#include "io.h"

#ifndef IMPLEMENTATION

u0 arginit(ArgParser *ctx)
{
    ctx->magic = ARGP_MAGIC_INIT_CONST;
    ctx->templates = (typeof(ctx->templates))AMAKE(ArgTemplate, 15);
    ctx->index_of_short_form = (typeof(ctx->index_of_short_form))MMAKE(byte,   usize, 15);
    ctx->index_of_long_form  = (typeof(ctx->index_of_long_form) )MMAKE(string, usize, 15);
    ctx->awaiting_value = false;
    ctx->string_count = 0;
    ctx->error_message = EMPTY(string);
}

ArgID argreg(ArgParser *ctx, char *short_form, char *long_form, bool takes_value, char *help)
{
    if (nil == ctx || ctx->magic != ARGP_MAGIC_INIT_CONST)
        panic("you did not init (`arginit`) the ArgParse structure.");

    usize index = ctx->templates.len;
    ArgID id = { (u16)index };
    string short_form_str = nil == short_form ? EMPTY(string) : to_string(short_form);
    string  long_form_str = nil ==  long_form ? EMPTY(string) : to_string(long_form);
    ArgTemplate template = {
        .id = id,
        .short_form = short_form_str,
        .long_form = long_form_str,
        .takes_value = takes_value,
        .help = to_string(help),
    };

    PUSH(ctx->templates, template);
    unless (nil == short_form) {
        if ('-' == short_form[0]) short_form += 1;
        ASSOCIATE(ctx->index_of_short_form, short_form[0], index);
    }
    unless (nil == long_form) {
        if (0 == strncmp(long_form, "--", 2)) long_form += 2;
        ASSOCIATE(ctx->index_of_long_form, to_string(long_form), index);
    }

    return id;
}

ierr argparse(ArgParser *ctx, void *map_id_to_arg, char *arg)
{
    ArgTemplate template;
    Arg argument;
    ArgID arg_id;
    usize *id = nil;
    char *equal_sign = nil;
    string value = to_string(arg);


    if (ctx->awaiting_value) {
        // set value of argument awaiting a value.
        argument = (Arg){ .value = to_string(arg), .is_on = true };
        associate(map_id_to_arg, &ctx->awaiting_value_id, &argument);
        ctx->awaiting_value = false;
    } else if (0 == strncmp(arg, "--", 2)) {
        // parsing long-form argument.
        equal_sign = strchr(arg, '=');
        if (nil != equal_sign)
            value = SLICE(string, value, 0, equal_sign - arg);
        value = SLICE(string, value, 2, -1);  // skip leading '--'

        id = LOOKUP(ctx->index_of_long_form, value);
        if (nil == id) {
            ctx->error_message = sprint("no such argument `--%S' exists.", value);
            return ARGPARSE_UNKNOWN;
        }
        arg_id = (ArgID){ *id };
        // get template for argument to determine behaviour.
        template = GET(ctx->templates, *id);
        if (template.takes_value) {
            // takes an argument.
            if (nil != equal_sign) {
                // value comes after equal sign.
                argument = (Arg){ .value = to_string(equal_sign + 1), .is_on = true };
                associate(map_id_to_arg, &arg_id, &argument);
            } else {
                // value comes in the next argument.
                ctx->awaiting_value = true;
                ctx->awaiting_value_id = arg_id;
            }
        } else {
            // is a toggle argument.
            argument = (Arg){ .value = EMPTY(string), .is_on = true };
            associate(map_id_to_arg, &arg_id, &argument);
        }
    } else if ('-' == arg[0] || '+' == arg[0]) {
        // parsing short-form argument(s).
        value = SLICE(string, value, 1, -1);  // skip the dash (-)
        foreach (opt, value) {
            id = LOOKUP(ctx->index_of_short_form, opt);
            if (nil == id) {
                ctx->error_message = sprint("no such argument `-%c' exists.", opt);
                return ARGPARSE_UNKNOWN;
            }
            arg_id = (ArgID){ *id };
            // get template for argument to determine behaviour.
            template = GET(ctx->templates, *id);
            if (template.takes_value) {
                // takes an argument.
                if (value.len != 1 && !it.first) {
                    // option mixed in with other options: not allowed (ambiguous).
                    ctx->error_message = sprint("argument `-%c' takes a value, and must stand alone.", opt);
                    return ARGPARSE_INVALID;
                } else if (value.len != 1) {
                    // the argument is glued onto the end (e.g. `-n13` <-> `-n 13`)
                    argument = (Arg){ .value = SLICE(string, value, 1, -1), .is_on = true };
                    associate(map_id_to_arg, &arg_id, &argument);
                    break;  // the argument has been consumed, no further options.
                } else {
                    // argument value comes in the next argument.
                    ctx->awaiting_value = true;
                    ctx->awaiting_value_id = arg_id;
                }
            } else {
                // is a toggle argument.
                argument = (Arg){ .value = EMPTY(string), .is_on = arg[0] == '-' };
                associate(map_id_to_arg, &arg_id, &argument);
            }
        }
    } else {
        ctx->error_message = sprint("unexpected string `%S' when parsing arguments.", value);
        return ARGPARSE_UNEXPECTED;
    }

    ++ctx->string_count;
    return ARGPARSE_OK;
}

ierr argparseall(ArgParser *ctx, void *map_id_to_arg, usize argc, char **argv)
{
    ierr err = OK;

    for (usize i = 0; i < argc; ++i)
        if (OK != (err = argparse(ctx, map_id_to_arg, argv[i]))) break;

    return err;
}

#endif
