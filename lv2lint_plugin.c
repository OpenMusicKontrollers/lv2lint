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

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

static const ret_t *
_test_instantiation(app_t *app)
{
	static const ret_t ret_instantiation = {
		LINT_FAIL, "failed to instantiate", LV2_CORE_URI, NULL};

	const ret_t *ret = NULL;

	if(!app->instance)
	{
		ret = &ret_instantiation;
	}

	return ret;
}

#ifdef ENABLE_ELF_TESTS
static const ret_t *
_test_symbols(app_t *app)
{
	static const ret_t ret_symbols = {
		LINT_FAIL, "binary exports superfluous globally visible symbols", LV2_CORE__binary, NULL};

	const ret_t *ret = NULL;

	const LilvNode* node = lilv_plugin_get_library_uri(app->plugin);
	if(node && lilv_node_is_uri(node))
	{
		const char *uri = lilv_node_as_uri(node);
		if(uri)
		{
			char *path = lilv_file_uri_parse(uri, NULL);
			if(path)
			{
				if(!test_visibility(path, "lv2_descriptor"))
				{
					ret = &ret_symbols;
				}

				lilv_free(path);
			}
		}
	}

	return ret;
}

static const ret_t *
_test_linking(app_t *app)
{
	static const ret_t ret_symbols = {
		LINT_WARN, "binary links to non-whitelisted shared library", LV2_CORE__binary,
		"The ideal plugin dynamically links maximally to libc and libm."},
	ret_libstdcpp = {
		LINT_WARN, "binary links to libstdc++", LV2_CORE__binary,
		"C++ ABI incompatibilities between host and plugin are to be expected."};

	const ret_t *ret = NULL;

	static const char *whitelist [] = {
		"libc",
		"libm",
		"libstdc++",
		"libgcc_s"
	};
	const unsigned n_whitelist = sizeof(whitelist) / sizeof(const char *);

	static const char *graylist [] = {
		"libstdc++",
		"libgcc_s"
	};
	const unsigned n_graylist = sizeof(graylist) / sizeof(const char *);

	const LilvNode* node = lilv_plugin_get_library_uri(app->plugin);
	if(node && lilv_node_is_uri(node))
	{
		const char *uri = lilv_node_as_uri(node);
		if(uri)
		{
			char *path = lilv_file_uri_parse(uri, NULL);
			if(path)
			{
				if(!test_shared_libraries(path, whitelist, n_whitelist, NULL, 0))
				{
					ret = &ret_symbols;
				}
				else if(!test_shared_libraries(path, NULL, 0, graylist, n_graylist))
				{
					ret = &ret_libstdcpp;
				}

				lilv_free(path);
			}
		}
	}

	return ret;
}
#endif

static const ret_t *
_test_verification(app_t *app)
{
	static const ret_t ret_verification = {
		LINT_FAIL, "failed", LV2_CORE_URI, NULL};

	const ret_t *ret = NULL;

	if(!lilv_plugin_verify(app->plugin))
	{
		ret = &ret_verification;
	}

	return ret;
}

static const ret_t *
_test_name(app_t *app)
{
	static const ret_t ret_name_not_found = {
		LINT_FAIL, "doap:name not found", LV2_CORE__Plugin, NULL},
	ret_name_not_a_string = {
		LINT_FAIL, "doap:name not a string", LILV_NS_DOAP"name", NULL},
	ret_name_empty = {
		LINT_FAIL, "doap:name empty", LILV_NS_DOAP"name", NULL};

	const ret_t *ret = NULL;

	LilvNode *name_node = lilv_plugin_get_name(app->plugin);
	if(name_node)
	{
		if(lilv_node_is_string(name_node))
		{
			const char *name = lilv_node_as_string(name_node);
			if(!name)
			{
				ret = &ret_name_empty;
			}
		}
		else // !is_string
		{
			ret = &ret_name_not_a_string;
		}
		lilv_node_free(name_node);
	}
	else // !name_node
	{
		ret = &ret_name_not_found;
	}

	return ret;
}

static const ret_t * 
_test_license(app_t *app)
{
	static const ret_t ret_license_not_found = {
		LINT_WARN, "doap:license not found", LV2_CORE__Plugin, NULL},
	ret_license_not_a_uri = {
		LINT_FAIL, "doap:license not a URI", LILV_NS_DOAP"license", NULL},
#ifdef ENABLE_ONLINE_TESTS
	ret_license_not_existing = {
		LINT_WARN, "doap:license Web URL does not exist", LILV_NS_DOAP"license", NULL},
#endif
	ret_license_empty = {
		LINT_FAIL, "doap:license empty", LILV_NS_DOAP"license", NULL};

	const ret_t *ret = NULL;

	LilvNode *license_node = lilv_world_get(app->world, lilv_plugin_get_uri(app->plugin), app->uris.doap_license, NULL);
	if(license_node)
	{
		if(lilv_node_is_uri(license_node))
		{
			const char *uri = lilv_node_as_uri(license_node);

			if(!uri)
			{
				ret = &ret_license_empty;
			}
#ifdef ENABLE_ONLINE_TESTS
			else if(is_url(uri))
			{
				const bool url_exists = !app->online || test_url(app, uri);

				if(!url_exists)
				{
					ret = &ret_license_not_existing;
				}
			}
#endif
		}
		else
		{
			ret = &ret_license_not_a_uri;
		}
		lilv_node_free(license_node);
	}
	else
	{
		ret = &ret_license_not_found;
	}

	return ret;
}

