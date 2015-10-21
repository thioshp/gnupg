/* t-dns-cert.c - Module test for dns-stuff.c
 * Copyright (C) 2011 Free Software Foundation, Inc.
 * Copyright (C) 2011, 2015 Werner Koch
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifndef HAVE_W32_SYSTEM
# include <netdb.h>
#endif

#include "util.h"
#include "dns-stuff.h"

#define PGM "t-dns-stuff"

static int verbose;
static int debug;



int
main (int argc, char **argv)
{
  int last_argc = -1;
  gpg_error_t err;
  int opt_cert = 0;
  char const *name;

  gpgrt_init ();
  if (argc)
    { argc--; argv++; }
  while (argc && last_argc != argc )
    {
      last_argc = argc;
      if (!strcmp (*argv, "--"))
        {
          argc--; argv++;
          break;
        }
      else if (!strcmp (*argv, "--help"))
        {
          fputs ("usage: " PGM " [HOST]\n"
                 "Options:\n"
                 "  --verbose         print timings etc.\n"
                 "  --debug           flyswatter\n"
                 "  --cert            lookup a CERT RR\n"
                 , stdout);
          exit (0);
        }
      else if (!strcmp (*argv, "--verbose"))
        {
          verbose++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--debug"))
        {
          verbose += 2;
          debug++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--cert"))
        {
          opt_cert = 1;
          argc--; argv++;
        }
      else if (!strncmp (*argv, "--", 2))
        {
          fprintf (stderr, PGM ": unknown option '%s'\n", *argv);
          exit (1);
        }
    }

  if (!argc)
    {
      opt_cert = 1;
      name = "simon.josefsson.org";
    }
  else if (argc == 1)
    name = *argv;
  else
    {
      fprintf (stderr, PGM ": too many host names given\n");
      exit (1);
    }

  if (opt_cert)
    {
      unsigned char *fpr;
      size_t fpr_len;
      char *url;
      void *key;
      size_t keylen;

      printf ("CERT lookup on '%s'\n", name);

      err = get_dns_cert (name, DNS_CERTTYPE_ANY, &key, &keylen,
                          &fpr, &fpr_len, &url);
      if (err)
        printf ("get_dns_cert failed: %s <%s>\n",
                gpg_strerror (err), gpg_strsource (err));
      else if (key)
        {
          printf ("Key found (%u bytes)\n", (unsigned int)keylen);
        }
      else
        {
          if (fpr)
            {
              int i;

              printf ("Fingerprint found (%d bytes): ", (int)fpr_len);
              for (i = 0; i < fpr_len; i++)
                printf ("%02X", fpr[i]);
              putchar ('\n');
            }
          else
            printf ("No fingerprint found\n");

          if (url)
            printf ("URL found: %s\n", url);
          else
            printf ("No URL found\n");

        }

      xfree (key);
      xfree (fpr);
      xfree (url);
    }
  else /* Standard lookup.  */
    {
      char *cname;
      dns_addrinfo_t aibuf, ai;
      int ret;
      char hostbuf[1025];

      printf ("Lookup on '%s'\n", name);

      err = resolve_dns_name (name, 0, 0, SOCK_STREAM, &aibuf, &cname);
      if (err)
        {
          fprintf (stderr, PGM": resolving '%s' failed: %s\n",
                   name, gpg_strerror (err));
          exit (1);
        }

      if (cname)
        printf ("cname: %s\n", cname);
      for (ai = aibuf; ai; ai = ai->next)
        {
          printf ("%s %3d %3d   ",
                  ai->family == AF_INET6? "inet6" :
                  ai->family == AF_INET?  "inet4" : "?    ",
                  ai->socktype, ai->protocol);

          ret = getnameinfo (ai->addr, ai->addrlen,
                             hostbuf, sizeof hostbuf,
                             NULL, 0,
                             NI_NUMERICHOST);
          if (ret)
            printf ("[getnameinfo failed: %s]\n", gai_strerror (ret));
          else
            printf ("%s\n", hostbuf);
        }
      xfree (cname);
      free_dns_addrinfo (aibuf);
    }


  return 0;
}