/*
 * Copyright (c) 2016-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/event/event.h>
#include <lv2/lv2plug.in/ns/ext/morph/morph.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/instance-access/instance-access.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/port-props/port-props.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/resize-port/resize-port.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>

#ifdef ENABLE_ELF_TESTS
#	include <fcntl.h>
#	include <libelf.h>
#	include <gelf.h>
#endif

const char *colors [2][ANSI_COLOR_MAX] = {
	{
		[ANSI_COLOR_BOLD]    = "",
		[ANSI_COLOR_RED]     = "",
		[ANSI_COLOR_GREEN]   = "",
		[ANSI_COLOR_YELLOW]  = "",
		[ANSI_COLOR_BLUE]    = "",
		[ANSI_COLOR_MAGENTA] = "",
		[ANSI_COLOR_CYAN]    = "",
		[ANSI_COLOR_RESET]   = ""
	},
	{
		[ANSI_COLOR_BOLD]    = "\x1b[1m",
		[ANSI_COLOR_RED]     = "\x1b[31m",
		[ANSI_COLOR_GREEN]   = "\x1b[32m",
		[ANSI_COLOR_YELLOW]  = "\x1b[33m",
		[ANSI_COLOR_BLUE]    = "\x1b[34m",
		[ANSI_COLOR_MAGENTA] = "\x1b[35m",
		[ANSI_COLOR_CYAN]    = "\x1b[36m",
		[ANSI_COLOR_RESET]   = "\x1b[0m"
	}
};

static void
_map_uris(app_t *app)
{
	app->uris.rdfs_label = lilv_new_uri(app->world, LILV_NS_RDFS"label");
	app->uris.rdfs_comment = lilv_new_uri(app->world, LILV_NS_RDFS"comment");
	app->uris.rdfs_range = lilv_new_uri(app->world, LILV_NS_RDFS"range");
	app->uris.rdfs_subClassOf = lilv_new_uri(app->world, LILV_NS_RDFS"subClassOf");

	app->uris.rdf_type = lilv_new_uri(app->world, LILV_NS_RDF"type");

	app->uris.doap_description = lilv_new_uri(app->world, LILV_NS_DOAP"description");
	app->uris.doap_license = lilv_new_uri(app->world, LILV_NS_DOAP"license");
	app->uris.doap_name = lilv_new_uri(app->world, LILV_NS_DOAP"name");
	app->uris.doap_shortdesc = lilv_new_uri(app->world, LILV_NS_DOAP"shortdesc");

	app->uris.lv2_minimum = lilv_new_uri(app->world, LV2_CORE__minimum);
	app->uris.lv2_maximum = lilv_new_uri(app->world, LV2_CORE__maximum);
	app->uris.lv2_Port = lilv_new_uri(app->world, LV2_CORE__Port);
	app->uris.lv2_PortProperty = lilv_new_uri(app->world, LV2_CORE__PortProperty);
	app->uris.lv2_default = lilv_new_uri(app->world, LV2_CORE__default);
	app->uris.lv2_ControlPort = lilv_new_uri(app->world, LV2_CORE__ControlPort);
	app->uris.lv2_CVPort = lilv_new_uri(app->world, LV2_CORE__CVPort);
	app->uris.lv2_AudioPort = lilv_new_uri(app->world, LV2_CORE__AudioPort);
	app->uris.lv2_OutputPort = lilv_new_uri(app->world, LV2_CORE__OutputPort);
	app->uris.lv2_InputPort = lilv_new_uri(app->world, LV2_CORE__InputPort);
	app->uris.lv2_integer = lilv_new_uri(app->world, LV2_CORE__integer);
	app->uris.lv2_toggled = lilv_new_uri(app->world, LV2_CORE__toggled);
	app->uris.lv2_Feature = lilv_new_uri(app->world, LV2_CORE__Feature);
	app->uris.lv2_minorVersion = lilv_new_uri(app->world, LV2_CORE__minorVersion);
	app->uris.lv2_microVersion = lilv_new_uri(app->world, LV2_CORE__microVersion);
	app->uris.lv2_ExtensionData = lilv_new_uri(app->world, LV2_CORE__ExtensionData);
	app->uris.lv2_requiredFeature = lilv_new_uri(app->world, LV2_CORE__requiredFeature);
	app->uris.lv2_optionalFeature = lilv_new_uri(app->world, LV2_CORE__optionalFeature);
	app->uris.lv2_extensionData = lilv_new_uri(app->world, LV2_CORE__extensionData);
	app->uris.lv2_isLive = lilv_new_uri(app->world, LV2_CORE__isLive);
	app->uris.lv2_inPlaceBroken = lilv_new_uri(app->world, LV2_CORE__inPlaceBroken);
	app->uris.lv2_hardRTCapable = lilv_new_uri(app->world, LV2_CORE__hardRTCapable);
	app->uris.lv2_documentation = lilv_new_uri(app->world, LV2_CORE__documentation);

	app->uris.atom_AtomPort = lilv_new_uri(app->world, LV2_ATOM__AtomPort);
	app->uris.atom_Bool = lilv_new_uri(app->world, LV2_ATOM__Bool);
	app->uris.atom_Int = lilv_new_uri(app->world, LV2_ATOM__Int);
	app->uris.atom_Long = lilv_new_uri(app->world, LV2_ATOM__Long);
	app->uris.atom_Float = lilv_new_uri(app->world, LV2_ATOM__Float);
	app->uris.atom_Double = lilv_new_uri(app->world, LV2_ATOM__Double);
	app->uris.atom_String = lilv_new_uri(app->world, LV2_ATOM__String);
	app->uris.atom_Literal = lilv_new_uri(app->world, LV2_ATOM__String);
	app->uris.atom_Path = lilv_new_uri(app->world, LV2_ATOM__Path);
	app->uris.atom_Chunk = lilv_new_uri(app->world, LV2_ATOM__Chunk);
	app->uris.atom_URI = lilv_new_uri(app->world, LV2_ATOM__URI);
	app->uris.atom_URID = lilv_new_uri(app->world, LV2_ATOM__URID);
	app->uris.atom_Tuple = lilv_new_uri(app->world, LV2_ATOM__Tuple);
	app->uris.atom_Object = lilv_new_uri(app->world, LV2_ATOM__Object);
	app->uris.atom_Vector = lilv_new_uri(app->world, LV2_ATOM__Vector);
	app->uris.atom_Sequence = lilv_new_uri(app->world, LV2_ATOM__Sequence);

	app->uris.state_loadDefaultState = lilv_new_uri(app->world, LV2_STATE__loadDefaultState);
	app->uris.state_state = lilv_new_uri(app->world, LV2_STATE__state);
	app->uris.state_interface = lilv_new_uri(app->world, LV2_STATE__interface);
	app->uris.state_threadSafeRestore = lilv_new_uri(app->world, LV2_STATE_PREFIX"threadSafeRestore");
	app->uris.state_makePath = lilv_new_uri(app->world, LV2_STATE__makePath);

	app->uris.work_schedule = lilv_new_uri(app->world, LV2_WORKER__schedule);
	app->uris.work_interface = lilv_new_uri(app->world, LV2_WORKER__interface);

	app->uris.idisp_queue_draw = lilv_new_uri(app->world, LV2_INLINEDISPLAY__queue_draw);
	app->uris.idisp_interface = lilv_new_uri(app->world, LV2_INLINEDISPLAY__interface);

	app->uris.opts_options = lilv_new_uri(app->world, LV2_OPTIONS__options);
	app->uris.opts_interface = lilv_new_uri(app->world, LV2_OPTIONS__interface);
	app->uris.opts_requiredOption = lilv_new_uri(app->world, LV2_OPTIONS__requiredOption);
	app->uris.opts_supportedOption = lilv_new_uri(app->world, LV2_OPTIONS__supportedOption);

	app->uris.patch_writable = lilv_new_uri(app->world, LV2_PATCH__writable);
	app->uris.patch_readable = lilv_new_uri(app->world, LV2_PATCH__readable);
	app->uris.patch_Message = lilv_new_uri(app->world, LV2_PATCH__Message);

	app->uris.pg_group = lilv_new_uri(app->world, LV2_PORT_GROUPS__group);

	app->uris.ui_binary = lilv_new_uri(app->world, LV2_UI__binary);
	app->uris.ui_makeSONameResident = lilv_new_uri(app->world, LV2_UI_PREFIX"makeSONameResident");
	app->uris.ui_idleInterface = lilv_new_uri(app->world, LV2_UI__idleInterface);
	app->uris.ui_showInterface = lilv_new_uri(app->world, LV2_UI__showInterface);
	app->uris.ui_resize = lilv_new_uri(app->world, LV2_UI__resize);
	app->uris.ui_UI= lilv_new_uri(app->world, LV2_UI__UI);
	app->uris.ui_X11UI = lilv_new_uri(app->world, LV2_UI__X11UI);
	app->uris.ui_WindowsUI = lilv_new_uri(app->world, LV2_UI__WindowsUI);
	app->uris.ui_CocoaUI = lilv_new_uri(app->world, LV2_UI__CocoaUI);
	app->uris.ui_GtkUI = lilv_new_uri(app->world, LV2_UI__GtkUI);
	app->uris.ui_Gtk3UI = lilv_new_uri(app->world, LV2_UI__Gtk3UI);
	app->uris.ui_Qt4UI = lilv_new_uri(app->world, LV2_UI__Qt4UI);
	app->uris.ui_Qt5UI = lilv_new_uri(app->world, LV2_UI__Qt5UI);

	app->uris.event_EventPort = lilv_new_uri(app->world, LV2_EVENT__EventPort);
	app->uris.uri_map = lilv_new_uri(app->world, LV2_URI_MAP_URI);
	app->uris.instance_access = lilv_new_uri(app->world, LV2_INSTANCE_ACCESS_URI);
	app->uris.data_access = lilv_new_uri(app->world, LV2_DATA_ACCESS_URI);

	app->uris.log_log = lilv_new_uri(app->world, LV2_LOG__log);

	app->uris.urid_map = lilv_new_uri(app->world, LV2_URID__map);
	app->uris.urid_unmap = lilv_new_uri(app->world, LV2_URID__unmap);

	app->uris.rsz_resize = lilv_new_uri(app->world, LV2_RESIZE_PORT__resize);

	app->uris.bufsz_boundedBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__boundedBlockLength);
	app->uris.bufsz_fixedBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__fixedBlockLength);
	app->uris.bufsz_powerOf2BlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__powerOf2BlockLength);
	app->uris.bufsz_coarseBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE_PREFIX"coarseBlockLength");

	app->uris.pprops_supportsStrictBounds = lilv_new_uri(app->world, LV2_PORT_PROPS__supportsStrictBounds);

	app->uris.param_sampleRate = lilv_new_uri(app->world, LV2_PARAMETERS__sampleRate);

	app->uris.bufsz_minBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__minBlockLength);
	app->uris.bufsz_maxBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__maxBlockLength);
	app->uris.bufsz_nominalBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__nominalBlockLength);
	app->uris.bufsz_sequenceSize = lilv_new_uri(app->world, LV2_BUF_SIZE__sequenceSize);

	app->uris.ui_updateRate = lilv_new_uri(app->world, LV2_UI__updateRate);

	app->uris.ext_Widget = lilv_new_uri(app->world, LV2_EXTERNAL_UI__Widget);

	app->uris.morph_MorphPort = lilv_new_uri(app->world, LV2_MORPH__MorphPort);
	app->uris.morph_AutoMorphPort = lilv_new_uri(app->world, LV2_MORPH__AutoMorphPort);
	app->uris.morph_supportsType = lilv_new_uri(app->world, LV2_MORPH__supportsType);

	app->uris.units_unit = lilv_new_uri(app->world, LV2_UNITS__unit);
	app->uris.units_Unit = lilv_new_uri(app->world, LV2_UNITS__Unit);
}

static void
_unmap_uris(app_t *app)
{
	lilv_node_free(app->uris.rdfs_label);
	lilv_node_free(app->uris.rdfs_comment);
	lilv_node_free(app->uris.rdfs_range);
	lilv_node_free(app->uris.rdfs_subClassOf);

	lilv_node_free(app->uris.rdf_type);

	lilv_node_free(app->uris.doap_description);
	lilv_node_free(app->uris.doap_license);
	lilv_node_free(app->uris.doap_name);
	lilv_node_free(app->uris.doap_shortdesc);

	lilv_node_free(app->uris.lv2_minimum);
	lilv_node_free(app->uris.lv2_maximum);
	lilv_node_free(app->uris.lv2_Port);
	lilv_node_free(app->uris.lv2_PortProperty);
	lilv_node_free(app->uris.lv2_default);
	lilv_node_free(app->uris.lv2_ControlPort);
	lilv_node_free(app->uris.lv2_CVPort);
	lilv_node_free(app->uris.lv2_AudioPort);
	lilv_node_free(app->uris.lv2_OutputPort);
	lilv_node_free(app->uris.lv2_InputPort);
	lilv_node_free(app->uris.lv2_integer);
	lilv_node_free(app->uris.lv2_toggled);
	lilv_node_free(app->uris.lv2_Feature);
	lilv_node_free(app->uris.lv2_minorVersion);
	lilv_node_free(app->uris.lv2_microVersion);
	lilv_node_free(app->uris.lv2_ExtensionData);
	lilv_node_free(app->uris.lv2_requiredFeature);
	lilv_node_free(app->uris.lv2_optionalFeature);
	lilv_node_free(app->uris.lv2_extensionData);
	lilv_node_free(app->uris.lv2_isLive);
	lilv_node_free(app->uris.lv2_inPlaceBroken);
	lilv_node_free(app->uris.lv2_hardRTCapable);
	lilv_node_free(app->uris.lv2_documentation);

	lilv_node_free(app->uris.atom_AtomPort);
	lilv_node_free(app->uris.atom_Bool);
	lilv_node_free(app->uris.atom_Int);
	lilv_node_free(app->uris.atom_Long);
	lilv_node_free(app->uris.atom_Float);
	lilv_node_free(app->uris.atom_Double);
	lilv_node_free(app->uris.atom_String);
	lilv_node_free(app->uris.atom_Literal);
	lilv_node_free(app->uris.atom_Path);
	lilv_node_free(app->uris.atom_Chunk);
	lilv_node_free(app->uris.atom_URI);
	lilv_node_free(app->uris.atom_URID);
	lilv_node_free(app->uris.atom_Tuple);
	lilv_node_free(app->uris.atom_Object);
	lilv_node_free(app->uris.atom_Vector);
	lilv_node_free(app->uris.atom_Sequence);

	lilv_node_free(app->uris.state_loadDefaultState);
	lilv_node_free(app->uris.state_state);
	lilv_node_free(app->uris.state_interface);
	lilv_node_free(app->uris.state_threadSafeRestore);
	lilv_node_free(app->uris.state_makePath);

	lilv_node_free(app->uris.work_schedule);
	lilv_node_free(app->uris.work_interface);

	lilv_node_free(app->uris.idisp_queue_draw);
	lilv_node_free(app->uris.idisp_interface);

	lilv_node_free(app->uris.opts_options);
	lilv_node_free(app->uris.opts_interface);
	lilv_node_free(app->uris.opts_requiredOption);
	lilv_node_free(app->uris.opts_supportedOption);

	lilv_node_free(app->uris.patch_writable);
	lilv_node_free(app->uris.patch_readable);
	lilv_node_free(app->uris.patch_Message);

	lilv_node_free(app->uris.pg_group);

	lilv_node_free(app->uris.ui_binary);
	lilv_node_free(app->uris.ui_makeSONameResident);
	lilv_node_free(app->uris.ui_idleInterface);
	lilv_node_free(app->uris.ui_showInterface);
	lilv_node_free(app->uris.ui_resize);
	lilv_node_free(app->uris.ui_UI);
	lilv_node_free(app->uris.ui_X11UI);
	lilv_node_free(app->uris.ui_WindowsUI);
	lilv_node_free(app->uris.ui_CocoaUI);
	lilv_node_free(app->uris.ui_GtkUI);
	lilv_node_free(app->uris.ui_Gtk3UI);
	lilv_node_free(app->uris.ui_Qt4UI);
	lilv_node_free(app->uris.ui_Qt5UI);

	lilv_node_free(app->uris.event_EventPort);
	lilv_node_free(app->uris.uri_map);
	lilv_node_free(app->uris.instance_access);
	lilv_node_free(app->uris.data_access);

	lilv_node_free(app->uris.log_log);

	lilv_node_free(app->uris.urid_map);
	lilv_node_free(app->uris.urid_unmap);

	lilv_node_free(app->uris.rsz_resize);

	lilv_node_free(app->uris.bufsz_boundedBlockLength);
	lilv_node_free(app->uris.bufsz_fixedBlockLength);
	lilv_node_free(app->uris.bufsz_powerOf2BlockLength);
	lilv_node_free(app->uris.bufsz_coarseBlockLength);

	lilv_node_free(app->uris.pprops_supportsStrictBounds);

	lilv_node_free(app->uris.param_sampleRate);

	lilv_node_free(app->uris.bufsz_minBlockLength);
	lilv_node_free(app->uris.bufsz_maxBlockLength);
	lilv_node_free(app->uris.bufsz_nominalBlockLength);
	lilv_node_free(app->uris.bufsz_sequenceSize);

	lilv_node_free(app->uris.ui_updateRate);

	lilv_node_free(app->uris.ext_Widget);

	lilv_node_free(app->uris.morph_MorphPort);
	lilv_node_free(app->uris.morph_AutoMorphPort);
	lilv_node_free(app->uris.morph_supportsType);

	lilv_node_free(app->uris.units_unit);
	lilv_node_free(app->uris.units_Unit);
}

static LV2_URID
_map(LV2_URID_Map_Handle instance, const char *uri)
{
	app_t *app = instance;

	for(unsigned i = 0; i < app->nurids; i++)
	{
		urid_t *itm = &app->urids[i];

		if(!strcmp(itm->uri, uri))
			return i + 1;
	}

	app->nurids += 1;
	app->urids = realloc(app->urids, app->nurids*sizeof(urid_t));
	if(app->urids)
	{
		urid_t *itm = &app->urids[app->nurids - 1];
		itm->uri = strdup(uri);
		return app->nurids;
	}

	return 0; // failed
}

static const char *
_unmap(LV2_URID_Unmap_Handle instance, LV2_URID urid)
{
	app_t *app = instance;

	if( (urid > 0) && (urid <= app->nurids) )
	{
		urid_t *itm = &app->urids[urid - 1];
		return itm->uri;
	}

	return NULL; // failed
}

static void
_free_urids(app_t *app)
{
	for(unsigned i = 0; i < app->nurids; i++)
	{
		urid_t *itm = &app->urids[i];

		if(itm->uri)
			free(itm->uri);
	}
	free(app->urids);

	app->urids = NULL;
	app->nurids = 0;
}

static LV2_Worker_Status
_respond(LV2_Worker_Respond_Handle instance, uint32_t size, const void *data)
{
	app_t *app = instance;

	if(app->work_iface && app->work_iface->work_response)
		return app->work_iface->work_response(&app->instance, size, data);

	else return LV2_WORKER_ERR_UNKNOWN;
}

static LV2_Worker_Status
_sched(LV2_Worker_Schedule_Handle instance, uint32_t size, const void *data)
{
	app_t *app = instance;

	LV2_Worker_Status status = LV2_WORKER_SUCCESS;
	if(app->work_iface && app->work_iface->work)
		status |= app->work_iface->work(&app->instance, _respond, app, size, data);
	if(app->work_iface && app->work_iface->end_run)
		status |= app->work_iface->end_run(&app->instance);

	return status;
}

static int
_vprintf(void *data, LV2_URID type, const char *fmt, va_list args)
{
	vfprintf(stderr, fmt, args);

	return 0;
}

static int
_printf(void *data, LV2_URID type, const char *fmt, ...)
{
  va_list args;
	int ret;

  va_start (args, fmt);
	ret = _vprintf(data, type, fmt, args);
  va_end(args);

	return ret;
}

static char *
_mkpath(LV2_State_Make_Path_Handle instance, const char *abstract_path)
{
	char *absolute_path = NULL;

	if(asprintf(&absolute_path, "/tmp/%s", abstract_path) == -1)
		absolute_path = NULL;

	return absolute_path;
}

static LV2_Resize_Port_Status
_resize(LV2_Resize_Port_Feature_Data instance, uint32_t index, size_t size)
{
	return LV2_RESIZE_PORT_SUCCESS;
}

static uint32_t
_uri_to_id(LV2_URI_Map_Callback_Data instance, const char *map, const char *uri)
{
	app_t *app = instance;
	(void)map;

	return _map(app, uri);
}

static void
_queue_draw(LV2_Inline_Display_Handle instance)
{
	app_t *app = instance;
	(void)app;
}

static void
_version(void)
{
	fprintf(stderr,
		"--------------------------------------------------------------------\n"
		"This is free software: you can redistribute it and/or modify\n"
		"it under the terms of the Artistic License 2.0 as published by\n"
		"The Perl Foundation.\n"
		"\n"
		"This source is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"Artistic License 2.0 for more details.\n"
		"\n"
		"You should have received a copy of the Artistic License 2.0\n"
		"along the source as a COPYING file. If not, obtain it from\n"
		"http://www.perlfoundation.org/artistic_license_2_0.\n\n");
}

static void
_usage(char **argv)
{
	fprintf(stderr,
		"--------------------------------------------------------------------\n"
		"USAGE\n"
		"   %s [OPTIONS] {PLUGIN_URI}*\n"
		"\n"
		"OPTIONS\n"
		"   [-v]                     print version information\n"
		"   [-h]                     print usage information\n"
		"   [-d]                     show verbose test item documentation\n"
#ifdef ENABLE_ONLINE_TESTS
		"   [-o]                     run online test items\n"
		"   [-m]                     create mail to plugin author\n"
#endif

		"   [-S] warn|note|pass|all  show warnings, notes, passes or all\n"
		"   [-E] warn|note|all       treat warnings, notes or all as errors\n\n"
		, argv[0]);
}

#ifdef ENABLE_ONLINE_TESTS
static const char *http_prefix = "http://";
static const char *https_prefix = "https://";
static const char *ftp_prefix = "ftp://";
static const char *ftps_prefix = "ftps://";

bool
is_url(const char *uri)
{
	const bool is_http = strncmp(uri, http_prefix, strlen(http_prefix));
	const bool is_https = strncmp(uri, https_prefix, strlen(https_prefix));
	const bool is_ftp = strncmp(uri, ftp_prefix, strlen(ftp_prefix));
	const bool is_ftps = strncmp(uri, ftps_prefix, strlen(ftps_prefix));

	return is_http || is_https || is_ftp || is_ftps;
}

bool
test_url(app_t *app, const char *url)
{
	curl_easy_setopt(app->curl, CURLOPT_URL, url);
	curl_easy_setopt(app->curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(app->curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(app->curl, CURLOPT_CONNECTTIMEOUT, 10L); // secs
	curl_easy_setopt(app->curl, CURLOPT_TIMEOUT, 20L); //secs

	const CURLcode resp = curl_easy_perform(app->curl);

	long http_code;
	curl_easy_getinfo(app->curl, CURLINFO_RESPONSE_CODE, &http_code);

	if( (resp == CURLE_OK) && (http_code == 200) )
	{
		return true;
	}

	return false;
}
#endif

#ifdef ENABLE_ELF_TESTS
bool
test_visibility(const char *path, const char *description)
{
	bool desc = false;
	unsigned invalid = 0;

	const int fd = open(path, O_RDONLY);
	if(fd != -1)
	{
		elf_version(EV_CURRENT);

		Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
		if(elf)
		{
			for(Elf_Scn *scn = elf_nextscn(elf, NULL);
				scn;
				scn = elf_nextscn(elf, scn))
			{
				GElf_Shdr shdr;
				memset(&shdr, 0x0, sizeof(GElf_Shdr));
				gelf_getshdr(scn, &shdr);

				if( (shdr.sh_type == SHT_SYMTAB) || (shdr.sh_type == SHT_DYNSYM) )
				{
					/* found a symbol table, go print it. */
					Elf_Data *data = elf_getdata(scn, NULL);
					const unsigned count = shdr.sh_size / shdr.sh_entsize;

					/* print the symbol names */
					for(unsigned i = 0; i < count; i++)
					{
						GElf_Sym sym;
						memset(&sym, 0x0, sizeof(GElf_Sym));
						gelf_getsym(data, i, &sym);

						const bool is_global = GELF_ST_BIND(sym.st_info) == STB_GLOBAL;
						if(sym.st_value && is_global)
						{
							const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);

							if(!strcmp(name, description))
							{
								desc = true;
							}
							else if(strcmp(name, "_init")
								&& strcmp(name, "_fini")
								&& strcmp(name, "_edata")
								&& strcmp(name, "_end")
								&& strcmp(name, "__bss_start") )
							{
								//fprintf(stderr, "%s\n", name);
								invalid++;
							}
						}
					}

					break;
				}
			}
			elf_end(elf);
		}
		close(fd);
	}

	return !(!desc || invalid);
}
#endif

