/* vi:set ts=4 sw=4 expandtab:
 *
 * Copyright 2016, Chris Leishman (http://github.com/cleishm)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "../../config.h"
#include "batch.h"
#include "evaluate.h"
#include "interactive.h"
#include "render.h"
#include "state.h"
#include "verification.h"
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <neo4j-client.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define NEO4J_HISTORY_FILE "client-history"


const char *shortopts = "hp:u:v";

#define HISTFILE_OPT 1000
#define INSECURE_OPT 1001
#define KNOWN_HOSTS_OPT 1002
#define NOHIST_OPT 1003
#define VERSION_OPT 1004
#define PIPELINE_MAX_OPT 1005

static struct option longopts[] =
    { { "help", no_argument, NULL, 'h' },
      { "history-file", required_argument, NULL, HISTFILE_OPT },
      { "no-history", no_argument, NULL, NOHIST_OPT },
      { "insecure", no_argument, NULL, INSECURE_OPT },
      { "username", required_argument, NULL, 'u' },
      { "password", required_argument, NULL, 'p' },
      { "known-hosts", required_argument, NULL, KNOWN_HOSTS_OPT },
      { "pipeline-max", required_argument, NULL, PIPELINE_MAX_OPT },
      { "verbose", no_argument, NULL, 'v' },
      { "version", no_argument, NULL, VERSION_OPT },
      { NULL, 0, NULL, 0 } };

static void usage(FILE *s, const char *prog_name)
{
    fprintf(s,
"usage: %s [OPTIONS] [URL]\n"
"options:\n"
" --help, -h          Output this usage information.\n"
" --history=file      Use the specified file for saving history.\n"
" --no-history        Do not save history.\n"
" --insecure          Do not attempt to establish a secure connection.\n"
" --username=name, -u name\n"
"                     Connect using the specified username.\n"
" --password=pass, -p pass\n"
"                     Connect using the specified password.\n"
" --known-hosts=file  Set the path to the known-hosts file.\n"
" --verbose, -v       Increase logging verbosity.\n"
" --version           Output the version of neo4j-client and dependencies.\n"
"\n"
"If URL is supplied then a connection is first made to the specified Neo4j\n"
"graph database.\n"
"\n"
"If the shell is run connected to a TTY, then an interactive command prompt\n"
"is shown. Use `:exit` to quit. If the shell is not connected to a TTY, then\n"
"directives are read from stdin.\n",
        prog_name);
}


int main(int argc, char *argv[])
{
    FILE *tty = fopen("/dev/tty", "r+");
    if (tty == NULL && errno != ENOENT)
    {
        perror("can't open /dev/tty");
        exit(EXIT_FAILURE);
    }

    char prog_name[PATH_MAX];
    if (neo4j_basename(argv[0], prog_name, sizeof(prog_name)) < 0)
    {
        perror("unexpected error");
        exit(EXIT_FAILURE);
    }

    uint8_t log_level = NEO4J_LOG_WARN;
    struct neo4j_logger_provider *provider = NULL;
    neo4j_config_t *config = NULL;

    neo4j_client_init();

    shell_state_t state;
    int result = EXIT_FAILURE;

    if (shell_state_init(&state, prog_name, stdin, stdout, stderr, tty))
    {
        perror("unexpected error");
        goto cleanup;
    }

    config = neo4j_new_config();
    if (config == NULL)
    {
        neo4j_perror(state.err, errno, "unexpected error");
        goto cleanup;
    }
    state.config = config;
    state.interactive = isatty(STDIN_FILENO);

    char histfile[PATH_MAX];
    if (neo4j_dot_dir(histfile, sizeof(histfile), NEO4J_HISTORY_FILE) < 0)
    {
        neo4j_perror(state.err, (errno == ERANGE)? ENAMETOOLONG : errno,
                "unexpected error");
        goto cleanup;
    }
    state.histfile = histfile;

    int c;
    while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) >= 0)
    {
        switch (c)
        {
        case 'h':
            usage(state.out, prog_name);
            result = EXIT_SUCCESS;
            goto cleanup;
        case 'v':
            ++log_level;
            break;
        case HISTFILE_OPT:
            state.histfile = (optarg[0] != '\0')? optarg : NULL;
            break;
        case INSECURE_OPT:
            state.connect_flags |= NEO4J_INSECURE;
            break;
        case 'u':
            if (neo4j_config_set_username(config, optarg))
            {
                neo4j_perror(state.err, errno, "unexpected error");
                goto cleanup;
            }
            break;
        case 'p':
            if (neo4j_config_set_password(config, optarg))
            {
                neo4j_perror(state.err, errno, "unexpected error");
                goto cleanup;
            }
            break;
        case KNOWN_HOSTS_OPT:
            if (neo4j_config_set_known_hosts_file(config, optarg))
            {
                neo4j_perror(state.err, errno, "unexpected error");
                goto cleanup;
            }
            break;
        case NOHIST_OPT:
            state.histfile = NULL;
            break;
        case PIPELINE_MAX_OPT:
            {
                int arg = atoi(optarg);
                if (arg < 1)
                {
                    fprintf(state.err, "Invalid pipeline-max '%s'\n", optarg);
                    goto cleanup;
                }
                state.pipeline_max = arg;
                neo4j_config_set_max_pipelined_requests(config, arg * 2);
            }
            break;
        case VERSION_OPT:
            fprintf(state.out, "neo4j-client: %s\n", PACKAGE_VERSION);
            fprintf(state.out, "libneo4j-client: %s\n",
                    libneo4j_client_version());
            result = EXIT_SUCCESS;
            goto cleanup;
        default:
            usage(state.err, prog_name);
            goto cleanup;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc > 1)
    {
        usage(state.err, prog_name);
        goto cleanup;
    }

    uint8_t logger_flags = 0;
    if (log_level < NEO4J_LOG_DEBUG)
    {
        logger_flags = NEO4J_STD_LOGGER_NO_PREFIX;
    }
    provider = neo4j_std_logger_provider(state.err, log_level, logger_flags);
    if (provider == NULL)
    {
        neo4j_perror(state.err, errno, "unexpected error");
        goto cleanup;
    }

    neo4j_config_set_logger_provider(config, provider);

    if (state.tty != NULL)
    {
        neo4j_config_set_unverified_host_callback(config,
                host_verification, &state);
    }

    if (argc >= 1)
    {
        if (db_connect(&state, argv[0]))
        {
            goto cleanup;
        }
    }

    if (state.interactive)
    {
        state.render = render_results_table;
        state.render_flags = NEO4J_RENDER_SHOW_NULLS;
        if (interact(&state))
        {
            goto cleanup;
        }
    }
    else
    {
        state.render = render_results_csv;
        state.width = 70;
        if (batch(&state))
        {
            goto cleanup;
        }
    }

    result = EXIT_SUCCESS;

cleanup:
    shell_state_destroy(&state);
    if (config != NULL)
    {
        neo4j_config_free(config);
    }
    if (provider != NULL)
    {
        neo4j_std_logger_provider_free(provider);
    }
    if (tty != NULL)
    {
        fclose(tty);
    }
    neo4j_client_cleanup();
    return result;
}
