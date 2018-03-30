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

#include <math.h>

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/event/event.h>
#include <lv2/lv2plug.in/ns/ext/morph/morph.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>

static const ret_t *
_test_class(app_t *app)
{
	static const ret_t ret_class_not_valid = {
		LINT_FAIL, "lv2:Port class <%s> not valid", LV2_CORE__Port, NULL};

	const ret_t *ret = NULL;

	LilvNodes *class = lilv_world_find_nodes(app->world,
		NULL, app->uris.rdfs_subClassOf, app->uris.lv2_Port);
	if(class)
	{
		const LilvNodes *supported= lilv_port_get_classes(app->plugin, app->port);
		if(supported)
		{
			LILV_FOREACH(nodes, itr, supported)
			{
				const LilvNode *node = lilv_nodes_get(supported, itr);

				if(!lilv_nodes_contains(class, node))
				{
					*app->urn = strdup(lilv_node_as_uri(node));
					ret = &ret_class_not_valid;
					break;
				}
			}
		}

		lilv_nodes_free(class);
	}

	return ret;
}

static const ret_t *
_test_properties(app_t *app)
{
	static const ret_t ret_properties_not_valid = {
		LINT_FAIL, "lv2:portProperty <%s> not valid", LV2_CORE__portProperty, NULL};

	const ret_t *ret = NULL;

	LilvNodes *properties = lilv_world_find_nodes(app->world,
		NULL, app->uris.rdf_type, app->uris.lv2_PortProperty);
	if(properties)
	{
		LilvNodes *supported = lilv_port_get_properties(app->plugin, app->port);
		if(supported)
		{
			LILV_FOREACH(nodes, itr, supported)
			{
				const LilvNode *node = lilv_nodes_get(supported, itr);

				if(!lilv_nodes_contains(properties, node))
				{
					*app->urn = strdup(lilv_node_as_uri(node));
					ret = &ret_properties_not_valid;
					break;
				}
			}

			lilv_nodes_free(supported);
		}

		lilv_nodes_free(properties);
	}

	return ret;
}

static inline const ret_t *
_test_num(LilvNode *node, bool is_integer, bool is_toggled,
	float *val)
{
	static const ret_t ret_num_not_found = {
		LINT_WARN, "number not found", LV2_CORE__Port, NULL},
	ret_num_not_an_int = {
		LINT_WARN, "number not an integer", LV2_CORE__default, NULL},
	ret_num_not_a_float = {
		LINT_WARN, "number no a float", LV2_CORE__default, NULL},
	ret_num_not_a_bool = {
		LINT_WARN, "number not a bool", LV2_CORE__default, NULL};

	const ret_t *ret = NULL;

	if(node)
	{
		if(lilv_node_is_int(node))
		{
			*val = lilv_node_as_int(node);
		}
		else if(lilv_node_is_float(node))
		{
			*val = lilv_node_as_float(node);
		}
		else if(lilv_node_is_bool(node))
		{
			*val = lilv_node_as_bool(node) ? 1.f : 0.f;
		}

		if(is_integer)
		{
			if(  lilv_node_is_int(node)
				|| (lilv_node_is_float(node)
					&& (rintf(lilv_node_as_float(node)) == lilv_node_as_float(node))) )
			{
				// OK
			}
			else
			{
				ret = &ret_num_not_an_int;
			}
		}
		else if(is_toggled)
		{
			if(  lilv_node_is_bool(node)
				|| (lilv_node_is_int(node)
					&& ((lilv_node_as_int(node) == 0) || (lilv_node_as_int(node) == 1)))
				|| (lilv_node_is_float(node)
					&& ((lilv_node_as_float(node) == 0.f) || (lilv_node_as_float(node) == 1.f))) )
			{
				// OK
			}
			else
			{
				ret = &ret_num_not_a_bool;
			}
		}
		else if( lilv_node_is_float(node)
					|| lilv_node_is_int(node) )
		{
			// OK
		}
		else
		{
			ret = &ret_num_not_a_float;
		}

		lilv_node_free(node);
	}
	else // !node
	{
		ret = &ret_num_not_found;
	}

	return ret;
}

static const ret_t *
_test_default(app_t *app)
{
	const ret_t *ret = NULL;

	app->dflt.f32 = 0.f; // fall-back

	const bool is_integer = lilv_port_has_property(app->plugin, app->port, app->uris.lv2_integer);
	const bool is_toggled = lilv_port_has_property(app->plugin, app->port, app->uris.lv2_toggled);

	if(  (lilv_port_is_a(app->plugin, app->port, app->uris.lv2_ControlPort)
			|| lilv_port_is_a(app->plugin, app->port, app->uris.lv2_CVPort))
		&& lilv_port_is_a(app->plugin, app->port, app->uris.lv2_InputPort) )
	{
		LilvNode *default_node = lilv_port_get(app->plugin, app->port, app->uris.lv2_default);
		ret = _test_num(default_node, is_integer, is_toggled, &app->dflt.f32);
	}

	return ret;
}

