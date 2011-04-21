@ stdcall wined3d_mutex_lock()
@ stdcall wined3d_mutex_unlock()

@ cdecl wined3d_check_depth_stencil_match(ptr long long long long long)
@ cdecl wined3d_check_device_format(ptr long long long long long long long)
@ cdecl wined3d_check_device_format_conversion(ptr long long long long)
@ cdecl wined3d_check_device_multisample_type(ptr long long long long long ptr)
@ cdecl wined3d_check_device_type(ptr long long long long long)
@ cdecl wined3d_create(long ptr)
@ cdecl wined3d_decref(ptr)
@ cdecl wined3d_enum_adapter_modes(ptr long long long ptr)
@ cdecl wined3d_get_adapter_count(ptr)
@ cdecl wined3d_get_adapter_display_mode(ptr long ptr)
@ cdecl wined3d_get_adapter_identifier(ptr long long ptr)
@ cdecl wined3d_get_adapter_mode_count(ptr long long)
@ cdecl wined3d_get_adapter_monitor(ptr long)
@ cdecl wined3d_get_device_caps(ptr long long ptr)
@ cdecl wined3d_get_parent(ptr)
@ cdecl wined3d_incref(ptr)
@ cdecl wined3d_register_software_device(ptr ptr)

@ cdecl wined3d_buffer_decref(ptr)
@ cdecl wined3d_buffer_free_private_data(ptr ptr)
@ cdecl wined3d_buffer_get_parent(ptr)
@ cdecl wined3d_buffer_get_priority(ptr)
@ cdecl wined3d_buffer_get_private_data(ptr ptr ptr ptr)
@ cdecl wined3d_buffer_get_resource(ptr)
@ cdecl wined3d_buffer_incref(ptr)
@ cdecl wined3d_buffer_map(ptr long long ptr long)
@ cdecl wined3d_buffer_preload(ptr)
@ cdecl wined3d_buffer_set_priority(ptr long)
@ cdecl wined3d_buffer_set_private_data(ptr ptr ptr long long)
@ cdecl wined3d_buffer_unmap(ptr)

@ cdecl wined3d_clipper_create()
@ cdecl wined3d_clipper_decref(ptr)
@ cdecl wined3d_clipper_get_clip_list(ptr ptr ptr ptr)
@ cdecl wined3d_clipper_get_window(ptr ptr)
@ cdecl wined3d_clipper_incref(ptr)
@ cdecl wined3d_clipper_is_clip_list_changed(ptr ptr)
@ cdecl wined3d_clipper_set_clip_list(ptr ptr long)
@ cdecl wined3d_clipper_set_window(ptr long ptr)

@ cdecl wined3d_device_create(ptr long long ptr long ptr ptr)

@ cdecl wined3d_palette_decref(ptr)
@ cdecl wined3d_palette_get_entries(ptr long long long ptr)
@ cdecl wined3d_palette_get_flags(ptr)
@ cdecl wined3d_palette_get_parent(ptr)
@ cdecl wined3d_palette_incref(ptr)
@ cdecl wined3d_palette_set_entries(ptr long long long ptr)

@ cdecl wined3d_query_decref(ptr)
@ cdecl wined3d_query_get_data(ptr ptr long long)
@ cdecl wined3d_query_get_data_size(ptr)
@ cdecl wined3d_query_get_type(ptr)
@ cdecl wined3d_query_incref(ptr)
@ cdecl wined3d_query_issue(ptr long)

@ cdecl wined3d_resource_get_desc(ptr ptr)
@ cdecl wined3d_resource_get_parent(ptr)

@ cdecl wined3d_rendertarget_view_decref(ptr)
@ cdecl wined3d_rendertarget_view_get_parent(ptr)
@ cdecl wined3d_rendertarget_view_get_resource(ptr)
@ cdecl wined3d_rendertarget_view_incref(ptr)

@ cdecl wined3d_shader_decref(ptr)
@ cdecl wined3d_shader_get_byte_code(ptr ptr ptr)
@ cdecl wined3d_shader_get_parent(ptr)
@ cdecl wined3d_shader_incref(ptr)
@ cdecl wined3d_shader_set_local_constants_float(ptr long ptr long)

@ cdecl wined3d_stateblock_apply(ptr)
@ cdecl wined3d_stateblock_capture(ptr)
@ cdecl wined3d_stateblock_decref(ptr)
@ cdecl wined3d_stateblock_incref(ptr)

@ cdecl wined3d_swapchain_decref(ptr)
@ cdecl wined3d_swapchain_get_back_buffer(ptr long long ptr)
@ cdecl wined3d_swapchain_get_device(ptr)
@ cdecl wined3d_swapchain_get_display_mode(ptr ptr)
@ cdecl wined3d_swapchain_get_front_buffer_data(ptr ptr)
@ cdecl wined3d_swapchain_get_gamma_ramp(ptr ptr)
@ cdecl wined3d_swapchain_get_parent(ptr)
@ cdecl wined3d_swapchain_get_present_parameters(ptr ptr)
@ cdecl wined3d_swapchain_get_raster_status(ptr ptr)
@ cdecl wined3d_swapchain_incref(ptr)
@ cdecl wined3d_swapchain_present(ptr ptr ptr ptr ptr long)
@ cdecl wined3d_swapchain_set_gamma_ramp(ptr long ptr)
@ cdecl wined3d_swapchain_set_window(ptr ptr)

@ cdecl wined3d_texture_add_dirty_region(ptr long ptr)
@ cdecl wined3d_texture_decref(ptr)
@ cdecl wined3d_texture_free_private_data(ptr ptr)
@ cdecl wined3d_texture_generate_mipmaps(ptr)
@ cdecl wined3d_texture_get_autogen_filter_type(ptr)
@ cdecl wined3d_texture_get_level_count(ptr)
@ cdecl wined3d_texture_get_lod(ptr)
@ cdecl wined3d_texture_get_parent(ptr)
@ cdecl wined3d_texture_get_priority(ptr)
@ cdecl wined3d_texture_get_private_data(ptr ptr ptr ptr)
@ cdecl wined3d_texture_get_sub_resource(ptr long)
@ cdecl wined3d_texture_get_type(ptr)
@ cdecl wined3d_texture_incref(ptr)
@ cdecl wined3d_texture_preload(ptr)
@ cdecl wined3d_texture_set_autogen_filter_type(ptr long)
@ cdecl wined3d_texture_set_lod(ptr long)
@ cdecl wined3d_texture_set_priority(ptr long)
@ cdecl wined3d_texture_set_private_data(ptr ptr ptr long long)

@ cdecl wined3d_vertex_declaration_decref(ptr)
@ cdecl wined3d_vertex_declaration_get_parent(ptr)
@ cdecl wined3d_vertex_declaration_incref(ptr)

@ cdecl wined3d_volume_decref(ptr)
@ cdecl wined3d_volume_free_private_data(ptr ptr)
@ cdecl wined3d_volume_from_resource(ptr)
@ cdecl wined3d_volume_get_parent(ptr)
@ cdecl wined3d_volume_get_priority(ptr)
@ cdecl wined3d_volume_get_private_data(ptr ptr ptr ptr)
@ cdecl wined3d_volume_get_resource(ptr)
@ cdecl wined3d_volume_incref(ptr)
@ cdecl wined3d_volume_map(ptr ptr ptr long)
@ cdecl wined3d_volume_preload(ptr)
@ cdecl wined3d_volume_set_priority(ptr long)
@ cdecl wined3d_volume_set_private_data(ptr ptr ptr long long)
@ cdecl wined3d_volume_unmap(ptr)
