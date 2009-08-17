
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1

#include "PropSetFile.h"
#include "properties.h"

// Global property bank for anjuta.
static GList *anjuta_propset;

static PropSetFile*
get_propset(PropsID pi)
{
  PropSetFile* p;
  if(pi < 0 || (guint)pi >= g_list_length(anjuta_propset))
  {
  	DEBUG_PRINT("%s", "Invalid PropSetFile handle");
  	return NULL;
  }
  p = (PropSetFile*)g_list_nth_data(anjuta_propset, pi);
  if (p == NULL)
  	DEBUG_PRINT("%s", "Trying to access already destroyed PropSetFile object");
  return p;
}

// Followings are the C++ to C interface for the PropSetFile

PropsID
sci_prop_set_new(void)
{
   PropsID handle;
   PropSetFile *p;
   gint length;

   length = g_list_length(anjuta_propset);
   p = new PropSetFile;
   anjuta_propset = g_list_append(anjuta_propset, (gpointer)p);
   handle = g_list_length(anjuta_propset);
   if (length == handle)
   {
   	DEBUG_PRINT("%s", "Unable to create PropSetFile Object");
   	return -1;
   }
   return handle-1;
}

gpointer
sci_prop_get_pointer(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  return (gpointer)p;
}

void
sci_prop_set_destroy(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  g_list_nth(anjuta_propset, handle)->data = NULL;
  delete p;
}

void
sci_prop_set_parent(PropsID handle1, PropsID handle2)
{
  PropSetFile *p1, *p2;
  p1 = get_propset(handle1);
  p2 = get_propset(handle2);
  if(!p1 || !p2) return;
  p1->superPS = p2;
}

void
sci_prop_set_with_key(PropsID handle, const gchar *key, const gchar *val)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  if(val)
	  p->Set(key, val);
  else
	  p->Set(key, "");
}

void 
sci_prop_set_int_with_key (PropsID p, const gchar *key, int value)
{
  gchar *str;
  str = g_strdup_printf ("%d", value);
  sci_prop_set_with_key (p, key, str);
  g_free (str);  
}

void
sci_prop_set(PropsID handle, const gchar *keyval)
{
  PropSetFile* p;
  p = get_propset (handle);
  if(!p) return;
  p->Set(keyval);
}

gchar*
sci_prop_get(PropsID handle, const gchar *key)
{
  PropSetFile* p;
  SString s;
  if (!key) return NULL;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->Get(key);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
sci_prop_get_expanded(PropsID handle, const gchar *key)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetExpanded(key);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
sci_prop_expand(PropsID handle, const gchar *withvars)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->Expand(withvars);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

int
sci_prop_get_int(PropsID handle, const gchar *key, gint defaultValue=0)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return defaultValue;
  return p->GetInt(key, defaultValue);
}

gchar*
sci_prop_get_wild(PropsID handle, const gchar *keybase, const gchar *filename)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetWild(keybase, filename);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

gchar*
sci_prop_get_new_expand(PropsID handle, const gchar *keybase, const gchar *filename)
{
  PropSetFile* p;
  SString s;
  p = get_propset(handle);
  if(!p) return NULL;
  s = p->GetNewExpand(keybase, filename);
  if (strlen(s.c_str()) == 0) return NULL;
  return g_strdup(s.c_str());
}

/* GList of strings operations */
static GList *
sci_prop_glist_from_string (const gchar *string)
{
	gchar *str, *temp, buff[256];
	GList *list;
	gchar *word_start, *word_end;
	gboolean the_end;

	list = NULL;
	the_end = FALSE;
	temp = g_strdup (string);
	str = temp;
	if (!str)
		return NULL;

	while (1)
	{
		gint i;
		gchar *ptr;

		/* Remove leading spaces */
		while (isspace (*str) && *str != '\0')
			str++;
		if (*str == '\0')
			break;

		/* Find start and end of word */
		word_start = str;
		while (!isspace (*str) && *str != '\0')
			str++;
		word_end = str;

		/* Copy the word into the buffer */
		for (ptr = word_start, i = 0; ptr < word_end; ptr++, i++)
			buff[i] = *ptr;
		buff[i] = '\0';
		if (strlen (buff))
			list = g_list_append (list, g_strdup (buff));
		if (*str == '\0')
			break;
	}
	if (temp)
		g_free (temp);
	return list;
}

/* Get the list of strings as GList from a property value.
   Strings are splitted from white spaces */
GList *
sci_prop_glist_from_data (guint props, const gchar *id)
{
	gchar *str;
	GList *list;

	str = sci_prop_get (props, id);
	list = sci_prop_glist_from_string (str);
	g_free(str);
	return list;
}

void
sci_prop_clear(PropsID handle)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Clear();
}

void
sci_prop_read_from_memory(PropsID handle, const gchar *data, gint len,
		const gchar *directoryForImports=0)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->ReadFromMemory(data, len, directoryForImports);
}

void
sci_prop_read(PropsID handle, const gchar *filename, const gchar *directoryForImports)
{
  PropSetFile* p;
  p = get_propset(handle);
  if(!p) return;
  p->Read( filename, directoryForImports);
}