static const ret_t * 
_test_author_name(app_t *app)
{
	static const ret_t ret_author_not_found = {
		LINT_WARN, "foaf:name not found", LV2_CORE__project, NULL},
	ret_author_not_a_string = {
		LINT_FAIL, "foaf:name not an string", LILV_NS_FOAF"name", NULL},
	ret_author_empty = {
		LINT_FAIL, "foaf:name empty", LILV_NS_FOAF"name", NULL};

	const ret_t *ret = NULL;

	LilvNode *author_name = lilv_plugin_get_author_name(app->plugin);
	if(author_name)
	{
		if(lilv_node_is_string(author_name))
		{
			if(!lilv_node_as_string(author_name))
			{
				ret = &ret_author_empty;
			}
		}
		else
		{
			ret = &ret_author_not_a_string;
		}
		lilv_node_free(author_name);
	}
	else
	{
		ret = &ret_author_not_found;
	}

	return ret;
}

static const ret_t *
_test_author_email(app_t *app)
{
	static const ret_t ret_email_not_found = {
		LINT_WARN, "foaf:email not found", LV2_CORE__project, NULL},
	ret_email_not_a_uri = {
		LINT_FAIL, "foaf:email not an URI", LILV_NS_FOAF"email", NULL},
	ret_email_empty = {
		LINT_FAIL, "foaf:email empty", LILV_NS_FOAF"email", NULL};

	const ret_t *ret = NULL;

	LilvNode *author_email = lilv_plugin_get_author_email(app->plugin);
	if(author_email)
	{
		if(lilv_node_is_uri(author_email))
		{
			if(!lilv_node_as_uri(author_email))
			{
				ret = &ret_email_empty;
			}
		}
		else
		{
			ret = &ret_email_not_a_uri;
		}
		lilv_node_free(author_email);
	}
	else
	{
		ret = &ret_email_not_found;
	}

	return ret;
}

static const ret_t *
_test_author_homepage(app_t *app)
{
	static const ret_t ret_homepage_not_found = {
		LINT_WARN, "foaf:homepage not found", LV2_CORE__project, NULL},
	ret_homepage_not_a_uri = {
		LINT_FAIL, "foaf:homepage not an URI", LILV_NS_FOAF"homepage", NULL},
#ifdef ENABLE_ONLINE_TESTS
	ret_homepage_not_existing = {
		LINT_WARN, "foaf:homepage Web URL does not exist", LILV_NS_FOAF"homepage", NULL},
#endif
	ret_homepage_empty = {
		LINT_FAIL, "foaf:homepage empty", LILV_NS_FOAF"homepage", NULL};

	const ret_t *ret = NULL;

	LilvNode *author_homepage = lilv_plugin_get_author_homepage(app->plugin);
	if(author_homepage)
	{
		if(lilv_node_is_uri(author_homepage))
		{
			const char *uri = lilv_node_as_uri(author_homepage);

			if(!uri)
			{
				ret = &ret_homepage_empty;
			}
#ifdef ENABLE_ONLINE_TESTS
			else if(is_url(uri))
			{
				const bool url_exists = !app->online || test_url(app, uri);

				if(!url_exists)
				{
					ret = &ret_homepage_not_existing;
				}
			}
#endif
		}
		else
		{
			ret = &ret_homepage_not_a_uri;
		}
		lilv_node_free(author_homepage);
	}
	else
	{
		ret = &ret_homepage_not_found;
	}

	return ret;
}