static const ret_t *
_test_minimum(app_t *app)
{
	const ret_t *ret = NULL;

	app->min.f32 = 0.f; // fall-back

	const bool is_integer = lilv_port_has_property(app->plugin, app->port, app->uris.lv2_integer);
	const bool is_toggled = lilv_port_has_property(app->plugin, app->port, app->uris.lv2_toggled);

	if(  (lilv_port_is_a(app->plugin, app->port, app->uris.lv2_ControlPort)
			|| lilv_port_is_a(app->plugin, app->port, app->uris.lv2_CVPort))
		&& lilv_port_is_a(app->plugin, app->port, app->uris.lv2_InputPort)
		&& !is_toggled )
	{
		LilvNode *minimum_node = lilv_port_get(app->plugin, app->port, app->uris.lv2_minimum);
		ret = _test_num(minimum_node, is_integer, is_toggled, &app->min.f32);
	}

	return ret;
}

static const ret_t *
_test_maximum(app_t *app)
{
	const ret_t *ret = NULL;

	app->max.f32 = 1.f; // fall-back

	const bool is_integer = lilv_port_has_property(app->plugin, app->port, app->uris.lv2_integer);
	const bool is_toggled = lilv_port_has_property(app->plugin, app->port, app->uris.lv2_toggled);

	if(  (lilv_port_is_a(app->plugin, app->port, app->uris.lv2_ControlPort)
			|| lilv_port_is_a(app->plugin, app->port, app->uris.lv2_CVPort))
		&& lilv_port_is_a(app->plugin, app->port, app->uris.lv2_InputPort)
		&& !is_toggled )
	{
		LilvNode *maximum_node = lilv_port_get(app->plugin, app->port, app->uris.lv2_maximum);
		ret = _test_num(maximum_node, is_integer, is_toggled, &app->max.f32);
	}

	return ret;
}

static const ret_t *
_test_range(app_t *app)
{
	static const ret_t ret_range = {
		LINT_FAIL, "range invalid (min <= default <= max)", LV2_CORE__Port, NULL};

	const ret_t *ret = NULL;

	if(  lilv_port_is_a(app->plugin, app->port, app->uris.lv2_ControlPort)
			|| lilv_port_is_a(app->plugin, app->port, app->uris.lv2_CVPort) )
	{
		if( !( (app->min.f32 <= app->dflt.f32) && (app->dflt.f32 <= app->max.f32) ) )
		{
			ret = &ret_range;
		}
	}

	return ret;
}

static const ret_t *
_test_event_port(app_t *app)
{
	static const ret_t ret_event_port_deprecated = {
		LINT_FAIL, "lv2:EventPort is deprecated, use atom:AtomPort instead", LV2_EVENT__EventPort, NULL};

	const ret_t *ret = NULL;

	if(lilv_port_is_a(app->plugin, app->port, app->uris.event_EventPort))
	{
		ret = &ret_event_port_deprecated;
	}

	return ret;
}