static void
_state_set_value(const char *symbol, void *data,
	const void *value, uint32_t size, uint32_t type)
{
	//FIXME
}

int
main(int argc, char **argv)
{
	static app_t app;
	app.atty = isatty(1);
	app.show = LINT_FAIL; // always report failed tests
	app.mask = LINT_FAIL; // always fail at failed tests

	fprintf(stderr,
		"%s "LV2LINT_VERSION"\n"
		"Copyright (c) 2016-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
		"Released under Artistic License 2.0 by Open Music Kontrollers\n",
		argv[0]);

	int c;
#ifdef ENABLE_ONLINE_TESTS
	while( (c = getopt(argc, argv, "vhdomS:E:") ) != -1)
#else
	while( (c = getopt(argc, argv, "vhdS:E:") ) != -1)
#endif
	{
		switch(c)
		{
			case 'v':
				_version();
				return 0;
			case 'h':
				_usage(argv);
				return 0;
			case 'd':
				app.debug = true;
				break;
#ifdef ENABLE_ONLINE_TESTS
			case 'o':
				app.online = true;
				break;
			case 'm':
				app.mailto = true;
				app.atty = false;
				break;
#endif
			case 'S':
				if(!strcmp(optarg, "warn"))
				{
					app.show |= LINT_WARN;
				}
				else if(!strcmp(optarg, "note"))
				{
					app.show |= LINT_NOTE;
				}
				else if(!strcmp(optarg, "pass"))
				{
					app.show |= LINT_PASS;
				}
				else if(!strcmp(optarg, "all"))
				{
					app.show |= LINT_WARN | LINT_NOTE | LINT_PASS;
				}
				break;
			case 'E':
				if(!strcmp(optarg, "warn"))
				{
					app.show |= LINT_WARN;
					app.mask |= LINT_WARN;
				}
				else if(!strcmp(optarg, "note"))
				{
					app.show |= LINT_NOTE;
					app.mask |= LINT_NOTE;
				}
				else if(!strcmp(optarg, "all"))
				{
					app.show |= LINT_WARN | LINT_NOTE;
					app.mask |= LINT_WARN | LINT_NOTE;
				}
				break;
			case '?':
				if( (optopt == 'S') || (optopt == 'E') )
					fprintf(stderr, "Option `-%c' requires an argument.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return -1;
			default:
				return -1;
		}
	}

	if(optind == argc) // no URI given
	{
		_usage(argv);
		return -1;
	}

#ifdef ENABLE_ONLINE_TESTS
	app.curl = curl_easy_init();
	if(!app.curl)
		return -1;
#endif

	app.world = lilv_world_new();
	if(!app.world)
		return -1;

	_map_uris(&app);

	lilv_world_load_all(app.world);

	LV2_URID_Map map = {
		.handle = &app,
		.map = _map
	};
	LV2_URID_Unmap unmap = {
		.handle = &app,
		.unmap = _unmap
	};
	LV2_Worker_Schedule sched = {
		.handle = &app,
		.schedule_work = _sched
	};
	LV2_Log_Log log = {
		.handle = &app,
		.printf = _printf,
		.vprintf = _vprintf
	};
	LV2_State_Make_Path mkpath = {
		.handle = &app,
		.path = _mkpath
	};
	LV2_Resize_Port_Resize rsz = {
		.data = &app,
		.resize = _resize
	};
	LV2_URI_Map_Feature urimap = {
		.callback_data = &app,
		.uri_to_id = _uri_to_id
	};
	LV2_Inline_Display queue_draw = {
		.handle = &app,
		.queue_draw = _queue_draw
	};

	const LV2_URID atom_Float = map.map(map.handle, LV2_ATOM__Float);
	const LV2_URID atom_Int = map.map(map.handle, LV2_ATOM__Int);
	const LV2_URID param_sampleRate = map.map(map.handle, LV2_PARAMETERS__sampleRate);
	const LV2_URID ui_updateRate = map.map(map.handle, LV2_UI__updateRate);
	const LV2_URID bufsz_minBlockLength = map.map(map.handle, LV2_BUF_SIZE__minBlockLength);
	const LV2_URID bufsz_maxBlockLength = map.map(map.handle, LV2_BUF_SIZE__maxBlockLength);
	const LV2_URID bufsz_nominalBlockLength = map.map(map.handle, LV2_BUF_SIZE_PREFIX"nominalBlockLength");
	const LV2_URID bufsz_sequenceSize = map.map(map.handle, LV2_BUF_SIZE__sequenceSize);

	const float param_sample_rate = 48000.f;
	const float ui_update_rate = 25.f;
	const int32_t bufsz_min_block_length = 256;
	const int32_t bufsz_max_block_length = 256;
	const int32_t bufsz_nominal_block_length = 256;
	const int32_t bufsz_sequence_size = 2048;

	const LV2_Options_Option opts_sampleRate = {
		.key = param_sampleRate,
		.size = sizeof(float),
		.type = atom_Float,
		.value = &param_sample_rate
	};

	const LV2_Options_Option opts_updateRate = {
		.key = ui_updateRate,
		.size = sizeof(float),
		.type = atom_Float,
		.value = &ui_update_rate
	};

	const LV2_Options_Option opts_minBlockLength = {
		.key = bufsz_minBlockLength,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_min_block_length
	};

	const LV2_Options_Option opts_maxBlockLength = {
		.key = bufsz_maxBlockLength,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_max_block_length
	};

	const LV2_Options_Option opts_nominalBlockLength = {
		.key = bufsz_nominalBlockLength,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_nominal_block_length
	};

	const LV2_Options_Option opts_sequenceSize = {
		.key = bufsz_sequenceSize,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_sequence_size
	};

	const LV2_Options_Option opts_sentinel = {
		.key = 0,
		.value =NULL
	};

#define MAX_OPTS  7
	LV2_Options_Option opts [MAX_OPTS];

	const LV2_Feature feat_map = {
		.URI = LV2_URID__map,
		.data = &map
	};
	const LV2_Feature feat_unmap = {
		.URI = LV2_URID__unmap,
		.data = &unmap
	};
	const LV2_Feature feat_sched = {
		.URI = LV2_WORKER__schedule,
		.data = &sched
	};
	const LV2_Feature feat_log = {
		.URI = LV2_LOG__log,
		.data = &log
	};
	const LV2_Feature feat_mkpath = {
		.URI = LV2_STATE__makePath,
		.data = &mkpath
	};
	const LV2_Feature feat_rsz = {
		.URI = LV2_RESIZE_PORT__resize,
		.data = &rsz
	};
	const LV2_Feature feat_opts = {
		.URI = LV2_OPTIONS__options,
		.data = opts
	};
	const LV2_Feature feat_urimap = {
		.URI = LV2_URI_MAP_URI,
		.data = &urimap
	};

	const LV2_Feature feat_islive = {
		.URI = LV2_CORE__isLive
	};
	const LV2_Feature feat_inplacebroken = {
		.URI = LV2_CORE__inPlaceBroken
	};
	const LV2_Feature feat_hardrtcapable = {
		.URI = LV2_CORE__hardRTCapable
	};
	const LV2_Feature feat_supportsstrictbounds = {
		.URI = LV2_PORT_PROPS__supportsStrictBounds
	};
	const LV2_Feature feat_boundedblocklength = {
		.URI = LV2_BUF_SIZE__boundedBlockLength
	};
	const LV2_Feature feat_fixedblocklength = {
		.URI = LV2_BUF_SIZE__fixedBlockLength
	};
	const LV2_Feature feat_powerof2blocklength = {
		.URI = LV2_BUF_SIZE__powerOf2BlockLength
	};
	const LV2_Feature feat_coarseblocklength = {
		.URI = LV2_BUF_SIZE_PREFIX"coarseBlockLength"
	};
	const LV2_Feature feat_loaddefaultstate = {
		.URI = LV2_STATE__loadDefaultState
	};
	const LV2_Feature feat_threadsaferestore = {
		.URI = LV2_STATE_PREFIX"threadSafeRestore"
	};
	const LV2_Feature feat_idispqueuedraw = {
		.URI = LV2_INLINEDISPLAY__queue_draw,
		.data = &queue_draw
	};

	int ret = 0;
	const LilvPlugin *plugins = lilv_world_get_all_plugins(app.world);
	if(plugins)
	{
		for(int i=optind; i<argc; i++)
		{
			const char *plugin_uri = argv[i];
			LilvNode *plugin_uri_node = lilv_new_uri(app.world, plugin_uri);
			if(plugin_uri_node)
			{
				app.plugin = lilv_plugins_get_by_uri(plugins, plugin_uri_node);
				if(app.plugin)
				{
#define MAX_FEATURES 20
					const LV2_Feature *features [MAX_FEATURES];
					bool requires_bounded_block_length = false;

					// populate feature list
					{
						int f = 0;

						LilvNodes *required_features = lilv_plugin_get_required_features(app.plugin);
						if(required_features)
						{
							LILV_FOREACH(nodes, itr, required_features)
							{
								const LilvNode *feature = lilv_nodes_get(required_features, itr);

								if(lilv_node_equals(feature, app.uris.urid_map))
									features[f++] = &feat_map;
								else if(lilv_node_equals(feature, app.uris.urid_unmap))
									features[f++] = &feat_unmap;
								else if(lilv_node_equals(feature, app.uris.work_schedule))
									features[f++] = &feat_sched;
								else if(lilv_node_equals(feature, app.uris.log_log))
									features[f++] = &feat_log;
								else if(lilv_node_equals(feature, app.uris.state_makePath))
									features[f++] = &feat_mkpath;
								else if(lilv_node_equals(feature, app.uris.rsz_resize))
									features[f++] = &feat_rsz;
								else if(lilv_node_equals(feature, app.uris.opts_options))
									features[f++] = &feat_opts;
								else if(lilv_node_equals(feature, app.uris.uri_map))
									features[f++] = &feat_urimap;
								else if(lilv_node_equals(feature, app.uris.lv2_isLive))
									features[f++] = &feat_islive;
								else if(lilv_node_equals(feature, app.uris.lv2_inPlaceBroken))
									features[f++] = &feat_inplacebroken;
								else if(lilv_node_equals(feature, app.uris.lv2_hardRTCapable))
									features[f++] = &feat_hardrtcapable;
								else if(lilv_node_equals(feature, app.uris.pprops_supportsStrictBounds))
									features[f++] = &feat_supportsstrictbounds;
								else if(lilv_node_equals(feature, app.uris.bufsz_boundedBlockLength))
								{
									features[f++] = &feat_boundedblocklength;
									requires_bounded_block_length = true;
								}
								else if(lilv_node_equals(feature, app.uris.bufsz_fixedBlockLength))
									features[f++] = &feat_fixedblocklength;
								else if(lilv_node_equals(feature, app.uris.bufsz_powerOf2BlockLength))
									features[f++] = &feat_powerof2blocklength;
								else if(lilv_node_equals(feature, app.uris.bufsz_coarseBlockLength))
									features[f++] = &feat_coarseblocklength;
								else if(lilv_node_equals(feature, app.uris.state_loadDefaultState))
									features[f++] = &feat_loaddefaultstate;
								else if(lilv_node_equals(feature, app.uris.state_threadSafeRestore))
									features[f++] = &feat_threadsaferestore;
								else if(lilv_node_equals(feature, app.uris.idisp_queue_draw))
									features[f++] = &feat_idispqueuedraw;
								else
								{
									//FIXME unknown feature
								}
							}
							lilv_nodes_free(required_features);
						}

						features[f++] = NULL; // sentinel
						assert(f <= MAX_FEATURES);
					}

					// populate required option list
					{
						unsigned n_opts = 0;
						bool requires_min_block_length = false;
						bool requires_max_block_length = false;

						LilvNodes *required_options = lilv_plugin_get_value(app.plugin, app.uris.opts_requiredOption);
						if(required_options)
						{
							LILV_FOREACH(nodes, itr, required_options)
							{
								const LilvNode *option = lilv_nodes_get(required_options, itr);

								if(lilv_node_equals(option, app.uris.param_sampleRate))
								{
									opts[n_opts++] = opts_sampleRate;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_minBlockLength))
								{
									opts[n_opts++] = opts_minBlockLength;
									requires_min_block_length = true;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_maxBlockLength))
								{
									opts[n_opts++] = opts_maxBlockLength;
									requires_max_block_length = true;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_nominalBlockLength))
								{
									opts[n_opts++] = opts_nominalBlockLength;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_sequenceSize))
								{
									opts[n_opts++] = opts_sequenceSize;
								}
								else if(lilv_node_equals(option, app.uris.ui_updateRate))
								{
									opts[n_opts++] = opts_updateRate;
								}
								else
								{
									//FIXME unknown option
								}
							}

							lilv_nodes_free(required_options);
						}

						// handle bufsz:boundedBlockLength feature which activates options itself
						if(requires_bounded_block_length)
						{
							if(!requires_min_block_length) // was not explicitely required
								opts[n_opts++] = opts_minBlockLength;

							if(!requires_max_block_length) // was not explicitely required
								opts[n_opts++] = opts_maxBlockLength;
						}

						opts[n_opts++] = opts_sentinel; // sentinel
						assert(n_opts <= MAX_OPTS);
					}

#ifdef ENABLE_ONLINE_TESTS
					if(app.mailto)
					{
						app.mail = calloc(1, sizeof(char));
					}
#endif

					lv2lint_printf(&app, "%s<%s>%s\n",
						colors[app.atty][ANSI_COLOR_BOLD],
						lilv_node_as_uri(lilv_plugin_get_uri(app.plugin)),
						colors[app.atty][ANSI_COLOR_RESET]);

					app.instance = lilv_plugin_instantiate(app.plugin, param_sample_rate, features);

					if(app.instance)
					{
						app.work_iface = lilv_instance_get_extension_data(app.instance, LV2_WORKER__interface);
						app.idisp_iface = lilv_instance_get_extension_data(app.instance, LV2_INLINEDISPLAY__interface);
						app.state_iface = lilv_instance_get_extension_data(app.instance, LV2_STATE__interface);
						app.opts_iface = lilv_instance_get_extension_data(app.instance, LV2_OPTIONS__interface);

						const bool has_load_default = lilv_plugin_has_feature(app.plugin,
							app.uris.state_loadDefaultState);
						if(has_load_default)
						{
							const LilvNode *pset = lilv_plugin_get_uri(app.plugin);

							lilv_world_load_resource(app.world, pset);

							LilvState *state = lilv_state_new_from_world(app.world, &map, pset);
							if(state)
							{
								lilv_state_restore(state, app.instance, _state_set_value, &app,
									LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, NULL); //FIXME features

								lilv_state_free(state);
							}

							lilv_world_unload_resource(app.world, pset);
						}
					}

					if(!test_plugin(&app))
					{
#ifdef ENABLE_ONLINE_TESTS // only print mailto strings if errors were encountered
						if(app.mailto && app.mail)
						{
							char *subj;
							if(asprintf(&subj, "[%s "LV2LINT_VERSION"] Report for <%s>", argv[0], plugin_uri) != -1)
							{
								char *subj_esc = curl_easy_escape(app.curl, subj, strlen(subj));
								if(subj_esc)
								{
									char *body_esc = curl_easy_escape(app.curl, app.mail, strlen(app.mail));
									if(body_esc)
									{
										LilvNode *email_node = lilv_plugin_get_author_email(app.plugin);
										const char *email = email && lilv_node_is_uri(email_node)
											? lilv_node_as_uri(email_node)
											: "mailto:unknown@example.com";

										fprintf(stdout, "%s?subject=%s&body=%s\n",
											email, subj_esc, body_esc);

										if(email_node)
										{
											lilv_node_free(email_node);
										}

										curl_free(body_esc);
									}

									curl_free(subj_esc);
								}

								free(subj);
							}
						}
#endif

						ret += 1;
					}

#ifdef ENABLE_ONLINE_TESTS
					if(app.mail)
					{
						free(app.mail);
						app.mail = NULL;
					}
#endif

					if(app.instance)
					{
						lilv_instance_free(app.instance);
						app.instance = NULL;
						app.work_iface = NULL;
						app.idisp_iface = NULL;
						app.state_iface= NULL;
						app.opts_iface = NULL;
					}

					app.plugin = NULL;

				}
				else
				{
					ret += 1;
				}
			}
			else
			{
				ret += 1;
			}
			lilv_node_free(plugin_uri_node);
		}
	}
	else
	{
		ret = -1;
	}

	_unmap_uris(&app);
	_free_urids(&app);

	lilv_world_free(app.world);

