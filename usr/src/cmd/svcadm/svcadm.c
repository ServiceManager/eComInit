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

#include "svcadm.h"
#include "svcadm.tab.h"
#include "svcadm.y.h"

void ParseTrace (FILE *, char *);
void * ParseAlloc (void * (*allocProc) (size_t));
void ParseFree (void *, void (*freeProc) (void *));
void * Parse (void *, int, void *, void *);

struct svcadm_s svcadm;

static void fail (const char * s) { printf ("Task failed: %s\n", s); }

static void success () { printf ("Task completed successfully.\n"); }

void disable (const char * spath)
{
    S16Path * path = s16db_string_to_path (spath);
    if (!path)
        fail ("Path is invalid.");
    else
    {
        int res = s16db_disable (&svcadm.h, path);
        S16PathDestroy (path);

        if (!res)
            success ();
        else
            fail ("Bad result from repository server.");
    }
}

void enable (const char * spath)
{
    S16Path * path = s16db_string_to_path (spath);
    if (!path)
        fail ("Path is invalid.");
    else
    {
        int res = s16db_enable (&svcadm.h, path);
        S16PathDestroy (path);

        if (!res)
            success ();
        else
            fail ("Bad result from repository server.\n");
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

    /* ParseTrace (stdout, "[parser]:"); */

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

    while (!svcadm.want_quit)
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
    if (s16db_hdl_new (&svcadm.h))
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
