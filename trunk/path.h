/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Access to project specific data files.
 *
 * LICENSE:
 *  GNU GENERAL PUBLIC LICENSE version 2
 *
 *****************************************************************************/

#ifndef PATH_H__6A0C8189_7048_457D_9081_B0F76AD4B93C__INCLUDED
#define PATH_H__6A0C8189_7048_457D_9081_B0F76AD4B93C__INCLUDED

void
path_init(const char * argv0);

/* g_free the return value if it is not NULL */
gchar *
path_get_data_filename(const gchar * filename);

void
path_uninit();

#endif /* #ifndef PATH_H__6A0C8189_7048_457D_9081_B0F76AD4B93C__INCLUDED */