#ifdef ENABLE_ONLINE_TESTS
	curl_easy_cleanup(app.curl);
#endif

	return ret;
}

int
lv2lint_vprintf(app_t *app, const char *fmt, va_list args)
{
#ifdef ENABLE_ONLINE_TESTS
	if(app->mailto)
	{
		char *buf = NULL;
		int len;

		if( (len = vasprintf(&buf, fmt, args)) != -1)
		{
			len += strlen(app->mail);
			app->mail = realloc(app->mail, len + 1);

			if(app->mail)
			{
				app->mail = strncat(app->mail, buf, len + 1);
			}

			free(buf);
		}
	}
	else
#endif
	{
		vfprintf(stdout, fmt, args);
	}

	return 0;
}

int
lv2lint_printf(app_t *app, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	const int ret = lv2lint_vprintf(app, fmt, args);

	va_end(args);

	return ret;
}

static void
_escape_markup(char *docu)
{
	char *wrp = docu;
	bool tag = false;
	bool amp = false;
	bool sep = false;

	for(const char *rdp = docu; *rdp != '\0'; rdp++)
	{
		switch(*rdp)
		{
			case '<':
			{
				tag = true;
			} continue;
			case '>':
			{
				if(tag)
				{
					tag = false;
					continue;
				}
			} break;

			case '&':
			{
				amp = true;
			} continue;
			case ';':
			{
				if(amp)
				{
					amp = false;
					continue;
				}
			} break;

			case ' ':
			{
				if(sep) // escape double spaces
				{
					continue;
				}

				sep = true;
			} break;

			default:
			{
				sep = false;
			} break;
		}

		if(tag || amp)
		{
			continue;
		}

		if(wrp != rdp)
		{
			*wrp++ = *rdp;
		}
	}

	*wrp = '\0';
}