static const ret_t *
_test_morph_port(app_t *app)
{
	static const ret_t ret_morph_port_not_found = {
		LINT_FAIL, "morph port not found", LV2_MORPH__MorphPort, NULL},
	ret_morph_supported_types_not_found = {
		LINT_FAIL, "supported types for morph port not found", LV2_MORPH__supportsType, NULL},
	ret_morph_supported_types_not_enough = {
		LINT_FAIL, "not enough supported types found", LV2_MORPH__supportsType, NULL},
	ret_morph_default_not_found = {
		LINT_FAIL, "default port type not found", LV2_MORPH__MorphPort, NULL};

	LilvNodes *morph_supported_types = lilv_port_get(app->plugin, app->port,
		app->uris.morph_supportsType);
	const unsigned n_morph_supported_types = morph_supported_types
		? lilv_nodes_size(morph_supported_types)
		: 0;

	const bool is_morph_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.morph_MorphPort);
	const bool is_auto_morph_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.morph_AutoMorphPort);
	const bool is_any_morph_port = is_morph_port || is_auto_morph_port;

	const bool is_control_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.lv2_ControlPort);
	const bool is_audio_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.lv2_AudioPort);
	const bool is_cv_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.lv2_CVPort);
	const bool is_atom_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.atom_AtomPort);
	const bool is_event_port = lilv_port_is_a(app->plugin, app->port,
		app->uris.event_EventPort);

	const ret_t *ret = NULL;

	if(is_any_morph_port || n_morph_supported_types)
	{
		if(!is_any_morph_port)
		{
			ret = &ret_morph_port_not_found;;
		}
		else if(!n_morph_supported_types)
		{
			ret = &ret_morph_supported_types_not_found;
		}
		else if(n_morph_supported_types < 2)
		{
			ret = &ret_morph_supported_types_not_enough;;
		}
		else if(!is_control_port || !is_audio_port || !is_cv_port || !is_atom_port || !is_event_port)
		{
			ret = &ret_morph_default_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_comment(app_t *app)
{
	static const ret_t ret_comment_not_found = {
		LINT_NOTE, "rdfs:comment not found", LILV_NS_RDFS"comment", NULL},
	ret_comment_not_a_string = {
		LINT_FAIL, "rdfs:comment not a string", LILV_NS_RDFS"comment", NULL};

	const ret_t *ret = NULL;

	LilvNode *comment = lilv_port_get(app->plugin, app->port, app->uris.rdfs_comment);
	if(comment)
	{
		if(!lilv_node_is_string(comment))
		{
			ret = &ret_comment_not_a_string;
		}

		lilv_node_free(comment);
	}
	else
	{
		ret = &ret_comment_not_found;
	}

	return ret;
}

static const ret_t *
_test_group(app_t *app)
{
	static const ret_t ret_group_not_found = {
		LINT_NOTE, "pg:group not found", LV2_PORT_GROUPS__group, NULL},
	ret_group_not_a_uri = {
		LINT_FAIL, "pg:group not a URI", LV2_PORT_GROUPS__group, NULL};

	const ret_t *ret = NULL;

	LilvNode *group = lilv_port_get(app->plugin, app->port, app->uris.pg_group);
	if(group)
	{
		if(!lilv_node_is_uri(group))
		{
			ret = &ret_group_not_a_uri;
		}

		lilv_node_free(group);
	}
	else
	{
		ret = &ret_group_not_found;
	}

	return ret;
}

static const ret_t *
_test_unit(app_t *app)
{
	static const ret_t ret_units_unit_not_found = {
		LINT_NOTE, "units:unit not found", LV2_UNITS__unit, NULL},
	ret_units_unit_not_a_uri_or_object = {
		LINT_FAIL, "units_unit not a URI or object", LV2_UNITS__unit, NULL};

	const ret_t *ret = NULL;


	if(  lilv_port_is_a(app->plugin, app->port, app->uris.lv2_ControlPort)
		|| lilv_port_is_a(app->plugin, app->port, app->uris.lv2_CVPort) )
	{
		LilvNode *unit = lilv_port_get(app->plugin, app->port, app->uris.units_unit);
		if(unit)
		{
			if(  !lilv_node_is_uri(unit)
				&& !lilv_world_ask(app->world, unit, app->uris.rdf_type, app->uris.units_Unit) )
			{
				ret = &ret_units_unit_not_a_uri_or_object;
			}

			lilv_node_free(unit);
		}
		else
		{
			ret = &ret_units_unit_not_found;
		}
	}

	return ret;
}

static const test_t tests [] = {
	{"Class",          _test_class},
	{"PortProperties", _test_properties},
	{"Default",        _test_default},
	{"Minimum",        _test_minimum},
	{"Maximum",        _test_maximum},
	{"Range",          _test_range},
	{"Event Port",     _test_event_port},
	{"Morph Port",     _test_morph_port},
	{"Comment",        _test_comment},
	{"Group",          _test_group},
	{"Units",          _test_unit},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_port(app_t *app)
{
	bool flag = true;
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return flag;

	for(unsigned i=0; i<tests_n; i++)
	{
		const test_t *test = &tests[i];
		res_t *res = &rets[i];

		res->urn = NULL;
		app->urn = &res->urn;
		res->ret = test->cb(app);
		if(res->ret && (res->ret->lnt & app->show) )
			msg = true;
	}

	const bool show_passes = LINT_PASS & app->show;

	if(msg || show_passes)
	{
		lv2lint_printf(app, "  %s{%d : %s}%s\n",
			colors[app->atty][ANSI_COLOR_BOLD],
			lilv_port_get_index(app->plugin, app->port),
			lilv_node_as_string(lilv_port_get_symbol(app->plugin, app->port)),
			colors[app->atty][ANSI_COLOR_RESET]);

		for(unsigned i=0; i<tests_n; i++)
		{
			const test_t *test = &tests[i];
			res_t *res = &rets[i];

			lv2lint_report(app, test, res, show_passes, &flag);
		}
	}

	return flag;
}
