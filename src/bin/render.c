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
#include "render.h"
#include <assert.h>
#include <neo4j-client.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>


struct renderer
{
    const char *name;
    int (*handler)(shell_state_t *state, neo4j_result_stream_t *results);
};


static int terminal_width(shell_state_t *state);

static struct renderer renderers[] =
    { { "table", render_results_table },
      { "csv", render_results_csv },
      { NULL, NULL } };


renderer_t find_renderer(const char *name)
{
    for (unsigned int i = 0; renderers[i].name != NULL; ++i)
    {
        if (strcmp(renderers[i].name, name) == 0)
        {
            return renderers[i].handler;
        }
    }
    return NULL;
}


int render_results_csv(shell_state_t *state, neo4j_result_stream_t *results)
{
    return neo4j_render_csv(state->out, results, state->render_flags);
}


int render_results_table(shell_state_t *state, neo4j_result_stream_t *results)
{
    int width = terminal_width(state);
    if (width < 2)
    {
        fprintf(state->err, "ERROR: terminal width of %d too narrow "
                "(use :output csv?)\n", width);
        return -1;
    }
    return neo4j_render_table(state->out, results, width, state->render_flags);
}


int terminal_width(shell_state_t *state)
{
    if (state->width > 0)
    {
        return state->width;
    }
    struct winsize w;
    if (ioctl(fileno(state->out), TIOCGWINSZ, &w))
    {
        return -1;
    }
    return w.ws_col;
}


int render_update_counts(shell_state_t *state, neo4j_result_stream_t *results)
{
    assert(results != NULL);
    struct neo4j_update_counts counts = neo4j_update_counts(results);

    static const char * const count_names[] = {
        "Nodes created",
        "Nodes deleted",
        "Relationships created",
        "Relationships deleted",
        "Properties set",
        "Labels added",
        "Labels removed",
        "Indexes added",
        "Indexes removed",
        "Constraints added",
        "Constraints removed",
        NULL
    };
    unsigned long long count_values[] = {
        counts.nodes_created,
        counts.nodes_deleted,
        counts.relationships_created,
        counts.relationships_deleted,
        counts.properties_set,
        counts.labels_added,
        counts.labels_removed,
        counts.indexes_added,
        counts.indexes_removed,
        counts.constraints_added,
        counts.constraints_removed
    };

    for (int i = 0; count_names[i] != NULL; ++i)
    {
        if (count_values[i] == 0)
        {
            continue;
        }
        if (fprintf(state->out, "%s: %llu\n",
                count_names[i], count_values[i]) < 0)
        {
            return -1;
        }
    }

    return 0;
}


int render_plan_table(shell_state_t *state, struct neo4j_statement_plan *plan)
{
    int width = terminal_width(state);
    if (width < 2)
    {
        fprintf(state->err, "ERROR: terminal width of %d too narrow "
                "to render plan/profile\n", width);
        return -1;
    }
    if (fprintf(state->out, "%sCompiler: %s\nPlanner: %s\nRuntime: %s\n%s:\n",
                plan->is_profile? "\n" : "", plan->version, plan->planner,
                plan->runtime, plan->is_profile? "Profile":"Plan") < 0)
    {
        return -1;
    }
    return neo4j_render_plan_table(state->out, plan, width, 0);
}