static void
_report_head(app_t *app, const char *label, ansi_color_t col, const test_t *test)
{
	lv2lint_printf(app, "    [%s%s%s]  %s\n",
		colors[app->atty][col], label, colors[app->atty][ANSI_COLOR_RESET], test->id);
}

static void
_report_body(app_t *app, const char *label, ansi_color_t col, const test_t *test,
	const ret_t *ret, const char *repl, char *docu)
{
	const char *sep = "\n";

	_report_head(app, label, col, test);

	lv2lint_printf(app, "              %s\n", repl ? repl : ret->msg);

	if(docu)
	{
		_escape_markup(docu);

		for(const char *ptr = strtok(docu, sep); ptr; ptr = strtok(NULL, sep))
		{
			lv2lint_printf(app, "                %s\n", ptr);
		}
	}

	lv2lint_printf(app, "              seeAlso: <%s>\n", ret->uri);
}

void
lv2lint_report(app_t *app, const test_t *test, res_t *res, bool show_passes, bool *flag)
{
	const ret_t *ret = res->ret;

	if(ret)
	{
		char *repl = NULL;

		if(res->urn)
		{
			if(strstr(ret->msg, "%s"))
			{
				if(asprintf(&repl, ret->msg, res->urn) == -1)
					repl = NULL;
			}

			free(res->urn);
		}

		char *docu = NULL;

		if(app->debug)
		{
			if(ret->dsc)
			{
				docu = strdup(ret->dsc);
			}
			else
			{
				LilvNode *subj_node = ret->uri ? lilv_new_uri(app->world, ret->uri) : NULL;
				if(subj_node)
				{
					LilvNode *docu_node = lilv_world_get(app->world, subj_node, app->uris.lv2_documentation, NULL);
					if(docu_node)
					{
						if(lilv_node_is_string(docu_node))
						{
							docu = strdup(lilv_node_as_string(docu_node));
						}

						lilv_node_free(docu_node);
					}

					lilv_node_free(subj_node);
				}
			}
		}

		switch(ret->lnt & app->show)
		{
			case LINT_FAIL:
				_report_body(app, "FAIL", ANSI_COLOR_RED, test, ret, repl, docu);
				break;
			case LINT_WARN:
				_report_body(app, "WARN", ANSI_COLOR_YELLOW, test, ret, repl, docu);
				break;
			case LINT_NOTE:
				_report_body(app, "NOTE", ANSI_COLOR_CYAN, test, ret, repl, docu);
				break;
		}

		if(docu)
		{
			free(docu);
		}

		if(repl)
		{
			free(repl);
		}

		if(flag && *flag)
		{
			*flag = (ret->lnt & app->mask) ? false : true;
		}
	}
	else if(show_passes)
	{
		_report_head(app, "PASS", ANSI_COLOR_GREEN, test);
	}
}