static const ret_t * 
_test_version_minor(app_t *app)
{
	static const ret_t ret_version_minor_not_found = {
		LINT_FAIL, "lv2:minorVersion not found", LV2_CORE__minorVersion, NULL},
	ret_version_minor_not_an_int = {
		LINT_FAIL, "lv2:minorVersion not an integer", LV2_CORE__minorVersion, NULL},
	ret_version_minor_unstable = {
		LINT_NOTE, "lv2:minorVersion denotes an unstable version", LV2_CORE__minorVersion, NULL};

	const ret_t *ret = NULL;

	LilvNodes *minor_version_nodes = lilv_plugin_get_value(app->plugin , app->uris.lv2_minorVersion);
	if(minor_version_nodes)
	{
		const LilvNode *minor_version_node = lilv_nodes_get_first(minor_version_nodes);
		if(minor_version_node)
		{
			if(lilv_node_is_int(minor_version_node))
			{
				const int minor_version = lilv_node_as_int(minor_version_node);
				if( (minor_version % 2 != 0) || (minor_version == 0) )
				{
					ret = &ret_version_minor_unstable;
				}
			}
			else
			{
				ret = &ret_version_minor_not_an_int;
			}
		}
		else
		{
			ret = &ret_version_minor_not_found;
		}
		lilv_nodes_free(minor_version_nodes);
	}
	else
	{
		ret = &ret_version_minor_not_found;
	}

	return ret;
}

static const ret_t * 
_test_version_micro(app_t *app)
{
	static const ret_t ret_version_micro_not_found = {
		LINT_FAIL, "lv2:microVersion not found", LV2_CORE__microVersion, NULL},
	ret_version_micro_not_an_int = {
		LINT_FAIL, "lv2:microVersion not an integer", LV2_CORE__microVersion, NULL},
	ret_version_micro_unstable = {
		LINT_NOTE, "lv2:microVersion denotes an unstable version", LV2_CORE__microVersion, NULL};

	const ret_t *ret = NULL;

	LilvNodes *micro_version_nodes = lilv_plugin_get_value(app->plugin , app->uris.lv2_microVersion);
	if(micro_version_nodes)
	{
		const LilvNode *micro_version_node = lilv_nodes_get_first(micro_version_nodes);
		if(micro_version_node)
		{
			if(lilv_node_is_int(micro_version_node))
			{
				const int micro_version = lilv_node_as_int(micro_version_node);
				if(micro_version % 2 != 0)
				{
					ret = &ret_version_micro_unstable;
				}
			}
			else
			{
				ret = &ret_version_micro_not_an_int;
			}
		}
		else
		{
			ret = &ret_version_micro_not_found;
		}
		lilv_nodes_free(micro_version_nodes);
	}
	else
	{
		ret = &ret_version_micro_not_found;
	}

	return ret;
}

#if 0 //FIXME
static const ret_t * 
_test_extension_data(app_t *app)
{
	const ret_t *ret = NULL;

	LilvNode *extension_data_nodes = lilv_plugin_get_extension_data(app->plugin);
	if(extension_data_nodes)
	{
		LILV_FOREACH(nodes, itr, extension_data_nodes)
		{
			const LilvNode *extension_data_node = lilv_nodes_get(extension_data_nodes, itr);
			if(extension_data_node)
			{
				// check with lilv_instanace_get_extension_data
			}
		}
		//FIXME flag = true; //FIXME

		lilv_nodes_free(extension_data_nodes);
	}

	return ret;
}
#endif

static const ret_t *
_test_project(app_t *app)
{
	static const ret_t ret_project_not_found = {
		LINT_NOTE, "lv2:project not found", LV2_CORE__project, NULL},
	ret_project_name_not_found = {
		LINT_WARN, "lv2:project doap:name not found", LV2_CORE__project, NULL},
	ret_project_name_not_a_string = {
		LINT_FAIL, "lv2:project doap:name not a string", LILV_NS_DOAP"name", NULL},
	ret_project_name_empty = {
		LINT_FAIL, "lv2:project doap:name empty", LILV_NS_DOAP"name", NULL};

	const ret_t *ret = NULL;

	LilvNode *project_node = lilv_plugin_get_project(app->plugin);
	if(project_node)
	{
		LilvNode *project_name_node = lilv_world_get(app->world, project_node, app->uris.doap_name, NULL);
		if(project_name_node)
		{
			if(lilv_node_is_string(project_name_node))
			{
				if(!lilv_node_as_string(project_name_node))
				{
					ret = &ret_project_name_empty;
				}
			}
			else
			{
				ret = &ret_project_name_not_a_string;
			}
			lilv_free(project_name_node);
		}
		else // !doap_name_node
		{
			ret = &ret_project_name_not_found;
		}
		lilv_node_free(project_node);
	}
	else // !project_node
	{
		ret = &ret_project_not_found;
	}

	return ret;
}

static inline bool
_test_class_equals(const LilvPluginClass *base, const LilvPluginClass *class)
{
	return lilv_node_equals(
		lilv_plugin_class_get_uri(base),
		lilv_plugin_class_get_uri(class) );
}

