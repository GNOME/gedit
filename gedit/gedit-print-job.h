/*
 * gedit-print-job.h
 * This file is part of gedit
 *
 * Copyright (C) 2000-2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2008 Paolo Maggi
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

#ifndef GEDIT_PRINT_JOB_H
#define GEDIT_PRINT_JOB_H

#include <tepl/tepl.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PRINT_JOB (gedit_print_job_get_type())

G_DECLARE_FINAL_TYPE (GeditPrintJob, gedit_print_job,
		      GEDIT, PRINT_JOB,
		      GObject)

typedef enum
{
	GEDIT_PRINT_JOB_STATUS_PAGINATING,
	GEDIT_PRINT_JOB_STATUS_DRAWING
} GeditPrintJobStatus;

typedef enum
{
	GEDIT_PRINT_JOB_RESULT_OK,
	GEDIT_PRINT_JOB_RESULT_CANCEL,
	GEDIT_PRINT_JOB_RESULT_ERROR
} GeditPrintJobResult;

GeditPrintJob		*gedit_print_job_new			(TeplView *view);

GtkPrintOperationResult	 gedit_print_job_print			(GeditPrintJob            *job,
								 GtkPrintOperationAction   action,
								 GtkPageSetup             *page_setup,
								 GtkPrintSettings         *settings,
								 GtkWindow                *parent,
								 GError                  **error);

void			 gedit_print_job_cancel			(GeditPrintJob *job);

const gchar		*gedit_print_job_get_status_string	(GeditPrintJob *job);

gdouble			 gedit_print_job_get_progress		(GeditPrintJob *job);

GtkPrintSettings	*gedit_print_job_get_print_settings	(GeditPrintJob *job);

GtkPageSetup		*gedit_print_job_get_page_setup		(GeditPrintJob *job);

G_END_DECLS

#endif /* GEDIT_PRINT_JOB_H */

/* ex:set ts=8 noet: */
