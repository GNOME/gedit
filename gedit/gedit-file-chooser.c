/*
 * This file is part of gedit
 *
 * Copyright (C) 2020 Sébastien Wilmet <swilmet@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "gedit-file-chooser.h"
#include <glib/gi18n.h>
#include <gtksourceview/gtksource.h>
#include "gedit-settings.h"

/* Common code between the different GeditFileChooser's. */

struct _GeditFileChooserPrivate
{
	GtkFileChooser *gtk_chooser;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditFileChooser, _gedit_file_chooser, G_TYPE_OBJECT)

#define ALL_FILES		_("All Files")
#define ALL_TEXT_FILES		_("All Text Files")

/* See the GtkFileChooserNative documentation, a GtkFileFilter with mime-types
 * is not supported on Windows.
 */
#ifdef G_OS_WIN32
#  define MIME_TYPES_ARE_SUPPORTED FALSE
#else
#  define MIME_TYPES_ARE_SUPPORTED TRUE
#endif

#if MIME_TYPES_ARE_SUPPORTED
/* Returns: (transfer none) (element-type utf8): a list containing "text/plain"
 * first and then the list of mime-types unrelated to "text/plain" that
 * GtkSourceView supports for the syntax highlighting.
 */
static GSList *
get_supported_mime_types (void)
{
	static GSList *supported_mime_types = NULL;
	static gboolean initialized = FALSE;

	GtkSourceLanguageManager *language_manager;
	const gchar * const *language_ids;
	gint language_num;

	if (initialized)
	{
		return supported_mime_types;
	}

	language_manager = gtk_source_language_manager_get_default ();
	language_ids = gtk_source_language_manager_get_language_ids (language_manager);
	for (language_num = 0; language_ids != NULL && language_ids[language_num] != NULL; language_num++)
	{
		const gchar *cur_language_id = language_ids[language_num];
		GtkSourceLanguage *language;
		gchar **mime_types;
		gint mime_type_num;

		language = gtk_source_language_manager_get_language (language_manager, cur_language_id);
		mime_types = gtk_source_language_get_mime_types (language);

		if (mime_types == NULL)
		{
			continue;
		}

		for (mime_type_num = 0; mime_types[mime_type_num] != NULL; mime_type_num++)
		{
			const gchar *cur_mime_type = mime_types[mime_type_num];

			if (!g_content_type_is_a (cur_mime_type, "text/plain"))
			{
				//g_message ("Mime-type '%s' is not related to 'text/plain'", cur_mime_type);
				supported_mime_types = g_slist_prepend (supported_mime_types,
									g_strdup (cur_mime_type));
			}
		}

		g_strfreev (mime_types);
	}

	// Note that all "text/*" mime-types are subclasses of "text/plain", see:
	// https://specifications.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-latest.html#subclassing
	supported_mime_types = g_slist_prepend (supported_mime_types, g_strdup ("text/plain"));

	initialized = TRUE;
	return supported_mime_types;
}

#else