static inline bool
_test_class_match(const LilvPluginClass *base, const LilvPluginClass *class)
{
	if(_test_class_equals(base, class))
		return true;

	bool ret = false;
	LilvPluginClasses *children= lilv_plugin_class_get_children(base);
	if(children)
	{
		LILV_FOREACH(plugin_classes, itr, children)
		{
			const LilvPluginClass *child = lilv_plugin_classes_get(children, itr);
			if(_test_class_match(child, class))
			{
				ret = true;
				break;
			}
		}
		lilv_plugin_classes_free(children);
	}

	return ret;
}

static const ret_t *
_test_class(app_t *app)
{
	static const ret_t ret_class_not_found = {
		LINT_FAIL, "lv2:class not found", LV2_CORE__Plugin, NULL},
	ret_class_is_base_class = {
		LINT_WARN, "lv2:class is base class", LV2_CORE__Plugin, NULL},
	ret_class_not_valid = {
		LINT_FAIL, "lv2:class <%s> not valid", LV2_CORE__Plugin, NULL};

	const ret_t *ret = NULL;

	const LilvPluginClass *class = lilv_plugin_get_class(app->plugin);
	if(class)
	{
		const LilvPluginClass *base = lilv_world_get_plugin_class(app->world);
		if(_test_class_equals(base, class))
		{
			ret = &ret_class_is_base_class;
		}
		else if(!_test_class_match(base, class))
		{
			*app->urn = strdup(lilv_node_as_uri(lilv_plugin_class_get_uri(class)));
			ret = &ret_class_not_valid;
		}
	}
	else // !class
	{
		ret = &ret_class_not_found;
	}

	return ret;
}

static const ret_t *
_test_features(app_t *app)
{
	static const ret_t ret_features_not_valid = {
		LINT_FAIL, "lv2:[optional|required]Feature <%s> not valid", LV2_CORE__Feature, NULL};

	const ret_t *ret = NULL;

	LilvNodes *features = lilv_world_find_nodes(app->world,
		NULL, app->uris.rdf_type, app->uris.lv2_Feature);
	if(features)
	{
		LilvNodes *supported = lilv_plugin_get_supported_features(app->plugin);
		if(supported)
		{
			LILV_FOREACH(nodes, itr, supported)
			{
				const LilvNode *node = lilv_nodes_get(supported, itr);

				if(!lilv_nodes_contains(features, node))
				{
					*app->urn = strdup(lilv_node_as_uri(node));
					ret = &ret_features_not_valid;
					break;
				}
			}

			lilv_nodes_free(supported);
		}

		lilv_nodes_free(features);
	}

	return ret;
}

static const ret_t *
_test_extensions(app_t *app)
{
	static const ret_t ret_extensions_not_valid = {
		LINT_FAIL, "lv2:extensionData <%s> not valid", LV2_CORE__ExtensionData, NULL};

	const ret_t *ret = NULL;

	LilvNodes *extensions = lilv_world_find_nodes(app->world,
		NULL, app->uris.rdf_type, app->uris.lv2_ExtensionData);
	if(extensions)
	{
		LilvNodes *data = lilv_plugin_get_extension_data(app->plugin);
		if(data)
		{
			LILV_FOREACH(nodes, itr, data)
			{
				const LilvNode *node = lilv_nodes_get(data, itr);

				if(!lilv_nodes_contains(extensions, node))
				{
					*app->urn = strdup(lilv_node_as_uri(node));
					ret = &ret_extensions_not_valid;
					break;
				}
			}

			lilv_nodes_free(data);
		}

		lilv_nodes_free(extensions);
	}

	return ret;
}

