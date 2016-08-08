/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include <glib-object.h>
#include <locale.h>
#include <arpa/inet.h>

#include <ModemManager.h>
#define _LIBMM_INSIDE_MM
#include <libmm-glib.h>

#include "mm-log.h"
#include "mm-modem-helpers.h"
#include "mm-modem-helpers-ublox.h"

/*****************************************************************************/
/* Test UUSBCONF? responses */

typedef struct {
    const gchar       *str;
    MMUbloxUsbProfile  profile;
} UusbconfResponseTest;

static const UusbconfResponseTest uusbconf_response_tests[] = {
    {
        .str     = "+UUSBCONF: 3,\"RNDIS\",,\"0x1146\"\r\n",
        .profile = MM_UBLOX_USB_PROFILE_RNDIS
    },
    {
        .str     = "+UUSBCONF: 2,\"ECM\",,\"0x1143\"\r\n",
        .profile = MM_UBLOX_USB_PROFILE_ECM
    },
    {
        .str     = "+UUSBCONF: 0,\"\",,\"0x1141\"\r\n",
        .profile = MM_UBLOX_USB_PROFILE_BACK_COMPATIBLE
    },
};

static void
test_uusbconf_response (void)
{
    guint i;

    for (i = 0; i < G_N_ELEMENTS (uusbconf_response_tests); i++) {
        MMUbloxUsbProfile profile = MM_UBLOX_USB_PROFILE_UNKNOWN;
        GError *error = NULL;
        gboolean success;

        success = mm_ublox_parse_uusbconf_response (uusbconf_response_tests[i].str, &profile, &error);
        g_assert_no_error (error);
        g_assert (success);
        g_assert_cmpuint (uusbconf_response_tests[i].profile, ==, profile);
    }
}

/*****************************************************************************/
/* Test UBMCONF? responses */

typedef struct {
    const gchar           *str;
    MMUbloxNetworkingMode  mode;
} UbmconfResponseTest;

static const UbmconfResponseTest ubmconf_response_tests[] = {
    {
        .str  = "+UBMCONF: 1\r\n",
        .mode = MM_UBLOX_NETWORKING_MODE_ROUTER
    },
    {
        .str  = "+UBMCONF: 2\r\n",
        .mode = MM_UBLOX_NETWORKING_MODE_BRIDGE
    },
};

static void
test_ubmconf_response (void)
{
    guint i;

    for (i = 0; i < G_N_ELEMENTS (ubmconf_response_tests); i++) {
        MMUbloxNetworkingMode mode = MM_UBLOX_NETWORKING_MODE_UNKNOWN;
        GError *error = NULL;
        gboolean success;

        success = mm_ublox_parse_ubmconf_response (ubmconf_response_tests[i].str, &mode, &error);
        g_assert_no_error (error);
        g_assert (success);
        g_assert_cmpuint (ubmconf_response_tests[i].mode, ==, mode);
    }
}

/*****************************************************************************/
/* Test UIPADDR=N responses */

typedef struct {
    const gchar *str;
    guint        cid;
    const gchar *if_name;
    const gchar *ipv4_address;
    const gchar *ipv4_subnet;
    const gchar *ipv6_global_address;
    const gchar *ipv6_link_local_address;
} UipaddrResponseTest;

static const UipaddrResponseTest uipaddr_response_tests[] = {
    {
        .str = "+UIPADDR: 1,\"ccinet0\",\"5.168.120.13\",\"255.255.255.0\",\"\",\"\"",
        .cid = 1,
        .if_name = "ccinet0",
        .ipv4_address = "5.168.120.13",
        .ipv4_subnet = "255.255.255.0",
    },
    {
        .str = "+UIPADDR: 2,\"ccinet1\",\"\",\"\",\"2001::1:200:FF:FE00:0/64\",\"FE80::200:FF:FE00:0/64\"",
        .cid = 2,
        .if_name = "ccinet1",
        .ipv6_global_address = "2001::1:200:FF:FE00:0/64",
        .ipv6_link_local_address = "FE80::200:FF:FE00:0/64",
    },
    {
        .str = "+UIPADDR: 3,\"ccinet2\",\"5.10.100.2\",\"255.255.255.0\",\"2001::1:200:FF:FE00:0/64\",\"FE80::200:FF:FE00:0/64\"",
        .cid = 3,
        .if_name = "ccinet2",
        .ipv4_address = "5.10.100.2",
        .ipv4_subnet = "255.255.255.0",
        .ipv6_global_address = "2001::1:200:FF:FE00:0/64",
        .ipv6_link_local_address = "FE80::200:FF:FE00:0/64",
    },
};

