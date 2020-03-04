/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/SYSTEMXVI.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2020 David MacKay.  All rights reserved.
 * Use is subject to license terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "ucl.h"
#include "utstring.h"

#include "svccfg.h"
#include "svccfg.tab.h"
#include "svccfg.y.h"

void ParseTrace (FILE *, char *);
void * ParseAlloc (void * (*allocProc) (size_t));
void ParseFree (void *, void (*freeProc) (void *));
void * Parse (void *, int, void *, void *);

struct svccfg_s svccfg;

static void success () { printf ("Task completed successfully.\n"); }

static void canonicalise_svc (ucl_object_t * svc)
{
    ucl_object_t * uname = ucl_object_pop_key (svc, "name");
    const ucl_object_t * uinsts = ucl_object_lookup (svc, "instances");
    const char * name = ucl_object_tostring (uname);

    ucl_object_insert_key (svc, uname, "path", 0, 1);

    if (uinsts)
    {
        ucl_object_t * uinst;
        ucl_object_iter_t it = NULL;

        while ((uinst = (ucl_object_t *)ucl_object_iterate (uinsts, &it, true)))
        {
            ucl_object_t * uname = ucl_object_pop_key (uinst, "name");
            char * path;

            asprintf (&path, "%s:%s", name, ucl_object_tostring (uname));
            ucl_object_insert_key (uinst, ucl_object_fromstring (path), "path",
                                   0, 1);

            ucl_object_unref (uname);
            free (path);
        }
    }
}

void import (str_list_t * paths)
{
    int num_manifests = str_list_size (paths);
    int loaded = 0;
    int failed = 0;
    UT_string * errs;

    utstring_new (errs);
    printf ("Loading S16 service manifests: ");

    LL_each (paths, it)
    {
        int bs = 0;
        int e;
        struct ucl_parser * parser = ucl_parser_new (0);
        ucl_object_t * obj = NULL;
        const char * path = it->val;

        bs = printf ("%d/%d", ++loaded, num_manifests);
        fflush (stdout);
        while (bs--)
            putchar ('\b');
        ucl_parser_add_file (parser, path);

        if (ucl_parser_get_error (parser))
        {
            failed++;
            utstring_printf (errs, "\t%s: Manifest parser error: %s\n", path,
                             ucl_parser_get_error (parser));
            goto cleanup;
        }

        obj = ucl_parser_get_object (parser);

        canonicalise_svc (obj);

        if ((e = s16db_import_ucl_svc (&svccfg.h, obj, L_MANIFEST)))
        {
            failed++;
            utstring_printf (errs, "\t%s: code %d", path, e);
        }

    cleanup:
        if (obj)
            ucl_obj_unref (obj);
        ucl_parser_free (parser);
    }
    putchar ('\n');

    if (failed)
    {
        printf ("%d S16 service descriptions failed to load:\n%s", failed,
                utstring_body (errs));
    }
}

void parse (const char * text)
{
    /* current token */
    int token;
    /* current text */
    char * tok = NULL;
    /* Lex requires this scanner structure in order to operate. */
    yyscan_t scanner;
    void * parser = ParseAlloc (malloc);

    // ParseTrace (stdout, "[parser]:"); // */

    yylex_init_extra (&tok, &scanner);
    yy_scan_string (text, scanner);

    while ((token = yylex (scanner)))
        Parse (parser, token, tok, 0);

    /* reduce it */
    Parse (parser, TK_NL, 0, 0);
    Parse (parser, TK_NL, 0, 0);

    ParseFree (parser, free);
    yylex_destroy (scanner);
}

void interact ()
{
    char * buf;
    char prompt[128];

    while (!svccfg.want_quit)
    {
        snprintf (prompt, 128, "%s> ", "svc:");

        if (!(buf = readline (prompt)))
            break;

        if (strlen (buf) > 0)
            add_history (buf);

        parse (buf);

        free (buf);
    }
}

int main (int argc, char * argv[])
{
    if (s16db_hdl_new (&svccfg.h))
        perror ("Failed to connect to repository");

    if (argc == 1)
        interact ();
    else
    {
        char * txt = calloc (1, 256);

        for (int i = 1; i < argc; i++)
        {
            strncat (txt, argv[i], 255);
            strncat (txt, " ", 255);
        }

        parse (txt);
        free (txt);
    }

    return 0;
}