static const ret_t *
_test_worker(app_t *app)
{
	static const ret_t ret_worker_schedule_not_found = {
		LINT_FAIL, "work:schedule not defined", LV2_WORKER__schedule, NULL},
	ret_worker_interface_not_found = {
		LINT_FAIL, "work:interface not defined", LV2_WORKER__interface, NULL},
	ret_worker_interface_not_returned = {
		LINT_FAIL, "work:interface not returned by 'extention_data'", LV2_WORKER__interface, NULL},
	ret_worker_work_not_found = {
		LINT_FAIL, "work:interface has no 'work' function", LV2_WORKER__interface, NULL},
	ret_worker_work_response_not_found = {
		LINT_FAIL, "work:interface has no 'work_response' function", LV2_WORKER__interface, NULL},
	ret_worker_end_run_not_found = {
		LINT_NOTE, "work:interface has no 'end_run' function", LV2_WORKER__interface, NULL};

	const ret_t *ret = NULL;

	const bool has_work_schedule= lilv_plugin_has_feature(app->plugin, app->uris.work_schedule);
	const bool has_work_iface = lilv_plugin_has_extension_data(app->plugin, app->uris.work_interface);

	if(has_work_schedule || has_work_iface || app->work_iface)
	{
		if(!app->work_iface)
		{
			ret = &ret_worker_interface_not_returned;
		}
		else if(!app->work_iface->work)
		{
			ret = &ret_worker_work_not_found;
		}
		else if(!app->work_iface->work_response)
		{
			ret = &ret_worker_work_response_not_found;
		}
		else if(!app->work_iface->end_run)
		{
			ret = &ret_worker_end_run_not_found;
		}
		else if(!has_work_schedule)
		{
			ret = &ret_worker_schedule_not_found;
		}
		else if(!has_work_iface)
		{
			ret = &ret_worker_interface_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_options_iface(app_t *app)
{
	static const ret_t ret_options_interface_not_found = {
		LINT_FAIL, "opts:interface not defined", LV2_OPTIONS__interface, NULL},
	ret_options_interface_not_returned = {
		LINT_FAIL, "opts:interface not returned by 'extention_data'", LV2_OPTIONS__interface, NULL},
	ret_options_get_not_found = {
		LINT_FAIL, "opts:interface has no 'get' function", LV2_OPTIONS__interface, NULL},
	ret_options_set_not_found = {
		LINT_FAIL, "opts:interface has no 'set' function", LV2_OPTIONS__interface, NULL};

	const ret_t *ret = NULL;

	const bool has_opts_iface = lilv_plugin_has_extension_data(app->plugin, app->uris.opts_interface);

	if(has_opts_iface || app->opts_iface)
	{
		if(!app->opts_iface)
		{
			ret = &ret_options_interface_not_returned;
		}
		else if(!app->opts_iface->get)
		{
			ret = &ret_options_get_not_found;
		}
		else if(!app->opts_iface->set)
		{
			ret = &ret_options_set_not_found;
		}
		else if(!has_opts_iface)
		{
			ret = &ret_options_interface_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_options_feature(app_t *app)
{
	static const ret_t ret_options_options_not_found = {
		LINT_FAIL, "opts:options not defined", LV2_OPTIONS__options, NULL},
	ret_options_supported_not_found = {
		LINT_WARN, "opts:{required,supported} options not defined", LV2_OPTIONS__supportedOption, NULL},
	ret_options_required_found = {
		LINT_WARN, "opts:required options defined", LV2_OPTIONS__requiredOption, NULL};

	const ret_t *ret = NULL;

	const bool has_opts_options= lilv_plugin_has_feature(app->plugin, app->uris.opts_options);
	LilvNodes *required_options = lilv_plugin_get_value(app->plugin, app->uris.opts_requiredOption);
	LilvNodes *supported_options = lilv_plugin_get_value(app->plugin, app->uris.opts_supportedOption);

	const unsigned required_n = lilv_nodes_size(required_options);
	const unsigned supported_n = lilv_nodes_size(supported_options);
	const unsigned n = required_n + supported_n;

	if(has_opts_options || n)
	{
		if(!has_opts_options)
		{
			ret = &ret_options_options_not_found;
		}
		else if(!n)
		{
			ret = &ret_options_supported_not_found;
		}
	}
	else if(required_n)
	{
		ret = &ret_options_required_found;
	}

	if(required_options)
	{
		lilv_nodes_free(required_options);
	}

	if(supported_options)
	{
		lilv_nodes_free(supported_options);
	}

	return ret;
}

static const ret_t *
_test_uri_map(app_t *app)
{
	static const ret_t ret_uri_map_deprecated = {
		LINT_FAIL, "uri-map is deprecated, use urid:map instead", LV2_URI_MAP_URI, NULL};

	const ret_t *ret = NULL;

	if(lilv_plugin_has_feature(app->plugin, app->uris.uri_map))
	{
		ret = &ret_uri_map_deprecated;
	}

	return ret;
}

static const ret_t *
_test_state(app_t *app)
{
	static const ret_t ret_state_load_default_not_found = {
		LINT_FAIL, "state:loadDefaultState not defined", LV2_STATE__loadDefaultState, NULL},
	ret_state_interface_not_found = {
		LINT_FAIL, "state:interface not defined", LV2_STATE__interface, NULL},
	ret_state_state_not_found = {
		LINT_WARN, "state:state not defined", LV2_STATE__state, NULL},
	ret_state_interface_not_returned = {
		LINT_FAIL, "state:interface not returned by 'extension_data'", LV2_STATE__interface, NULL},
	ret_state_save_not_found = {
		LINT_FAIL, "state:interface has no 'save' function", LV2_STATE__interface, NULL},
	ret_state_restore_not_found = {
		LINT_FAIL, "state:interface has no 'restore' function", LV2_STATE__interface, NULL};

	const ret_t *ret = NULL;

	const bool has_load_default = lilv_plugin_has_feature(app->plugin, app->uris.state_loadDefaultState);
	const bool has_thread_safe_restore = lilv_plugin_has_feature(app->plugin, app->uris.state_threadSafeRestore);
	const bool has_state = lilv_world_ask(app->world,
		lilv_plugin_get_uri(app->plugin), app->uris.state_state, NULL);
	const bool has_iface = lilv_plugin_has_extension_data(app->plugin, app->uris.state_interface);

	if(has_load_default || has_thread_safe_restore || has_state || has_iface || app->state_iface)
	{
		if(!app->state_iface)
		{
			ret = &ret_state_interface_not_returned;
		}
		else if(!app->state_iface->save)
		{
			ret = &ret_state_save_not_found;
		}
		else if(!app->state_iface->restore)
		{
			ret = &ret_state_restore_not_found;
		}
		else if(!has_iface)
		{
			ret = &ret_state_interface_not_found;
		}
		else if(has_load_default || has_state)
		{
			if(!has_load_default)
			{
				ret = &ret_state_load_default_not_found;
			}
			else if(!has_state)
			{
				ret = &ret_state_state_not_found;
			}
		}
	}

	return ret;
}

static const ret_t *
_test_comment(app_t *app)
{
	static const ret_t ret_comment_not_found = {
		LINT_NOTE, "rdfs:comment or doap:description not found", LV2_CORE__Plugin, NULL},
	ret_comment_not_a_string = {
		LINT_FAIL, "rdfs:comment not a string", LILV_NS_RDFS"comment", NULL},
	ret_description_not_a_string = {
		LINT_FAIL, "doap:description not a string", LILV_NS_DOAP"description", NULL};

	const ret_t *ret = NULL;

	LilvNode *comment = lilv_world_get(app->world,
		lilv_plugin_get_uri(app->plugin), app->uris.rdfs_comment, NULL);
	LilvNode *description= lilv_world_get(app->world,
		lilv_plugin_get_uri(app->plugin), app->uris.doap_description, NULL);

	if(comment)
	{
		if(!lilv_node_is_string(comment))
		{
			ret = &ret_comment_not_a_string;
		}

		lilv_node_free(comment);
		if(description)
			lilv_node_free(description);
	}
	else if(description)
	{
		if(!lilv_node_is_string(description))
		{
			ret = &ret_description_not_a_string;
		}

		lilv_node_free(description);
	}
	else
	{
		ret = &ret_comment_not_found;
	}

	return ret;
}

static const ret_t *
_test_shortdesc(app_t *app)
{
	static const ret_t ret_shortdesc_not_found = {
		LINT_NOTE, "doap:shortdesc not found", LILV_NS_DOAP"shortdesc", NULL},
	ret_shortdesc_not_a_string = {
		LINT_FAIL, "doap:shortdesc not a string", LILV_NS_DOAP"shortdesc", NULL};

	const ret_t *ret = NULL;

	LilvNode *shortdesc = lilv_world_get(app->world,
		lilv_plugin_get_uri(app->plugin), app->uris.doap_shortdesc, NULL);
	if(shortdesc)
	{
		if(!lilv_node_is_string(shortdesc))
		{
			ret = &ret_shortdesc_not_a_string;
		}

		lilv_node_free(shortdesc);
	}
	else
	{
		ret = &ret_shortdesc_not_found;
	}

	return ret;
}

static const ret_t *
_test_idisp(app_t *app)
{
	static const ret_t ret_idisp_queue_draw_not_found = {
		LINT_FAIL, "idisp:queue_draw not defined", LV2_INLINEDISPLAY__queue_draw, NULL},
	ret_idisp_interface_not_found = {
		LINT_FAIL, "idisp:interface not defined", LV2_INLINEDISPLAY__interface, NULL},
	ret_idisp_interface_not_returned = {
		LINT_FAIL, "idisp:interface not returned by 'extention_data'", LV2_INLINEDISPLAY__interface, NULL},
	ret_idisp_render_not_found = {
		LINT_FAIL, "idisp:interface has no 'render' function", LV2_INLINEDISPLAY__interface, NULL};

	const ret_t *ret = NULL;

	const bool has_idisp_queue_draw = lilv_plugin_has_feature(app->plugin, app->uris.idisp_queue_draw);
	const bool has_idisp_iface = lilv_plugin_has_extension_data(app->plugin, app->uris.idisp_interface);

	if(has_idisp_queue_draw || has_idisp_iface || app->idisp_iface)
	{
		if(!app->idisp_iface)
		{
			ret = &ret_idisp_interface_not_returned;
		}
		else if(!app->idisp_iface->render)
		{
			ret = &ret_idisp_render_not_found;
		}
		else if(!has_idisp_queue_draw)
		{
			ret = &ret_idisp_queue_draw_not_found;
		}
		else if(!has_idisp_iface)
		{
			ret = &ret_idisp_interface_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_hard_rt_capable(app_t *app)
{
	static const ret_t ret_hard_rt_capable_not_found = {
		LINT_WARN, "not advertized as real-time safe", LV2_CORE__hardRTCapable, NULL};

	const ret_t *ret = NULL;

	const bool is_hard_rt_capable = lilv_plugin_has_feature(app->plugin, app->uris.lv2_hardRTCapable);

	if(!is_hard_rt_capable)
	{
		ret = &ret_hard_rt_capable_not_found;
	}

	return ret;
}

static const ret_t *
_test_in_place_broken(app_t *app)
{
	static const ret_t ret_in_place_broken_found = {
		LINT_WARN, "cannot process audio/CV in-place", LV2_CORE__inPlaceBroken, NULL};

	const ret_t *ret = NULL;

	const bool is_in_place_broken = lilv_plugin_has_feature(app->plugin, app->uris.lv2_inPlaceBroken);

	if(is_in_place_broken)
	{
		ret = &ret_in_place_broken_found;
	}

	return ret;
}

static const ret_t *
_test_is_live(app_t *app)
{
	static const ret_t ret_is_live_not_found = {
		LINT_NOTE, "not meant for live usage", LV2_CORE__isLive, NULL};

	const ret_t *ret = NULL;

	const bool is_live = lilv_plugin_has_feature(app->plugin, app->uris.lv2_isLive);

	if(!is_live)
	{
		ret = &ret_is_live_not_found;
	}

	return ret;
}

static const ret_t *
_test_fixed_block_length(app_t *app)
{
	static const ret_t ret_fixed_block_length_found = {
		LINT_WARN, "requiring a fixed block length is highly discouraged", LV2_BUF_SIZE__fixedBlockLength, NULL};

	const ret_t *ret = NULL;

	const bool wants_fixed_block_length =
		lilv_plugin_has_feature(app->plugin, app->uris.bufsz_fixedBlockLength);

	if(wants_fixed_block_length)
	{
		ret = &ret_fixed_block_length_found;
	}

	return ret;
}

static const ret_t *
_test_power_of_2_block_length(app_t *app)
{
	static const ret_t ret_power_of_2_block_length_found = {
		LINT_WARN, "requiring a power of 2 block length is highly discouraged", LV2_BUF_SIZE__powerOf2BlockLength, NULL};

	const ret_t *ret = NULL;

	const bool wants_power_of_2_block_length =
		lilv_plugin_has_feature(app->plugin, app->uris.bufsz_powerOf2BlockLength);

	if(wants_power_of_2_block_length)
	{
		ret = &ret_power_of_2_block_length_found;
	}

	return ret;
}

#ifdef ENABLE_ONLINE_TESTS
static const ret_t *
_test_plugin_url(app_t *app)
{
	static const ret_t ret_plugin_url_not_existing = {
		LINT_WARN, "Plugin Web URL does not exist", LV2_CORE__Plugin, NULL};

	const ret_t *ret = NULL;

	const char *uri = lilv_node_as_uri(lilv_plugin_get_uri(app->plugin));

	if(is_url(uri))
	{
		const bool url_exists = !app->online || test_url(app, uri);

		if(!url_exists)
		{
			ret = &ret_plugin_url_not_existing;
		}
	}

	return ret;
}
#endif

static const ret_t *
_test_patch(app_t *app)
{
	static const ret_t ret_patch_no_patch_message_support_on_output = {
		LINT_FAIL, "no patch:Message support on any output", LV2_PATCH__Message, NULL},
	ret_patch_no_patch_message_support_on_input = {
		LINT_FAIL, "no patch:Message support on any input", LV2_PATCH__Message, NULL},
	ret_patch_no_parameters_found = {
		LINT_WARN, "no patch:writable/readable parameters found", LV2_PATCH__writable, NULL};

	const ret_t *ret = NULL;

	const unsigned n_writables = app->writables
		? lilv_nodes_size(app->writables) : 0;
	const unsigned n_readables = app->readables
		? lilv_nodes_size(app->readables) : 0;
	const unsigned n_parameters = n_writables + n_readables;

	unsigned n_patch_message_input = 0;
	unsigned n_patch_message_output = 0;

	const uint32_t num_ports = lilv_plugin_get_num_ports(app->plugin);
	for(unsigned i=0; i<num_ports; i++)
	{
		const LilvPort *port = lilv_plugin_get_port_by_index(app->plugin, i);

		if(  lilv_port_is_a(app->plugin, port, app->uris.atom_AtomPort)
			&& lilv_port_supports_event(app->plugin, port, app->uris.patch_Message) )
		{
			if(lilv_port_is_a(app->plugin, port, app->uris.lv2_InputPort))
			{
				n_patch_message_input += 1;
			}
			else if(lilv_port_is_a(app->plugin, port, app->uris.lv2_OutputPort))
			{
				n_patch_message_output += 1;
			}
		}
	}

	if(n_parameters + n_patch_message_input + n_patch_message_output)
	{
		if(n_parameters == 0)
		{
			ret = &ret_patch_no_parameters_found;
		}
		else if(n_patch_message_input == 0)
		{
			ret = &ret_patch_no_patch_message_support_on_input;
		}
		else if(n_patch_message_output == 0)
		{
			ret = &ret_patch_no_patch_message_support_on_output;
		}
	}

	return ret;
}

static const test_t tests [] = {
	{"Instantiation",   _test_instantiation},
#ifdef ENABLE_ELF_TESTS
	{"Symbols",         _test_symbols},
	{"Linking",         _test_linking},
#endif
	{"Verification",    _test_verification},
	{"Name",            _test_name},
	{"License",         _test_license},
	{"Author Name",     _test_author_name},
	{"Author Email",    _test_author_email},
	{"Author Homepage", _test_author_homepage},
	{"Version Minor",   _test_version_minor},
	{"Version Micro",   _test_version_micro},
	{"Project",         _test_project},
	{"Class",           _test_class},
	{"Features",        _test_features},
	{"Extension Data",  _test_extensions},
	{"Worker",          _test_worker},
	{"Options Iface",   _test_options_iface},
	{"Options Feature", _test_options_feature},
	{"URI-Map",         _test_uri_map},
	{"State",           _test_state},
	{"Comment",         _test_comment},
	{"Shortdesc",       _test_shortdesc},
	{"Inline Display",  _test_idisp},
	{"Hard RT Capable", _test_hard_rt_capable},
	{"In Place Broken", _test_in_place_broken},
	{"Is Live",         _test_is_live},
	//{"Bounded Block",   _test_bounded_block_length}, //TODO check for opts:opt
	{"Fixed Block",     _test_fixed_block_length},
	{"PowerOf2 Block",  _test_power_of_2_block_length},
#ifdef ENABLE_ONLINE_TESTS
	{"Plugin URL",      _test_plugin_url},
#endif
	{"Patch",           _test_patch},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_plugin(app_t *app)
{
	bool flag = true;
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return flag;

	app->writables = lilv_plugin_get_value(app->plugin, app->uris.patch_writable);
	app->readables = lilv_plugin_get_value(app->plugin, app->uris.patch_readable);

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
		for(unsigned i=0; i<tests_n; i++)
		{
			const test_t *test = &tests[i];
			res_t *res = &rets[i];

			lv2lint_report(app, test, res, show_passes, &flag);
		}
	}

	const uint32_t num_ports = lilv_plugin_get_num_ports(app->plugin);
	for(unsigned i=0; i<num_ports; i++)
	{
		bool port_flag = true;

		app->port = lilv_plugin_get_port_by_index(app->plugin, i);
		if(app->port)
		{
			if(!test_port(app))
				port_flag = false;
			app->port = NULL;
		}
		else
			port_flag = false;

		if(flag && !port_flag)
			flag = port_flag;
	}

	if(app->writables)
	{
		LILV_FOREACH(nodes, itr, app->writables)
		{
			bool param_flag = true;

			app->parameter = lilv_nodes_get(app->writables, itr);
			if(app->parameter)
			{
				if(!test_parameter(app))
					param_flag = false;
				app->parameter = NULL;
			}
			else
				param_flag = false;

			if(flag && !param_flag)
				flag = param_flag;
		}

		lilv_nodes_free(app->writables);
		app->writables = NULL;
	}

	if(app->readables)
	{
		LILV_FOREACH(nodes, itr, app->readables)
		{
			bool param_flag = true;

			app->parameter = lilv_nodes_get(app->readables, itr);
			if(app->parameter)
			{
				if(!test_parameter(app))
					param_flag = false;
				app->parameter = NULL;
			}
			else
				param_flag = false;

			if(flag && !param_flag)
				flag = param_flag;
		}

		lilv_nodes_free(app->readables);
		app->readables = NULL;
	}

	LilvUIs *uis = lilv_plugin_get_uis(app->plugin);
	if(uis)
	{
		LILV_FOREACH(uis, itr, uis)
		{
			bool ui_flag = true;

			app->ui = lilv_uis_get(uis, itr);
			if(app->ui)
			{
				const LilvNode *ui_uri = lilv_ui_get_uri(app->ui);
				lilv_world_load_resource(app->world, ui_uri);

				if(!test_ui(app))
					ui_flag = false;
				app->ui = NULL;

				lilv_world_unload_resource(app->world, ui_uri);
			}
			else
				ui_flag = false;

			if(flag && !ui_flag)
				flag = ui_flag;
		}

		lilv_uis_free(uis);
	}

	lv2lint_printf(app, "\n");

	return flag;
}