static void
test_uipaddr_response (void)
{
    guint i;

    for (i = 0; i < G_N_ELEMENTS (uipaddr_response_tests); i++) {
        GError   *error = NULL;
        gboolean  success;
        guint     cid = G_MAXUINT;
        gchar    *if_name = NULL;
        gchar    *ipv4_address = NULL;
        gchar    *ipv4_subnet = NULL;
        gchar    *ipv6_global_address = NULL;
        gchar    *ipv6_link_local_address = NULL;

        success = mm_ublox_parse_uipaddr_response (uipaddr_response_tests[i].str,
                                                   &cid,
                                                   &if_name,
                                                   &ipv4_address,
                                                   &ipv4_subnet,
                                                   &ipv6_global_address,
                                                   &ipv6_link_local_address,
                                                   &error);
        g_assert_no_error (error);
        g_assert (success);
        g_assert_cmpuint (uipaddr_response_tests[i].cid,                     ==, cid);
        g_assert_cmpstr  (uipaddr_response_tests[i].if_name,                 ==, if_name);
        g_assert_cmpstr  (uipaddr_response_tests[i].ipv4_address,            ==, ipv4_address);
        g_assert_cmpstr  (uipaddr_response_tests[i].ipv4_subnet,             ==, ipv4_subnet);
        g_assert_cmpstr  (uipaddr_response_tests[i].ipv6_global_address,     ==, ipv6_global_address);
        g_assert_cmpstr  (uipaddr_response_tests[i].ipv6_link_local_address, ==, ipv6_link_local_address);

        g_free (if_name);
        g_free (ipv4_address);
        g_free (ipv4_subnet);
        g_free (ipv6_global_address);
        g_free (ipv6_link_local_address);
    }
}

/*****************************************************************************/
/* Test URAT=? responses and model based filtering */

static void
compare_combinations (const gchar                  *response,
                      const gchar                  *model,
                      const MMModemModeCombination *expected_combinations,
                      guint                         n_expected_combinations)
{
    GArray *combinations;
    GError *error = NULL;
    guint i;

    combinations = mm_ublox_parse_urat_test_response (response, &error);
    g_assert_no_error (error);
    g_assert (combinations);

    combinations = mm_ublox_filter_supported_modes (model, combinations, &error);
    g_assert_no_error (error);
    g_assert (combinations);

    g_assert_cmpuint (combinations->len, ==, n_expected_combinations);

    for (i = 0; i < combinations->len; i++) {
        MMModemModeCombination combination;
        guint                  j;
        gboolean               found = FALSE;

        combination = g_array_index (combinations, MMModemModeCombination, i);
        for (j = 0; !found && j < n_expected_combinations; j++)
            found = (combination.allowed == expected_combinations[j].allowed &&
                     combination.preferred == expected_combinations[j].preferred);
        g_assert (found);
    }

    g_array_unref (combinations);
}