static const gchar * const *
get_supported_globs (void)
{
	/* List generated with Tepl's shared-mime-info-list-text-plain-globs
	 * tool.
	 *
	 * TODO: can be improved by including globs from GtkSourceLanguage's.
	 */
	static const gchar * const supported_globs[] =
	{
		"*.abw",
		"*.adb",
		"*.ads",
		"*.al",
		"*.asc",
		"*.asp",
		"*.ass",
		"*.atom",
		"*.automount",
		"*.awk",
		"*.bib",
		"*.build",
		"*.c",
		"*.c++",
		"*.cbl",
		"*.cc",
		"*.ccmx",
		"*.cl",
		"*.cls",
		"*.cmake",
		"*.cob",
		"*.coffee",
		"*.cpp",
		"*.cs",
		"*.csh",
		"*.css",
		"*.csv",
		"*.csvs",
		"*.cue",
		"*.cxx",
		"*.d",
		"*.dbk",
		"*.dcl",
		"*.desktop",
		"*.device",
		"*.di",
		"*.dia",
		"*.diff",
		"*.docbook",
		"*.dot",
		"*.dsl",
		"*.dtd",
		"*.dtx",
		"*.e",
		"*.eif",
		"*.el",
		"*.eml",
		"*.ent",
		"*.eps",
		"*.epsf",
		"*.epsi",
		"*.erl",
		"*.es",
		"*.etx",
		"*.f",
		"*.f90",
		"*.f95",
		"*.fb2",
		"*.feature",
		"*.fl",
		"*.flatpakref",
		"*.flatpakrepo",
		"*.fo",
		"*.fodg",
		"*.fodp",
		"*.fods",
		"*.fodt",
		"*.for",
		"*.gcode",
		"*.gcrd",
		"*.geojson",
		"*.glade",
		"*.gml",
		"*.gnuplot",
		"*.go",
		"*.gp",
		"*.gpg",
		"*.gplt",
		"*.gpx",
		"*.gradle",
		"*.groovy",
		"*.gs",
		"*.gsf",
		"*.gsh",
		"*.gv",
		"*.gvp",
		"*.gvy",
		"*.gy",
		"*.h",
		"*.h++",
		"*.hh",
		"*.hp",
		"*.hpp",
		"*.hs",
		"*.htm",
		"*.html",
		"*.hxx",
		"*.ica",
		"*.ics",
		"*.idl",
		"*.iges",
		"*.igs",
		"*.ime",
		"*.imy",
		"*.ins",
		"*.iptables",
		"*.ipynb",
		"*.it87",
		"*.jad",
		"*.java",
		"*.jnlp",
		"*.jrd",
		"*.js",
		"*.jsm",
		"*.json",
		"*.jsonld",
		"*.json-patch",
		"*.kdelnk",
		"*.key",
		"*.kino",
		"*.kml",
		"*.la",
		"*.latex",
		"*.ldif",
		"*.lhs",
		"*.log",
		"*.ltx",
		"*.lua",
		"*.ly",
		"*.lyx",
		"*.m",
		"*.m1u",
		"*.m3u",
		"*.m3u8",
		"*.m4",
		"*.m4u",
		"*.mab",
		"*.mak",
		"*.man",
		"*.manifest",
		"*.markdown",
		"*.mbox",
		"*.mc2",
		"*.md",
		"*.me",
		"*.meta4",
		"*.metalink",
		"*.mgp",
		"*.mjs",
		"*.mk",
		"*.mkd",
		"*.ml",
		"*.mli",
		"*.mm",
		"*.mml",
		"*.mo",
		"*.moc",
		"*.mof",
		"*.mount",
		"*.mrl",
		"*.mrml",
		"*.ms",
		"*.mup",
		"*.mxu",
		"*.nb",
		"*.nfo",
		"*.not",
		"*.nzb",
		"*.ocl",
		"*.ooc",
		"*.opml",
		"*.owl",
		"*.owx",
		"*.p",
		"*.p7s",
		"*.pas",
		"*.patch",
		"*.path",
		"*.perl",
		"*.pfa",
		"*.pfb",
		"*.pgn",
		"*.pgp",
		"*.php",
		"*.php3",
		"*.php4",
		"*.php5",
		"*.phps",
		"*.pkr",
		"*.pl",
		"*.pm",
		"*.po",
		"*.pod",
		"*.pot",
		"*.ps",
		"*.py",
		"*.py3",
		"*.py3x",
		"*.pyx",
		"*.qml",
		"*.qmlproject",
		"*.qmltypes",
		"*.qti",
		"*.raml",
		"*.rb",
		"*.rdf",
		"*.rdfs",
		"*.rej",
		"*.rnc",
		"*.rng",
		"*.roff",
		"*.rs",
		"*.rss",
		"*.rst",
		"*.rt",
		"*.rtf",
		"*.rtx",
		"*.sami",
		"*.sass",
		"*.scala",
		"*.scm",
		"*.scope",
		"*.scss",
		"*.sdp",
		"*.service",
		"*.sgf",
		"*.sgm",
		"*.sgml",
		"*.sh",
		"*.shape",
		"*.sig",
		"*.siv",
		"*.skr",
		"*.slice",
		"*.slk",
		"*.smi",
		"*.smil",
		"*.sml",
		"*.socket",
		"*.spec",
		"*.sql",
		"*.src",
		"*.srt",
		"*.ss",
		"*.ssa",
		"*.sty",
		"*.sub",
		"*.sv",
		"*.svg",
		"*.svh",
		"*.swap",
		"*.sylk",
		"*.t",
		"*.t2t",
		"*.target",
		"*.tcl",
		"*.tex",
		"*.texi",
		"*.texinfo",
		"*.theme",
		"*.timer",
		"*.tk",
		"*.toc",
		"*.tr",
		"*.trig",
		"*.ts",
		"*.tsv",
		"*.ttl",
		"*.ttx",
		"*.twig",
		"*.txt",
		"*.ufraw",
		"*.ui",
		"*.uil",
		"*.uue",
		"*.v",
		"*.vala",
		"*.vapi",
		"*.vcard",
		"*.vcf",
		"*.vcs",
		"*.vct",
		"*.vhd",
		"*.vhdl",
		"*.vlc",
		"*.vrm",
		"*.vrml",
		"*.vtt",
		"*.wml",
		"*.wmls",
		"*.wrl",
		"*.wsgi",
		"*.xbel",
		"*.xbl",
		"*.xht",
		"*.xhtml",
		"*.xlf",
		"*.xliff",
		"*.xmi",
		"*.xml",
		"*.xsd",
		"*.xsl",
		"*.xslfo",
		"*.xslt",
		"*.xspf",
		"*.xul",
		"*.yaml",
		"*.yml",
		"*.zabw",
		NULL
	};

	return supported_globs;
}
#endif