static void
test_urat_test_response_2g (void)
{
    static const MMModemModeCombination expected_combinations[] = {
        { MM_MODEM_MODE_2G, MM_MODEM_MODE_NONE }
    };

    compare_combinations ("+URAT: 0",       NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
    compare_combinations ("+URAT: 0,0",     NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
    compare_combinations ("+URAT: (0)",     NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
    compare_combinations ("+URAT: (0),(0)", NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
}

static void
test_urat_test_response_2g3g (void)
{
    static const MMModemModeCombination expected_combinations[] = {
        { MM_MODEM_MODE_2G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_2G },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_3G },
    };

    compare_combinations ("+URAT: (0,1,2),(0,2)", NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
    compare_combinations ("+URAT: (0-2),(0,2)",   NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
}

static void
test_urat_test_response_2g3g4g (void)
{
    static const MMModemModeCombination expected_combinations[] = {
        { MM_MODEM_MODE_2G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_4G, MM_MODEM_MODE_NONE },

        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_2G },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_3G },

        { MM_MODEM_MODE_2G | MM_MODEM_MODE_4G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_4G, MM_MODEM_MODE_2G },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_4G, MM_MODEM_MODE_4G },

        { MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_3G },
        { MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_4G },

        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_2G },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_3G },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_4G },
    };

    compare_combinations ("+URAT: (0,1,2,3,4,5,6),(0,2,3)", NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
    compare_combinations ("+URAT: (0-6),(0,2,3)",           NULL, expected_combinations, G_N_ELEMENTS (expected_combinations));
}

static void
test_mode_filtering_toby_l201 (void)
{
    static const MMModemModeCombination expected_combinations[] = {
        { MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_4G, MM_MODEM_MODE_NONE },

        { MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_3G },
        { MM_MODEM_MODE_3G | MM_MODEM_MODE_4G, MM_MODEM_MODE_4G },
    };

    compare_combinations ("+URAT: (0-6),(0,2,3)", "TOBY-L201", expected_combinations, G_N_ELEMENTS (expected_combinations));
}

static void
test_mode_filtering_lisa_u200 (void)
{
    static const MMModemModeCombination expected_combinations[] = {
        { MM_MODEM_MODE_2G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },

        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_2G },
        { MM_MODEM_MODE_2G | MM_MODEM_MODE_3G, MM_MODEM_MODE_3G },
    };

    compare_combinations ("+URAT: (0-6),(0,2,3)", "LISA-U200", expected_combinations, G_N_ELEMENTS (expected_combinations));
}

static void
test_mode_filtering_sara_u280 (void)
{
    static const MMModemModeCombination expected_combinations[] = {
        { MM_MODEM_MODE_3G, MM_MODEM_MODE_NONE },
    };

    compare_combinations ("+URAT: (0-6),(0,2,3)", "SARA-U280", expected_combinations, G_N_ELEMENTS (expected_combinations));
}

/*****************************************************************************/

void
_mm_log (const char *loc,
         const char *func,
         guint32 level,
         const char *fmt,
         ...)
{
#if defined ENABLE_TEST_MESSAGE_TRACES
    /* Dummy log function */
    va_list args;
    gchar *msg;

    va_start (args, fmt);
    msg = g_strdup_vprintf (fmt, args);
    va_end (args);
    g_print ("%s\n", msg);
    g_free (msg);
#endif
}

int main (int argc, char **argv)
{
    setlocale (LC_ALL, "");

    g_type_init ();
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/MM/ublox/uusbconf/response", test_uusbconf_response);
    g_test_add_func ("/MM/ublox/ubmconf/response",  test_ubmconf_response);
    g_test_add_func ("/MM/ublox/uipaddr/response",  test_uipaddr_response);
    g_test_add_func ("/MM/ublox/urat/test/response/2g",        test_urat_test_response_2g);
    g_test_add_func ("/MM/ublox/urat/test/response/2g3g",      test_urat_test_response_2g3g);
    g_test_add_func ("/MM/ublox/urat/test/response/2g3g4g",    test_urat_test_response_2g3g4g);
    g_test_add_func ("/MM/ublox/urat/test/response/toby-l201", test_mode_filtering_toby_l201);
    g_test_add_func ("/MM/ublox/urat/test/response/lisa-u200", test_mode_filtering_lisa_u200);
    g_test_add_func ("/MM/ublox/urat/test/response/sara-u280", test_mode_filtering_sara_u280);

    return g_test_run ();
}