static GtkFileFilter *
create_all_text_files_filter (void)
{
	GtkFileFilter *filter;

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_TEXT_FILES);

#if MIME_TYPES_ARE_SUPPORTED
	{
		GSList *supported_mime_types;
		GSList *l;

		supported_mime_types = get_supported_mime_types ();
		for (l = supported_mime_types; l != NULL; l = l->next)
		{
			const gchar *mime_type = l->data;

			gtk_file_filter_add_mime_type (filter, mime_type);
		}

		/* FIXME: use globs too - Paolo (Aug. 27, 2007).
		 *
		 * Yes, use globs too - swilmet (June 2020). Get the list of
		 * globs from GtkSourceLanguage's, and add them to the filter
		 * (only those that are not covered by a previously added
		 * mime-type). Note that the list of extra globs here must be
		 * computed differently than the list of extra globs for
		 * get_supported_globs() (see the TODO comment there).
		 */
	}
#else
	{
		const gchar * const *supported_globs;
		gint i;

		supported_globs = get_supported_globs ();
		for (i = 0; supported_globs != NULL && supported_globs[i] != NULL; i++)
		{
			const gchar *glob = supported_globs[i];

			gtk_file_filter_add_pattern (filter, glob);
		}
	}
#endif

	return filter;
}

static void
notify_filter_cb (GtkFileChooser *gtk_chooser,
		  GParamSpec     *pspec,
		  gpointer        user_data)
{
	GtkFileFilter *filter;
	const gchar *name;
	gint id = 0;
	GeditSettings *settings;
	GSettings *file_chooser_state_settings;

	/* Remember the selected filter. */

	filter = gtk_file_chooser_get_filter (gtk_chooser);
	if (filter == NULL)
	{
		return;
	}

	name = gtk_file_filter_get_name (filter);
	if (g_strcmp0 (name, ALL_FILES) == 0)
	{
		id = 1;
	}

	settings = _gedit_settings_get_singleton ();
	file_chooser_state_settings = _gedit_settings_peek_file_chooser_state_settings (settings);
	g_settings_set_int (file_chooser_state_settings, GEDIT_SETTINGS_ACTIVE_FILE_FILTER, id);
}

static void
setup_filters (GeditFileChooser *chooser)
{
	GeditSettings *settings;
	GSettings *file_chooser_state_settings;
	gint active_filter;
	GtkFileFilter *filter;

	settings = _gedit_settings_get_singleton ();
	file_chooser_state_settings = _gedit_settings_peek_file_chooser_state_settings (settings);
	active_filter = g_settings_get_int (file_chooser_state_settings, GEDIT_SETTINGS_ACTIVE_FILE_FILTER);

	/* "All Text Files" filter */
	filter = create_all_text_files_filter ();

	g_object_ref_sink (filter);
	gtk_file_chooser_add_filter (chooser->priv->gtk_chooser, filter);
	if (active_filter != 1)
	{
		/* Use this filter if set by user and as default. */
		gtk_file_chooser_set_filter (chooser->priv->gtk_chooser, filter);
	}
	g_object_unref (filter);

	/* "All Files" filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_FILES);
	gtk_file_filter_add_pattern (filter, "*");

	g_object_ref_sink (filter);
	gtk_file_chooser_add_filter (chooser->priv->gtk_chooser, filter);
	if (active_filter == 1)
	{
		/* Use this filter if set by user. */
		gtk_file_chooser_set_filter (chooser->priv->gtk_chooser, filter);
	}
	g_object_unref (filter);

	g_signal_connect (chooser->priv->gtk_chooser,
			  "notify::filter",
			  G_CALLBACK (notify_filter_cb),
			  NULL);
}

static void
_gedit_file_chooser_dispose (GObject *object)
{
	GeditFileChooser *chooser = GEDIT_FILE_CHOOSER (object);

	g_clear_object (&chooser->priv->gtk_chooser);

	G_OBJECT_CLASS (_gedit_file_chooser_parent_class)->dispose (object);
}

static void
_gedit_file_chooser_class_init (GeditFileChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_file_chooser_dispose;
}

static void
_gedit_file_chooser_init (GeditFileChooser *chooser)
{
	chooser->priv = _gedit_file_chooser_get_instance_private (chooser);
}

GeditFileChooser *
_gedit_file_chooser_new (void)
{
	return g_object_new (GEDIT_TYPE_FILE_CHOOSER, NULL);
}

void
_gedit_file_chooser_set_gtk_file_chooser (GeditFileChooser *chooser,
					  GtkFileChooser   *gtk_chooser)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER (chooser));
	g_return_if_fail (GTK_IS_FILE_CHOOSER (gtk_chooser));
	g_return_if_fail (chooser->priv->gtk_chooser == NULL);

	chooser->priv->gtk_chooser = g_object_ref (gtk_chooser);

	setup_filters (chooser);
}
