/*************************************************************************/
/*  editor_properties.cpp                                                */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "editor_properties.h"

#include "editor/editor_resource_preview.h"
#include "editor/filesystem_dock.h"
#include "editor_node.h"
#include "editor_properties_array_dict.h"
#include "editor_scale.h"
#include "scene/2d/gpu_particles_2d.h"
#include "scene/3d/fog_volume.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"

///////////////////// Nil /////////////////////////

void EditorPropertyNil::update_property() {
}

EditorPropertyNil::EditorPropertyNil() {
	Label *label = memnew(Label);
	label->set_text("[null]");
	add_child(label);
}

///////////////////// TEXT /////////////////////////

void EditorPropertyText::_set_read_only(bool p_read_only) {
	text->set_editable(!p_read_only);
};

void EditorPropertyText::_text_submitted(const String &p_string) {
	if (updating) {
		return;
	}

	if (text->has_focus()) {
		text->release_focus();
		_text_changed(p_string);
	}
}

void EditorPropertyText::_text_changed(const String &p_string) {
	if (updating) {
		return;
	}

	if (string_name) {
		emit_changed(get_edited_property(), StringName(p_string));
	} else {
		emit_changed(get_edited_property(), p_string);
	}
}

void EditorPropertyText::update_property() {
	String s = get_edited_object()->get(get_edited_property());
	updating = true;
	if (text->get_text() != s) {
		text->set_text(s);
	}
	text->set_editable(!is_read_only());
	updating = false;
}

void EditorPropertyText::set_string_name(bool p_enabled) {
	string_name = p_enabled;
}

void EditorPropertyText::set_placeholder(const String &p_string) {
	text->set_placeholder(p_string);
}

void EditorPropertyText::_bind_methods() {
}

EditorPropertyText::EditorPropertyText() {
	text = memnew(LineEdit);
	add_child(text);
	add_focusable(text);
	text->connect("text_changed", callable_mp(this, &EditorPropertyText::_text_changed));
	text->connect("text_submitted", callable_mp(this, &EditorPropertyText::_text_submitted));

	string_name = false;
	updating = false;
}

///////////////////// MULTILINE TEXT /////////////////////////

void EditorPropertyMultilineText::_set_read_only(bool p_read_only) {
	text->set_editable(!p_read_only);
	open_big_text->set_disabled(p_read_only);
};

void EditorPropertyMultilineText::_big_text_changed() {
	text->set_text(big_text->get_text());
	emit_changed(get_edited_property(), big_text->get_text(), "", true);
}

void EditorPropertyMultilineText::_text_changed() {
	emit_changed(get_edited_property(), text->get_text(), "", true);
}

void EditorPropertyMultilineText::_open_big_text() {
	if (!big_text_dialog) {
		big_text = memnew(TextEdit);
		big_text->connect("text_changed", callable_mp(this, &EditorPropertyMultilineText::_big_text_changed));
		big_text->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
		big_text_dialog = memnew(AcceptDialog);
		big_text_dialog->add_child(big_text);
		big_text_dialog->set_title(TTR("Edit Text:"));
		add_child(big_text_dialog);
	}

	big_text_dialog->popup_centered_clamped(Size2(1000, 900) * EDSCALE, 0.8);
	big_text->set_text(text->get_text());
	big_text->grab_focus();
}

void EditorPropertyMultilineText::update_property() {
	String t = get_edited_object()->get(get_edited_property());
	if (text->get_text() != t) {
		text->set_text(t);
		if (big_text && big_text->is_visible_in_tree()) {
			big_text->set_text(t);
		}
	}
}

void EditorPropertyMultilineText::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_ENTER_TREE: {
			Ref<Texture2D> df = get_theme_icon(SNAME("DistractionFree"), SNAME("EditorIcons"));
			open_big_text->set_icon(df);
			Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Label"));
			int font_size = get_theme_font_size(SNAME("font_size"), SNAME("Label"));
			text->set_custom_minimum_size(Vector2(0, font->get_height(font_size) * 6));

		} break;
	}
}

void EditorPropertyMultilineText::_bind_methods() {
}

EditorPropertyMultilineText::EditorPropertyMultilineText() {
	HBoxContainer *hb = memnew(HBoxContainer);
	add_child(hb);
	set_bottom_editor(hb);
	text = memnew(TextEdit);
	text->connect("text_changed", callable_mp(this, &EditorPropertyMultilineText::_text_changed));
	text->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	add_focusable(text);
	hb->add_child(text);
	text->set_h_size_flags(SIZE_EXPAND_FILL);
	open_big_text = memnew(Button);
	open_big_text->set_flat(true);
	open_big_text->connect("pressed", callable_mp(this, &EditorPropertyMultilineText::_open_big_text));
	hb->add_child(open_big_text);
	big_text_dialog = nullptr;
	big_text = nullptr;
}

///////////////////// TEXT ENUM /////////////////////////

void EditorPropertyTextEnum::_set_read_only(bool p_read_only) {
	option_button->set_disabled(p_read_only);
	edit_button->set_disabled(p_read_only);
};

void EditorPropertyTextEnum::_emit_changed_value(String p_string) {
	if (string_name) {
		emit_changed(get_edited_property(), StringName(p_string));
	} else {
		emit_changed(get_edited_property(), p_string);
	}
}

void EditorPropertyTextEnum::_option_selected(int p_which) {
	_emit_changed_value(option_button->get_item_text(p_which));
}

void EditorPropertyTextEnum::_edit_custom_value() {
	default_layout->hide();
	edit_custom_layout->show();
	custom_value_edit->grab_focus();
}

void EditorPropertyTextEnum::_custom_value_submitted(String p_value) {
	edit_custom_layout->hide();
	default_layout->show();

	_emit_changed_value(p_value.strip_edges());
}

void EditorPropertyTextEnum::_custom_value_accepted() {
	String new_value = custom_value_edit->get_text().strip_edges();
	_custom_value_submitted(new_value);
}

void EditorPropertyTextEnum::_custom_value_cancelled() {
	custom_value_edit->set_text(get_edited_object()->get(get_edited_property()));

	edit_custom_layout->hide();
	default_layout->show();
}

void EditorPropertyTextEnum::update_property() {
	String current_value = get_edited_object()->get(get_edited_property());
	int default_option = options.find(current_value);

	// The list can change in the loose mode.
	if (loose_mode) {
		custom_value_edit->set_text(current_value);
		option_button->clear();

		// Manually entered value.
		if (default_option < 0 && !current_value.is_empty()) {
			option_button->add_item(current_value, options.size() + 1001);
			option_button->select(0);

			option_button->add_separator();
		}

		// Add an explicit empty value for clearing the property.
		option_button->add_item("", options.size() + 1000);

		for (int i = 0; i < options.size(); i++) {
			option_button->add_item(options[i], i);
			if (options[i] == current_value) {
				option_button->select(option_button->get_item_count() - 1);
			}
		}
	} else {
		option_button->select(default_option);
	}
}

void EditorPropertyTextEnum::setup(const Vector<String> &p_options, bool p_string_name, bool p_loose_mode) {
	string_name = p_string_name;
	loose_mode = p_loose_mode;

	options.clear();

	if (loose_mode) {
		// Add an explicit empty value for clearing the property in the loose mode.
		option_button->add_item("", options.size() + 1000);
	}

	for (int i = 0; i < p_options.size(); i++) {
		options.append(p_options[i]);
		option_button->add_item(p_options[i], i);
	}

	if (loose_mode) {
		edit_button->show();
	}
}

void EditorPropertyTextEnum::_bind_methods() {
}

void EditorPropertyTextEnum::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			edit_button->set_icon(get_theme_icon(SNAME("Edit"), SNAME("EditorIcons")));
			accept_button->set_icon(get_theme_icon(SNAME("ImportCheck"), SNAME("EditorIcons")));
			cancel_button->set_icon(get_theme_icon(SNAME("ImportFail"), SNAME("EditorIcons")));
			break;
	}
}

EditorPropertyTextEnum::EditorPropertyTextEnum() {
	default_layout = memnew(HBoxContainer);
	add_child(default_layout);

	edit_custom_layout = memnew(HBoxContainer);
	edit_custom_layout->hide();
	add_child(edit_custom_layout);

	option_button = memnew(OptionButton);
	option_button->set_h_size_flags(SIZE_EXPAND_FILL);
	option_button->set_clip_text(true);
	option_button->set_flat(true);
	default_layout->add_child(option_button);
	option_button->connect("item_selected", callable_mp(this, &EditorPropertyTextEnum::_option_selected));

	edit_button = memnew(Button);
	edit_button->set_flat(true);
	edit_button->hide();
	default_layout->add_child(edit_button);
	edit_button->connect("pressed", callable_mp(this, &EditorPropertyTextEnum::_edit_custom_value));

	custom_value_edit = memnew(LineEdit);
	custom_value_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	edit_custom_layout->add_child(custom_value_edit);
	custom_value_edit->connect("text_submitted", callable_mp(this, &EditorPropertyTextEnum::_custom_value_submitted));

	accept_button = memnew(Button);
	accept_button->set_flat(true);
	edit_custom_layout->add_child(accept_button);
	accept_button->connect("pressed", callable_mp(this, &EditorPropertyTextEnum::_custom_value_accepted));

	cancel_button = memnew(Button);
	cancel_button->set_flat(true);
	edit_custom_layout->add_child(cancel_button);
	cancel_button->connect("pressed", callable_mp(this, &EditorPropertyTextEnum::_custom_value_cancelled));

	add_focusable(option_button);
	add_focusable(edit_button);
	add_focusable(custom_value_edit);
	add_focusable(accept_button);
	add_focusable(cancel_button);
}

///////////////////// PATH /////////////////////////

void EditorPropertyPath::_set_read_only(bool p_read_only) {
	path->set_editable(!p_read_only);
	path_edit->set_disabled(p_read_only);
};

void EditorPropertyPath::_path_selected(const String &p_path) {
	emit_changed(get_edited_property(), p_path);
	update_property();
}

void EditorPropertyPath::_path_pressed() {
	if (!dialog) {
		dialog = memnew(EditorFileDialog);
		dialog->connect("file_selected", callable_mp(this, &EditorPropertyPath::_path_selected));
		dialog->connect("dir_selected", callable_mp(this, &EditorPropertyPath::_path_selected));
		add_child(dialog);
	}

	String full_path = get_edited_object()->get(get_edited_property());

	dialog->clear_filters();

	if (global) {
		dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	} else {
		dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
	}

	if (folder) {
		dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
		dialog->set_current_dir(full_path);
	} else {
		dialog->set_file_mode(save_mode ? EditorFileDialog::FILE_MODE_SAVE_FILE : EditorFileDialog::FILE_MODE_OPEN_FILE);
		for (int i = 0; i < extensions.size(); i++) {
			String e = extensions[i].strip_edges();
			if (e != String()) {
				dialog->add_filter(extensions[i].strip_edges());
			}
		}
		dialog->set_current_path(full_path);
	}

	dialog->popup_file_dialog();
}

void EditorPropertyPath::update_property() {
	String full_path = get_edited_object()->get(get_edited_property());
	path->set_text(full_path);
	path->set_tooltip(full_path);
}

void EditorPropertyPath::setup(const Vector<String> &p_extensions, bool p_folder, bool p_global) {
	extensions = p_extensions;
	folder = p_folder;
	global = p_global;
}

void EditorPropertyPath::set_save_mode() {
	save_mode = true;
}

void EditorPropertyPath::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		path_edit->set_icon(get_theme_icon(SNAME("Folder"), SNAME("EditorIcons")));
	}
}

void EditorPropertyPath::_path_focus_exited() {
	_path_selected(path->get_text());
}

void EditorPropertyPath::_bind_methods() {
}

EditorPropertyPath::EditorPropertyPath() {
	HBoxContainer *path_hb = memnew(HBoxContainer);
	add_child(path_hb);
	path = memnew(LineEdit);
	path->set_structured_text_bidi_override(Control::STRUCTURED_TEXT_FILE);
	path_hb->add_child(path);
	path->connect("text_submitted", callable_mp(this, &EditorPropertyPath::_path_selected));
	path->connect("focus_exited", callable_mp(this, &EditorPropertyPath::_path_focus_exited));
	path->set_h_size_flags(SIZE_EXPAND_FILL);

	path_edit = memnew(Button);
	path_edit->set_clip_text(true);
	path_hb->add_child(path_edit);
	add_focusable(path);
	dialog = nullptr;
	path_edit->connect("pressed", callable_mp(this, &EditorPropertyPath::_path_pressed));
	folder = false;
	global = false;
	save_mode = false;
}

///////////////////// CLASS NAME /////////////////////////

void EditorPropertyClassName::_set_read_only(bool p_read_only) {
	property->set_disabled(p_read_only);
};

void EditorPropertyClassName::setup(const String &p_base_type, const String &p_selected_type) {
	base_type = p_base_type;
	dialog->set_base_type(base_type);
	selected_type = p_selected_type;
	property->set_text(selected_type);
}

void EditorPropertyClassName::update_property() {
	String s = get_edited_object()->get(get_edited_property());
	property->set_text(s);
	selected_type = s;
}

void EditorPropertyClassName::_property_selected() {
	dialog->popup_create(true);
}

void EditorPropertyClassName::_dialog_created() {
	selected_type = dialog->get_selected_type();
	emit_changed(get_edited_property(), selected_type);
	update_property();
}

void EditorPropertyClassName::_bind_methods() {
}

EditorPropertyClassName::EditorPropertyClassName() {
	property = memnew(Button);
	property->set_clip_text(true);
	add_child(property);
	add_focusable(property);
	property->set_text(selected_type);
	property->connect("pressed", callable_mp(this, &EditorPropertyClassName::_property_selected));
	dialog = memnew(CreateDialog);
	dialog->set_base_type(base_type);
	dialog->connect("create", callable_mp(this, &EditorPropertyClassName::_dialog_created));
	add_child(dialog);
}

///////////////////// MEMBER /////////////////////////

void EditorPropertyMember::_set_read_only(bool p_read_only) {
	property->set_disabled(p_read_only);
};

void EditorPropertyMember::_property_selected(const String &p_selected) {
	emit_changed(get_edited_property(), p_selected);
	update_property();
}

void EditorPropertyMember::_property_select() {
	if (!selector) {
		selector = memnew(PropertySelector);
		selector->connect("selected", callable_mp(this, &EditorPropertyMember::_property_selected));
		add_child(selector);
	}

	String current = get_edited_object()->get(get_edited_property());

	if (hint == MEMBER_METHOD_OF_VARIANT_TYPE) {
		Variant::Type type = Variant::NIL;
		for (int i = 0; i < Variant::VARIANT_MAX; i++) {
			if (hint_text == Variant::get_type_name(Variant::Type(i))) {
				type = Variant::Type(i);
			}
		}
		if (type != Variant::NIL) {
			selector->select_method_from_basic_type(type, current);
		}

	} else if (hint == MEMBER_METHOD_OF_BASE_TYPE) {
		selector->select_method_from_base_type(hint_text, current);

	} else if (hint == MEMBER_METHOD_OF_INSTANCE) {
		Object *instance = ObjectDB::get_instance(ObjectID(hint_text.to_int()));
		if (instance) {
			selector->select_method_from_instance(instance, current);
		}

	} else if (hint == MEMBER_METHOD_OF_SCRIPT) {
		Object *obj = ObjectDB::get_instance(ObjectID(hint_text.to_int()));
		if (Object::cast_to<Script>(obj)) {
			selector->select_method_from_script(Object::cast_to<Script>(obj), current);
		}

	} else if (hint == MEMBER_PROPERTY_OF_VARIANT_TYPE) {
		Variant::Type type = Variant::NIL;
		String tname = hint_text;
		if (tname.find(".") != -1) {
			tname = tname.get_slice(".", 0);
		}
		for (int i = 0; i < Variant::VARIANT_MAX; i++) {
			if (tname == Variant::get_type_name(Variant::Type(i))) {
				type = Variant::Type(Variant::Type(i));
			}
		}

		if (type != Variant::NIL) {
			selector->select_property_from_basic_type(type, current);
		}

	} else if (hint == MEMBER_PROPERTY_OF_BASE_TYPE) {
		selector->select_property_from_base_type(hint_text, current);

	} else if (hint == MEMBER_PROPERTY_OF_INSTANCE) {
		Object *instance = ObjectDB::get_instance(ObjectID(hint_text.to_int()));
		if (instance) {
			selector->select_property_from_instance(instance, current);
		}

	} else if (hint == MEMBER_PROPERTY_OF_SCRIPT) {
		Object *obj = ObjectDB::get_instance(ObjectID(hint_text.to_int()));
		if (Object::cast_to<Script>(obj)) {
			selector->select_property_from_script(Object::cast_to<Script>(obj), current);
		}
	}
}

void EditorPropertyMember::setup(Type p_hint, const String &p_hint_text) {
	hint = p_hint;
	hint_text = p_hint_text;
}

void EditorPropertyMember::update_property() {
	String full_path = get_edited_object()->get(get_edited_property());
	property->set_text(full_path);
}

void EditorPropertyMember::_bind_methods() {
}

EditorPropertyMember::EditorPropertyMember() {
	selector = nullptr;
	property = memnew(Button);
	property->set_clip_text(true);
	add_child(property);
	add_focusable(property);
	property->connect("pressed", callable_mp(this, &EditorPropertyMember::_property_select));
}

///////////////////// CHECK /////////////////////////

void EditorPropertyCheck::_set_read_only(bool p_read_only) {
	checkbox->set_disabled(p_read_only);
};

void EditorPropertyCheck::_checkbox_pressed() {
	emit_changed(get_edited_property(), checkbox->is_pressed());
}

void EditorPropertyCheck::update_property() {
	bool c = get_edited_object()->get(get_edited_property());
	checkbox->set_pressed(c);
	checkbox->set_disabled(is_read_only());
}

void EditorPropertyCheck::_bind_methods() {
}

EditorPropertyCheck::EditorPropertyCheck() {
	checkbox = memnew(CheckBox);
	checkbox->set_text(TTR("On"));
	add_child(checkbox);
	add_focusable(checkbox);
	checkbox->connect("pressed", callable_mp(this, &EditorPropertyCheck::_checkbox_pressed));
}

///////////////////// ENUM /////////////////////////

void EditorPropertyEnum::_set_read_only(bool p_read_only) {
	options->set_disabled(p_read_only);
};

void EditorPropertyEnum::_option_selected(int p_which) {
	int64_t val = options->get_item_metadata(p_which);
	emit_changed(get_edited_property(), val);
}

void EditorPropertyEnum::update_property() {
	int64_t which = get_edited_object()->get(get_edited_property());

	for (int i = 0; i < options->get_item_count(); i++) {
		if (which == (int64_t)options->get_item_metadata(i)) {
			options->select(i);
			return;
		}
	}
}

void EditorPropertyEnum::setup(const Vector<String> &p_options) {
	options->clear();
	int64_t current_val = 0;
	for (int i = 0; i < p_options.size(); i++) {
		Vector<String> text_split = p_options[i].split(":");
		if (text_split.size() != 1) {
			current_val = text_split[1].to_int();
		}
		options->add_item(text_split[0]);
		options->set_item_metadata(i, current_val);
		current_val += 1;
	}
}

void EditorPropertyEnum::set_option_button_clip(bool p_enable) {
	options->set_clip_text(p_enable);
}

void EditorPropertyEnum::_bind_methods() {
}

EditorPropertyEnum::EditorPropertyEnum() {
	options = memnew(OptionButton);
	options->set_clip_text(true);
	options->set_flat(true);
	add_child(options);
	add_focusable(options);
	options->connect("item_selected", callable_mp(this, &EditorPropertyEnum::_option_selected));
}

///////////////////// FLAGS /////////////////////////

void EditorPropertyFlags::_set_read_only(bool p_read_only) {
	for (CheckBox *check : flags) {
		check->set_disabled(p_read_only);
	}
};

void EditorPropertyFlags::_flag_toggled() {
	uint32_t value = 0;
	for (int i = 0; i < flags.size(); i++) {
		if (flags[i]->is_pressed()) {
			uint32_t val = 1;
			val <<= flag_indices[i];
			value |= val;
		}
	}

	emit_changed(get_edited_property(), value);
}

void EditorPropertyFlags::update_property() {
	uint32_t value = get_edited_object()->get(get_edited_property());

	for (int i = 0; i < flags.size(); i++) {
		uint32_t val = 1;
		val <<= flag_indices[i];
		if (value & val) {
			flags[i]->set_pressed(true);
		} else {
			flags[i]->set_pressed(false);
		}
	}
}

void EditorPropertyFlags::setup(const Vector<String> &p_options) {
	ERR_FAIL_COND(flags.size());

	bool first = true;
	for (int i = 0; i < p_options.size(); i++) {
		String option = p_options[i].strip_edges();
		if (option != "") {
			CheckBox *cb = memnew(CheckBox);
			cb->set_text(option);
			cb->set_clip_text(true);
			cb->connect("pressed", callable_mp(this, &EditorPropertyFlags::_flag_toggled));
			add_focusable(cb);
			vbox->add_child(cb);
			flags.push_back(cb);
			flag_indices.push_back(i);
			if (first) {
				set_label_reference(cb);
				first = false;
			}
		}
	}
}

void EditorPropertyFlags::_bind_methods() {
}

EditorPropertyFlags::EditorPropertyFlags() {
	vbox = memnew(VBoxContainer);
	add_child(vbox);
}

///////////////////// LAYERS /////////////////////////

class EditorPropertyLayersGrid : public Control {
	GDCLASS(EditorPropertyLayersGrid, Control);

private:
	Vector<Rect2> flag_rects;
	Rect2 expand_rect;
	bool expand_hovered = false;
	bool expanded = false;
	int expansion_rows = 0;
	int hovered_index = -1;
	bool read_only = false;

	Size2 get_grid_size() const {
		Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Label"));
		int font_size = get_theme_font_size(SNAME("font_size"), SNAME("Label"));
		return Vector2(0, font->get_height(font_size) * 3);
	}

public:
	uint32_t value = 0;
	int layer_group_size = 0;
	int layer_count = 0;
	Vector<String> names;
	Vector<String> tooltips;

	void set_read_only(bool p_read_only) {
		read_only = p_read_only;
	}

	virtual Size2 get_minimum_size() const override {
		Size2 min_size = get_grid_size();

		// Add extra rows when expanded.
		if (expanded) {
			const int bsize = (min_size.height * 80 / 100) / 2;
			for (int i = 0; i < expansion_rows; ++i) {
				min_size.y += 2 * (bsize + 1) + 3;
			}
		}

		return min_size;
	}

	virtual String get_tooltip(const Point2 &p_pos) const override {
		for (int i = 0; i < flag_rects.size(); i++) {
			if (i < tooltips.size() && flag_rects[i].has_point(p_pos)) {
				return tooltips[i];
			}
		}
		return String();
	}

	void gui_input(const Ref<InputEvent> &p_ev) override {
		if (read_only) {
			return;
		}
		const Ref<InputEventMouseMotion> mm = p_ev;
		if (mm.is_valid()) {
			bool expand_was_hovered = expand_hovered;
			expand_hovered = expand_rect.has_point(mm->get_position());
			if (expand_hovered != expand_was_hovered) {
				update();
			}

			if (!expand_hovered) {
				for (int i = 0; i < flag_rects.size(); i++) {
					if (flag_rects[i].has_point(mm->get_position())) {
						// Used to highlight the hovered flag in the layers grid.
						hovered_index = i;
						update();
						return;
					}
				}
			}

			// Remove highlight when no square is hovered.
			if (hovered_index != -1) {
				hovered_index = -1;
				update();
			}

			return;
		}

		const Ref<InputEventMouseButton> mb = p_ev;
		if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT && mb->is_pressed()) {
			if (hovered_index >= 0) {
				// Toggle the flag.
				// We base our choice on the hovered flag, so that it always matches the hovered flag.
				if (value & (1 << hovered_index)) {
					value &= ~(1 << hovered_index);
				} else {
					value |= (1 << hovered_index);
				}

				emit_signal(SNAME("flag_changed"), value);
				update();
			} else if (expand_hovered) {
				expanded = !expanded;
				update_minimum_size();
				update();
			}
		}
	}

	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_DRAW: {
				Size2 grid_size = get_grid_size();
				grid_size.x = get_size().x;

				flag_rects.clear();

				int prev_expansion_rows = expansion_rows;
				expansion_rows = 0;

				const int bsize = (grid_size.height * 80 / 100) / 2;
				const int h = bsize * 2 + 1;

				Color color = get_theme_color(read_only ? SNAME("disabled_highlight_color") : SNAME("highlight_color"), SNAME("Editor"));

				Color text_color = get_theme_color(read_only ? SNAME("disabled_font_color") : SNAME("font_color"), SNAME("Editor"));
				text_color.a *= 0.5;

				Color text_color_on = get_theme_color(read_only ? SNAME("disabled_font_color") : SNAME("font_hover_color"), SNAME("Editor"));
				text_color_on.a *= 0.7;

				const int vofs = (grid_size.height - h) / 2;

				int layer_index = 0;
				int block_index = 0;

				Point2 arrow_pos;

				Point2 block_ofs(4, vofs);

				while (true) {
					Point2 ofs = block_ofs;

					for (int i = 0; i < 2; i++) {
						for (int j = 0; j < layer_group_size; j++) {
							const bool on = value & (1 << layer_index);
							Rect2 rect2 = Rect2(ofs, Size2(bsize, bsize));

							color.a = on ? 0.6 : 0.2;
							if (layer_index == hovered_index) {
								// Add visual feedback when hovering a flag.
								color.a += 0.15;
							}

							draw_rect(rect2, color);
							flag_rects.push_back(rect2);

							Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Label"));
							int font_size = get_theme_font_size(SNAME("font_size"), SNAME("Label"));
							Vector2 offset;
							offset.y = rect2.size.y * 0.75;

							draw_string(font, rect2.position + offset, itos(layer_index + 1), HALIGN_CENTER, rect2.size.x, font_size, on ? text_color_on : text_color);

							ofs.x += bsize + 1;

							++layer_index;
						}

						ofs.x = block_ofs.x;
						ofs.y += bsize + 1;
					}

					if (layer_index >= layer_count) {
						if (!flag_rects.is_empty() && (expansion_rows == 0)) {
							const Rect2 &last_rect = flag_rects[flag_rects.size() - 1];
							arrow_pos = last_rect.get_end();
						}
						break;
					}

					int block_size_x = layer_group_size * (bsize + 1);
					block_ofs.x += block_size_x + 3;

					if (block_ofs.x + block_size_x + 12 > grid_size.width) {
						// Keep last valid cell position for the expansion icon.
						if (!flag_rects.is_empty() && (expansion_rows == 0)) {
							const Rect2 &last_rect = flag_rects[flag_rects.size() - 1];
							arrow_pos = last_rect.get_end();
						}
						++expansion_rows;

						if (expanded) {
							// Expand grid to next line.
							block_ofs.x = 4;
							block_ofs.y += 2 * (bsize + 1) + 3;
						} else {
							// Skip remaining blocks.
							break;
						}
					}

					++block_index;
				}

				if ((expansion_rows != prev_expansion_rows) && expanded) {
					update_minimum_size();
				}

				if ((expansion_rows == 0) && (layer_index == layer_count)) {
					// Whole grid was drawn, no need for expansion icon.
					break;
				}

				Ref<Texture2D> arrow = get_theme_icon(SNAME("arrow"), SNAME("Tree"));
				ERR_FAIL_COND(arrow.is_null());

				Color arrow_color = get_theme_color(SNAME("highlight_color"), SNAME("Editor"));
				arrow_color.a = expand_hovered ? 1.0 : 0.6;

				arrow_pos.x += 2.0;
				arrow_pos.y -= arrow->get_height();

				Rect2 arrow_draw_rect(arrow_pos, arrow->get_size());
				expand_rect = arrow_draw_rect;
				if (expanded) {
					arrow_draw_rect.size.y *= -1.0; // Flip arrow vertically when expanded.
				}

				RID ci = get_canvas_item();
				arrow->draw_rect(ci, arrow_draw_rect, false, arrow_color);

			} break;

			case NOTIFICATION_MOUSE_EXIT: {
				if (expand_hovered) {
					expand_hovered = false;
					update();
				}
				if (hovered_index != -1) {
					hovered_index = -1;
					update();
				}
			} break;

			default:
				break;
		}
	}

	void set_flag(uint32_t p_flag) {
		value = p_flag;
		update();
	}

	static void _bind_methods() {
		ADD_SIGNAL(MethodInfo("flag_changed", PropertyInfo(Variant::INT, "flag")));
	}
};

void EditorPropertyLayers::_set_read_only(bool p_read_only) {
	button->set_disabled(p_read_only);
	grid->set_read_only(p_read_only);
};

void EditorPropertyLayers::_grid_changed(uint32_t p_grid) {
	emit_changed(get_edited_property(), p_grid);
}

void EditorPropertyLayers::update_property() {
	uint32_t value = get_edited_object()->get(get_edited_property());

	grid->set_flag(value);
}

void EditorPropertyLayers::setup(LayerType p_layer_type) {
	String basename;
	int layer_group_size = 0;
	int layer_count = 0;
	switch (p_layer_type) {
		case LAYER_RENDER_2D: {
			basename = "layer_names/2d_render";
			layer_group_size = 5;
			layer_count = 20;
		} break;

		case LAYER_PHYSICS_2D: {
			basename = "layer_names/2d_physics";
			layer_group_size = 4;
			layer_count = 32;
		} break;

		case LAYER_NAVIGATION_2D: {
			basename = "layer_names/2d_navigation";
			layer_group_size = 4;
			layer_count = 32;
		} break;

		case LAYER_RENDER_3D: {
			basename = "layer_names/3d_render";
			layer_group_size = 5;
			layer_count = 20;
		} break;

		case LAYER_PHYSICS_3D: {
			basename = "layer_names/3d_physics";
			layer_group_size = 4;
			layer_count = 32;
		} break;

		case LAYER_NAVIGATION_3D: {
			basename = "layer_names/3d_navigation";
			layer_group_size = 4;
			layer_count = 32;
		} break;
	}

	Vector<String> names;
	Vector<String> tooltips;
	for (int i = 0; i < layer_count; i++) {
		String name;

		if (ProjectSettings::get_singleton()->has_setting(basename + vformat("/layer_%d", i + 1))) {
			name = ProjectSettings::get_singleton()->get(basename + vformat("/layer_%d", i + 1));
		}

		if (name == "") {
			name = vformat(TTR("Layer %d"), i + 1);
		}

		names.push_back(name);
		tooltips.push_back(name + "\n" + vformat(TTR("Bit %d, value %d"), i, 1 << i));
	}

	grid->names = names;
	grid->tooltips = tooltips;
	grid->layer_group_size = layer_group_size;
	grid->layer_count = layer_count;
}

void EditorPropertyLayers::_button_pressed() {
	int layer_count = grid->layer_count;
	int layer_group_size = grid->layer_group_size;

	layers->clear();
	for (int i = 0; i < layer_count; i++) {
		if ((i != 0) && ((i % layer_group_size) == 0)) {
			layers->add_separator();
		}
		layers->add_check_item(grid->names[i], i);
		int idx = layers->get_item_index(i);
		layers->set_item_checked(idx, grid->value & (1 << i));
	}

	Rect2 gp = button->get_screen_rect();
	layers->set_as_minsize();
	Vector2 popup_pos = gp.position - Vector2(layers->get_contents_minimum_size().x, 0);
	layers->set_position(popup_pos);
	layers->popup();
}

void EditorPropertyLayers::_menu_pressed(int p_menu) {
	if (grid->value & (1 << p_menu)) {
		grid->value &= ~(1 << p_menu);
	} else {
		grid->value |= (1 << p_menu);
	}
	grid->update();
	layers->set_item_checked(layers->get_item_index(p_menu), grid->value & (1 << p_menu));
	_grid_changed(grid->value);
}

void EditorPropertyLayers::_bind_methods() {
}

EditorPropertyLayers::EditorPropertyLayers() {
	HBoxContainer *hb = memnew(HBoxContainer);
	hb->set_clip_contents(true);
	add_child(hb);
	grid = memnew(EditorPropertyLayersGrid);
	grid->connect("flag_changed", callable_mp(this, &EditorPropertyLayers::_grid_changed));
	grid->set_h_size_flags(SIZE_EXPAND_FILL);
	hb->add_child(grid);

	button = memnew(Button);
	button->set_toggle_mode(true);
	button->set_text("...");
	button->connect("pressed", callable_mp(this, &EditorPropertyLayers::_button_pressed));
	hb->add_child(button);

	set_bottom_editor(hb);

	layers = memnew(PopupMenu);
	add_child(layers);
	layers->set_hide_on_checkable_item_selection(false);
	layers->connect("id_pressed", callable_mp(this, &EditorPropertyLayers::_menu_pressed));
	layers->connect("popup_hide", callable_mp((BaseButton *)button, &BaseButton::set_pressed), varray(false));
}

///////////////////// INT /////////////////////////

void EditorPropertyInteger::_set_read_only(bool p_read_only) {
	spin->set_read_only(p_read_only);
};

void EditorPropertyInteger::_value_changed(int64_t val) {
	if (setting) {
		return;
	}
	emit_changed(get_edited_property(), val);
}

void EditorPropertyInteger::update_property() {
	int64_t val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin->set_value(val);
	setting = false;
#ifdef DEBUG_ENABLED
	// If spin (currently EditorSplinSlider : Range) is changed so that it can use int64_t, then the below warning wouldn't be a problem.
	if (val != (int64_t)(double)(val)) {
		WARN_PRINT("Cannot reliably represent '" + itos(val) + "' in the inspector, value is too large.");
	}
#endif
}

void EditorPropertyInteger::_bind_methods() {
}

void EditorPropertyInteger::setup(int64_t p_min, int64_t p_max, int64_t p_step, bool p_allow_greater, bool p_allow_lesser) {
	spin->set_min(p_min);
	spin->set_max(p_max);
	spin->set_step(p_step);
	spin->set_allow_greater(p_allow_greater);
	spin->set_allow_lesser(p_allow_lesser);
}

EditorPropertyInteger::EditorPropertyInteger() {
	spin = memnew(EditorSpinSlider);
	spin->set_flat(true);
	add_child(spin);
	add_focusable(spin);
	spin->connect("value_changed", callable_mp(this, &EditorPropertyInteger::_value_changed));
	setting = false;
}

///////////////////// OBJECT ID /////////////////////////

void EditorPropertyObjectID::_set_read_only(bool p_read_only) {
	edit->set_disabled(p_read_only);
};

void EditorPropertyObjectID::_edit_pressed() {
	emit_signal(SNAME("object_id_selected"), get_edited_property(), get_edited_object()->get(get_edited_property()));
}

void EditorPropertyObjectID::update_property() {
	String type = base_type;
	if (type == "") {
		type = "Object";
	}

	ObjectID id = get_edited_object()->get(get_edited_property());
	if (id.is_valid()) {
		edit->set_text(type + " ID: " + itos(id));
		edit->set_disabled(false);
		edit->set_icon(EditorNode::get_singleton()->get_class_icon(type));
	} else {
		edit->set_text(TTR("[Empty]"));
		edit->set_disabled(true);
		edit->set_icon(Ref<Texture2D>());
	}
}

void EditorPropertyObjectID::setup(const String &p_base_type) {
	base_type = p_base_type;
}

void EditorPropertyObjectID::_bind_methods() {
}

EditorPropertyObjectID::EditorPropertyObjectID() {
	edit = memnew(Button);
	add_child(edit);
	add_focusable(edit);
	edit->connect("pressed", callable_mp(this, &EditorPropertyObjectID::_edit_pressed));
}

///////////////////// FLOAT /////////////////////////

void EditorPropertyFloat::_set_read_only(bool p_read_only) {
	spin->set_read_only(p_read_only);
};

void EditorPropertyFloat::_value_changed(double val) {
	if (setting) {
		return;
	}

	if (angle_in_radians) {
		val = Math::deg2rad(val);
	}
	emit_changed(get_edited_property(), val);
}

void EditorPropertyFloat::update_property() {
	double val = get_edited_object()->get(get_edited_property());
	if (angle_in_radians) {
		val = Math::rad2deg(val);
	}
	setting = true;
	spin->set_value(val);
	setting = false;
}

void EditorPropertyFloat::_bind_methods() {
}

void EditorPropertyFloat::setup(double p_min, double p_max, double p_step, bool p_no_slider, bool p_exp_range, bool p_greater, bool p_lesser, const String &p_suffix, bool p_angle_in_radians) {
	angle_in_radians = p_angle_in_radians;
	spin->set_min(p_min);
	spin->set_max(p_max);
	spin->set_step(p_step);
	spin->set_hide_slider(p_no_slider);
	spin->set_exp_ratio(p_exp_range);
	spin->set_allow_greater(p_greater);
	spin->set_allow_lesser(p_lesser);
	spin->set_suffix(p_suffix);
}

EditorPropertyFloat::EditorPropertyFloat() {
	spin = memnew(EditorSpinSlider);
	spin->set_flat(true);
	add_child(spin);
	add_focusable(spin);
	spin->connect("value_changed", callable_mp(this, &EditorPropertyFloat::_value_changed));
}

///////////////////// EASING /////////////////////////

void EditorPropertyEasing::_set_read_only(bool p_read_only) {
	spin->set_read_only(p_read_only);
};

void EditorPropertyEasing::_drag_easing(const Ref<InputEvent> &p_ev) {
	if (is_read_only()) {
		return;
	}
	const Ref<InputEventMouseButton> mb = p_ev;
	if (mb.is_valid()) {
		if (mb->is_double_click() && mb->get_button_index() == MouseButton::LEFT) {
			_setup_spin();
		}

		if (mb->is_pressed() && mb->get_button_index() == MouseButton::RIGHT) {
			preset->set_position(easing_draw->get_screen_position() + mb->get_position());
			preset->reset_size();
			preset->popup();

			// Ensure the easing doesn't appear as being dragged
			dragging = false;
			easing_draw->update();
		}

		if (mb->get_button_index() == MouseButton::LEFT) {
			dragging = mb->is_pressed();
			// Update to display the correct dragging color
			easing_draw->update();
		}
	}

	const Ref<InputEventMouseMotion> mm = p_ev;

	if (dragging && mm.is_valid() && (mm->get_button_mask() & MouseButton::MASK_LEFT) != MouseButton::NONE) {
		float rel = mm->get_relative().x;
		if (rel == 0) {
			return;
		}

		if (flip) {
			rel = -rel;
		}

		float val = get_edited_object()->get(get_edited_property());
		bool sg = val < 0;
		val = Math::absf(val);

		val = Math::log(val) / Math::log((float)2.0);
		// Logarithmic space.
		val += rel * 0.05;

		val = Math::pow(2.0f, val);
		if (sg) {
			val = -val;
		}

		// 0 is a singularity, but both positive and negative values
		// are otherwise allowed. Enforce 0+ as workaround.
		if (Math::is_zero_approx(val)) {
			val = 0.00001;
		}

		// Limit to a reasonable value to prevent the curve going into infinity,
		// which can cause crashes and other issues.
		val = CLAMP(val, -1'000'000, 1'000'000);

		emit_changed(get_edited_property(), val);
		easing_draw->update();
	}
}

void EditorPropertyEasing::_draw_easing() {
	RID ci = easing_draw->get_canvas_item();

	Size2 s = easing_draw->get_size();

	const int point_count = 48;

	const float exp = get_edited_object()->get(get_edited_property());

	const Ref<Font> f = get_theme_font(SNAME("font"), SNAME("Label"));
	int font_size = get_theme_font_size(SNAME("font_size"), SNAME("Label"));
	const Color font_color = get_theme_color(is_read_only() ? SNAME("font_uneditable_color") : SNAME("font_color"), SNAME("LineEdit"));
	Color line_color;
	if (dragging) {
		line_color = get_theme_color(SNAME("accent_color"), SNAME("Editor"));
	} else {
		line_color = get_theme_color(is_read_only() ? SNAME("font_uneditable_color") : SNAME("font_color"), SNAME("LineEdit")) * Color(1, 1, 1, 0.9);
	}

	Vector<Point2> points;
	for (int i = 0; i <= point_count; i++) {
		float ifl = i / float(point_count);

		const float h = 1.0 - Math::ease(ifl, exp);

		if (flip) {
			ifl = 1.0 - ifl;
		}

		points.push_back(Point2(ifl * s.width, h * s.height));
	}

	easing_draw->draw_polyline(points, line_color, 1.0, true);
	// Draw more decimals for small numbers since higher precision is usually required for fine adjustments.
	int decimals;
	if (Math::abs(exp) < 0.1 - CMP_EPSILON) {
		decimals = 4;
	} else if (Math::abs(exp) < 1 - CMP_EPSILON) {
		decimals = 3;
	} else if (Math::abs(exp) < 10 - CMP_EPSILON) {
		decimals = 2;
	} else {
		decimals = 1;
	}
	f->draw_string(ci, Point2(10, 10 + f->get_ascent(font_size)), TS->format_number(rtos(exp).pad_decimals(decimals)), HALIGN_LEFT, -1, font_size, font_color);
}

void EditorPropertyEasing::update_property() {
	easing_draw->update();
}

void EditorPropertyEasing::_set_preset(int p_preset) {
	static const float preset_value[EASING_MAX] = { 0.0, 1.0, 2.0, 0.5, -2.0, -0.5 };

	emit_changed(get_edited_property(), preset_value[p_preset]);
	easing_draw->update();
}

void EditorPropertyEasing::_setup_spin() {
	setting = true;
	spin->setup_and_show();
	spin->get_line_edit()->set_text(TS->format_number(rtos(get_edited_object()->get(get_edited_property()))));
	setting = false;
	spin->show();
}

void EditorPropertyEasing::_spin_value_changed(double p_value) {
	if (setting) {
		return;
	}

	// 0 is a singularity, but both positive and negative values
	// are otherwise allowed. Enforce 0+ as workaround.
	if (Math::is_zero_approx(p_value)) {
		p_value = 0.00001;
	}

	// Limit to a reasonable value to prevent the curve going into infinity,
	// which can cause crashes and other issues.
	p_value = CLAMP(p_value, -1'000'000, 1'000'000);

	emit_changed(get_edited_property(), p_value);
	_spin_focus_exited();
}

void EditorPropertyEasing::_spin_focus_exited() {
	spin->hide();
	// Ensure the easing doesn't appear as being dragged
	dragging = false;
	easing_draw->update();
}

void EditorPropertyEasing::setup(bool p_full, bool p_flip) {
	flip = p_flip;
	full = p_full;
}

void EditorPropertyEasing::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_ENTER_TREE: {
			preset->clear();
			preset->add_icon_item(get_theme_icon(SNAME("CurveConstant"), SNAME("EditorIcons")), "Zero", EASING_ZERO);
			preset->add_icon_item(get_theme_icon(SNAME("CurveLinear"), SNAME("EditorIcons")), "Linear", EASING_LINEAR);
			preset->add_icon_item(get_theme_icon(SNAME("CurveIn"), SNAME("EditorIcons")), "In", EASING_IN);
			preset->add_icon_item(get_theme_icon(SNAME("CurveOut"), SNAME("EditorIcons")), "Out", EASING_OUT);
			if (full) {
				preset->add_icon_item(get_theme_icon(SNAME("CurveInOut"), SNAME("EditorIcons")), "In-Out", EASING_IN_OUT);
				preset->add_icon_item(get_theme_icon(SNAME("CurveOutIn"), SNAME("EditorIcons")), "Out-In", EASING_OUT_IN);
			}
			easing_draw->set_custom_minimum_size(Size2(0, get_theme_font(SNAME("font"), SNAME("Label"))->get_height(get_theme_font_size(SNAME("font_size"), SNAME("Label"))) * 2));
		} break;
	}
}

void EditorPropertyEasing::_bind_methods() {
}

EditorPropertyEasing::EditorPropertyEasing() {
	easing_draw = memnew(Control);
	easing_draw->connect("draw", callable_mp(this, &EditorPropertyEasing::_draw_easing));
	easing_draw->connect("gui_input", callable_mp(this, &EditorPropertyEasing::_drag_easing));
	easing_draw->set_default_cursor_shape(Control::CURSOR_MOVE);
	add_child(easing_draw);

	preset = memnew(PopupMenu);
	add_child(preset);
	preset->connect("id_pressed", callable_mp(this, &EditorPropertyEasing::_set_preset));

	spin = memnew(EditorSpinSlider);
	spin->set_flat(true);
	spin->set_min(-100);
	spin->set_max(100);
	spin->set_step(0);
	spin->set_hide_slider(true);
	spin->set_allow_lesser(true);
	spin->set_allow_greater(true);
	spin->connect("value_changed", callable_mp(this, &EditorPropertyEasing::_spin_value_changed));
	spin->get_line_edit()->connect("focus_exited", callable_mp(this, &EditorPropertyEasing::_spin_focus_exited));
	spin->hide();
	add_child(spin);

	dragging = false;
	flip = false;
	full = false;
}

///////////////////// VECTOR2 /////////////////////////

void EditorPropertyVector2::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 2; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyVector2::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Vector2 v2;
	v2.x = spin[0]->get_value();
	v2.y = spin[1]->get_value();
	emit_changed(get_edited_property(), v2, p_name);
}

void EditorPropertyVector2::update_property() {
	Vector2 val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.x);
	spin[1]->set_value(val.y);
	setting = false;
}

void EditorPropertyVector2::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 2; i++) {
			spin[i]->set_custom_label_color(true, colors[i]);
		}
	}
}

void EditorPropertyVector2::_bind_methods() {
}

void EditorPropertyVector2::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 2; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyVector2::EditorPropertyVector2(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector2_editing"));

	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[2] = { "x", "y" };
	for (int i = 0; i < 2; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_flat(true);
		spin[i]->set_label(desc[i]);
		bc->add_child(spin[i]);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyVector2::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// RECT2 /////////////////////////

void EditorPropertyRect2::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyRect2::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Rect2 r2;
	r2.position.x = spin[0]->get_value();
	r2.position.y = spin[1]->get_value();
	r2.size.x = spin[2]->get_value();
	r2.size.y = spin[3]->get_value();
	emit_changed(get_edited_property(), r2, p_name);
}

void EditorPropertyRect2::update_property() {
	Rect2 val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.position.x);
	spin[1]->set_value(val.position.y);
	spin[2]->set_value(val.size.x);
	spin[3]->set_value(val.size.y);
	setting = false;
}

void EditorPropertyRect2::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->set_custom_label_color(true, colors[i % 2]);
		}
	}
}

void EditorPropertyRect2::_bind_methods() {
}

void EditorPropertyRect2::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyRect2::EditorPropertyRect2(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector_types_editing"));
	bool grid = false;
	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(VBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);

		bc->add_child(memnew(HBoxContainer));
		bc->add_child(memnew(HBoxContainer));
		grid = true;
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[4] = { "x", "y", "w", "h" };
	for (int i = 0; i < 4; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_label(desc[i]);
		spin[i]->set_flat(true);

		if (grid) {
			bc->get_child(i / 2)->add_child(spin[i]);
		} else {
			bc->add_child(spin[i]);
		}

		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyRect2::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// VECTOR3 /////////////////////////

void EditorPropertyVector3::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 3; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyVector3::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Vector3 v3;
	v3.x = spin[0]->get_value();
	v3.y = spin[1]->get_value();
	v3.z = spin[2]->get_value();
	if (angle_in_radians) {
		v3.x = Math::deg2rad(v3.x);
		v3.y = Math::deg2rad(v3.y);
		v3.z = Math::deg2rad(v3.z);
	}
	emit_changed(get_edited_property(), v3, p_name);
}

void EditorPropertyVector3::update_property() {
	update_using_vector(get_edited_object()->get(get_edited_property()));
}

void EditorPropertyVector3::update_using_vector(Vector3 p_vector) {
	if (angle_in_radians) {
		p_vector.x = Math::rad2deg(p_vector.x);
		p_vector.y = Math::rad2deg(p_vector.y);
		p_vector.z = Math::rad2deg(p_vector.z);
	}
	setting = true;
	spin[0]->set_value(p_vector.x);
	spin[1]->set_value(p_vector.y);
	spin[2]->set_value(p_vector.z);
	setting = false;
}

Vector3 EditorPropertyVector3::get_vector() {
	Vector3 v3;
	v3.x = spin[0]->get_value();
	v3.y = spin[1]->get_value();
	v3.z = spin[2]->get_value();
	if (angle_in_radians) {
		v3.x = Math::deg2rad(v3.x);
		v3.y = Math::deg2rad(v3.y);
		v3.z = Math::deg2rad(v3.z);
	}

	return v3;
}

void EditorPropertyVector3::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 3; i++) {
			spin[i]->set_custom_label_color(true, colors[i]);
		}
	}
}

void EditorPropertyVector3::_bind_methods() {
}

void EditorPropertyVector3::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix, bool p_angle_in_radians) {
	angle_in_radians = p_angle_in_radians;
	for (int i = 0; i < 3; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyVector3::EditorPropertyVector3(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector_types_editing"));

	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[3] = { "x", "y", "z" };
	for (int i = 0; i < 3; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_flat(true);
		spin[i]->set_label(desc[i]);
		bc->add_child(spin[i]);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyVector3::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
}

///////////////////// VECTOR2i /////////////////////////

void EditorPropertyVector2i::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 2; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyVector2i::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Vector2i v2;
	v2.x = spin[0]->get_value();
	v2.y = spin[1]->get_value();
	emit_changed(get_edited_property(), v2, p_name);
}

void EditorPropertyVector2i::update_property() {
	Vector2i val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.x);
	spin[1]->set_value(val.y);
	setting = false;
}

void EditorPropertyVector2i::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 2; i++) {
			spin[i]->set_custom_label_color(true, colors[i]);
		}
	}
}

void EditorPropertyVector2i::_bind_methods() {
}

void EditorPropertyVector2i::setup(int p_min, int p_max, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 2; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(1);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyVector2i::EditorPropertyVector2i(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector2_editing"));

	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[2] = { "x", "y" };
	for (int i = 0; i < 2; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_flat(true);
		spin[i]->set_label(desc[i]);
		bc->add_child(spin[i]);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyVector2i::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// RECT2i /////////////////////////

void EditorPropertyRect2i::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyRect2i::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Rect2i r2;
	r2.position.x = spin[0]->get_value();
	r2.position.y = spin[1]->get_value();
	r2.size.x = spin[2]->get_value();
	r2.size.y = spin[3]->get_value();
	emit_changed(get_edited_property(), r2, p_name);
}

void EditorPropertyRect2i::update_property() {
	Rect2i val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.position.x);
	spin[1]->set_value(val.position.y);
	spin[2]->set_value(val.size.x);
	spin[3]->set_value(val.size.y);
	setting = false;
}

void EditorPropertyRect2i::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->set_custom_label_color(true, colors[i % 2]);
		}
	}
}

void EditorPropertyRect2i::_bind_methods() {
}

void EditorPropertyRect2i::setup(int p_min, int p_max, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(1);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyRect2i::EditorPropertyRect2i(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector_types_editing"));
	bool grid = false;
	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(VBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);

		bc->add_child(memnew(HBoxContainer));
		bc->add_child(memnew(HBoxContainer));
		grid = true;
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[4] = { "x", "y", "w", "h" };
	for (int i = 0; i < 4; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_label(desc[i]);
		spin[i]->set_flat(true);

		if (grid) {
			bc->get_child(i / 2)->add_child(spin[i]);
		} else {
			bc->add_child(spin[i]);
		}

		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyRect2i::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// VECTOR3i /////////////////////////

void EditorPropertyVector3i::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 3; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyVector3i::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Vector3i v3;
	v3.x = spin[0]->get_value();
	v3.y = spin[1]->get_value();
	v3.z = spin[2]->get_value();
	emit_changed(get_edited_property(), v3, p_name);
}

void EditorPropertyVector3i::update_property() {
	Vector3i val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.x);
	spin[1]->set_value(val.y);
	spin[2]->set_value(val.z);
	setting = false;
}

void EditorPropertyVector3i::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 3; i++) {
			spin[i]->set_custom_label_color(true, colors[i]);
		}
	}
}

void EditorPropertyVector3i::_bind_methods() {
}

void EditorPropertyVector3i::setup(int p_min, int p_max, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 3; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(1);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyVector3i::EditorPropertyVector3i(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector_types_editing"));

	BoxContainer *bc;
	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[3] = { "x", "y", "z" };
	for (int i = 0; i < 3; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_flat(true);
		spin[i]->set_label(desc[i]);
		bc->add_child(spin[i]);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyVector3i::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// PLANE /////////////////////////

void EditorPropertyPlane::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyPlane::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Plane p;
	p.normal.x = spin[0]->get_value();
	p.normal.y = spin[1]->get_value();
	p.normal.z = spin[2]->get_value();
	p.d = spin[3]->get_value();
	emit_changed(get_edited_property(), p, p_name);
}

void EditorPropertyPlane::update_property() {
	Plane val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.normal.x);
	spin[1]->set_value(val.normal.y);
	spin[2]->set_value(val.normal.z);
	spin[3]->set_value(val.d);
	setting = false;
}

void EditorPropertyPlane::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->set_custom_label_color(true, colors[i]);
		}
	}
}

void EditorPropertyPlane::_bind_methods() {
}

void EditorPropertyPlane::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyPlane::EditorPropertyPlane(bool p_force_wide) {
	bool horizontal = p_force_wide || bool(EDITOR_GET("interface/inspector/horizontal_vector_types_editing"));

	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[4] = { "x", "y", "z", "d" };
	for (int i = 0; i < 4; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_flat(true);
		spin[i]->set_label(desc[i]);
		bc->add_child(spin[i]);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyPlane::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// QUATERNION /////////////////////////

void EditorPropertyQuaternion::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyQuaternion::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Quaternion p;
	p.x = spin[0]->get_value();
	p.y = spin[1]->get_value();
	p.z = spin[2]->get_value();
	p.w = spin[3]->get_value();
	emit_changed(get_edited_property(), p, p_name);
}

void EditorPropertyQuaternion::update_property() {
	Quaternion val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.x);
	spin[1]->set_value(val.y);
	spin[2]->set_value(val.z);
	spin[3]->set_value(val.w);
	setting = false;
}

void EditorPropertyQuaternion::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->set_custom_label_color(true, colors[i]);
		}
	}
}

void EditorPropertyQuaternion::_bind_methods() {
}

void EditorPropertyQuaternion::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyQuaternion::EditorPropertyQuaternion() {
	bool horizontal = EDITOR_GET("interface/inspector/horizontal_vector_types_editing");

	BoxContainer *bc;

	if (horizontal) {
		bc = memnew(HBoxContainer);
		add_child(bc);
		set_bottom_editor(bc);
	} else {
		bc = memnew(VBoxContainer);
		add_child(bc);
	}

	static const char *desc[4] = { "x", "y", "z", "w" };
	for (int i = 0; i < 4; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_flat(true);
		spin[i]->set_label(desc[i]);
		bc->add_child(spin[i]);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyQuaternion::_value_changed), varray(desc[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
	}

	if (!horizontal) {
		set_label_reference(spin[0]); //show text and buttons around this
	}
	setting = false;
}

///////////////////// AABB /////////////////////////

void EditorPropertyAABB::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 6; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyAABB::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	AABB p;
	p.position.x = spin[0]->get_value();
	p.position.y = spin[1]->get_value();
	p.position.z = spin[2]->get_value();
	p.size.x = spin[3]->get_value();
	p.size.y = spin[4]->get_value();
	p.size.z = spin[5]->get_value();

	emit_changed(get_edited_property(), p, p_name);
}

void EditorPropertyAABB::update_property() {
	AABB val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val.position.x);
	spin[1]->set_value(val.position.y);
	spin[2]->set_value(val.position.z);
	spin[3]->set_value(val.size.x);
	spin[4]->set_value(val.size.y);
	spin[5]->set_value(val.size.z);

	setting = false;
}

void EditorPropertyAABB::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 6; i++) {
			spin[i]->set_custom_label_color(true, colors[i % 3]);
		}
	}
}

void EditorPropertyAABB::_bind_methods() {
}

void EditorPropertyAABB::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 6; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyAABB::EditorPropertyAABB() {
	GridContainer *g = memnew(GridContainer);
	g->set_columns(3);
	add_child(g);

	static const char *desc[6] = { "x", "y", "z", "w", "h", "d" };
	for (int i = 0; i < 6; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_label(desc[i]);
		spin[i]->set_flat(true);

		g->add_child(spin[i]);
		spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyAABB::_value_changed), varray(desc[i]));
	}
	set_bottom_editor(g);
	setting = false;
}

///////////////////// TRANSFORM2D /////////////////////////

void EditorPropertyTransform2D::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 6; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyTransform2D::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Transform2D p;
	p[0][0] = spin[0]->get_value();
	p[1][0] = spin[1]->get_value();
	p[2][0] = spin[2]->get_value();
	p[0][1] = spin[3]->get_value();
	p[1][1] = spin[4]->get_value();
	p[2][1] = spin[5]->get_value();

	emit_changed(get_edited_property(), p, p_name);
}

void EditorPropertyTransform2D::update_property() {
	Transform2D val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val[0][0]);
	spin[1]->set_value(val[1][0]);
	spin[2]->set_value(val[2][0]);
	spin[3]->set_value(val[0][1]);
	spin[4]->set_value(val[1][1]);
	spin[5]->set_value(val[2][1]);

	setting = false;
}

void EditorPropertyTransform2D::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 6; i++) {
			// For Transform2D, use the 4th color (cyan) for the origin vector.
			if (i % 3 == 2) {
				spin[i]->set_custom_label_color(true, colors[3]);
			} else {
				spin[i]->set_custom_label_color(true, colors[i % 3]);
			}
		}
	}
}

void EditorPropertyTransform2D::_bind_methods() {
}

void EditorPropertyTransform2D::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 6; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyTransform2D::EditorPropertyTransform2D(bool p_include_origin) {
	GridContainer *g = memnew(GridContainer);
	g->set_columns(p_include_origin ? 3 : 2);
	add_child(g);

	static const char *desc[6] = { "xx", "xy", "xo", "yx", "yy", "yo" };
	for (int i = 0; i < 6; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_label(desc[i]);
		spin[i]->set_flat(true);
		if (p_include_origin || i % 3 != 2) {
			g->add_child(spin[i]);
		}
		spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyTransform2D::_value_changed), varray(desc[i]));
	}
	set_bottom_editor(g);
	setting = false;
}

///////////////////// BASIS /////////////////////////

void EditorPropertyBasis::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 9; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyBasis::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Basis p;
	p[0][0] = spin[0]->get_value();
	p[0][1] = spin[1]->get_value();
	p[0][2] = spin[2]->get_value();
	p[1][0] = spin[3]->get_value();
	p[1][1] = spin[4]->get_value();
	p[1][2] = spin[5]->get_value();
	p[2][0] = spin[6]->get_value();
	p[2][1] = spin[7]->get_value();
	p[2][2] = spin[8]->get_value();

	emit_changed(get_edited_property(), p, p_name);
}

void EditorPropertyBasis::update_property() {
	Basis val = get_edited_object()->get(get_edited_property());
	setting = true;
	spin[0]->set_value(val[0][0]);
	spin[1]->set_value(val[0][1]);
	spin[2]->set_value(val[0][2]);
	spin[3]->set_value(val[1][0]);
	spin[4]->set_value(val[1][1]);
	spin[5]->set_value(val[1][2]);
	spin[6]->set_value(val[2][0]);
	spin[7]->set_value(val[2][1]);
	spin[8]->set_value(val[2][2]);

	setting = false;
}

void EditorPropertyBasis::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 9; i++) {
			spin[i]->set_custom_label_color(true, colors[i % 3]);
		}
	}
}

void EditorPropertyBasis::_bind_methods() {
}

void EditorPropertyBasis::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 9; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyBasis::EditorPropertyBasis() {
	GridContainer *g = memnew(GridContainer);
	g->set_columns(3);
	add_child(g);

	static const char *desc[9] = { "xx", "xy", "xz", "yx", "yy", "yz", "zx", "zy", "zz" };
	for (int i = 0; i < 9; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_label(desc[i]);
		spin[i]->set_flat(true);
		g->add_child(spin[i]);
		spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyBasis::_value_changed), varray(desc[i]));
	}
	set_bottom_editor(g);
	setting = false;
}

///////////////////// TRANSFORM /////////////////////////

void EditorPropertyTransform3D::_set_read_only(bool p_read_only) {
	for (int i = 0; i < 12; i++) {
		spin[i]->set_read_only(p_read_only);
	}
};

void EditorPropertyTransform3D::_value_changed(double val, const String &p_name) {
	if (setting) {
		return;
	}

	Transform3D p;
	p.basis[0][0] = spin[0]->get_value();
	p.basis[0][1] = spin[1]->get_value();
	p.basis[0][2] = spin[2]->get_value();
	p.origin[0] = spin[3]->get_value();
	p.basis[1][0] = spin[4]->get_value();
	p.basis[1][1] = spin[5]->get_value();
	p.basis[1][2] = spin[6]->get_value();
	p.origin[1] = spin[7]->get_value();
	p.basis[2][0] = spin[8]->get_value();
	p.basis[2][1] = spin[9]->get_value();
	p.basis[2][2] = spin[10]->get_value();
	p.origin[2] = spin[11]->get_value();

	emit_changed(get_edited_property(), p, p_name);
}

void EditorPropertyTransform3D::update_property() {
	update_using_transform(get_edited_object()->get(get_edited_property()));
}

void EditorPropertyTransform3D::update_using_transform(Transform3D p_transform) {
	setting = true;
	spin[0]->set_value(p_transform.basis[0][0]);
	spin[1]->set_value(p_transform.basis[0][1]);
	spin[2]->set_value(p_transform.basis[0][2]);
	spin[3]->set_value(p_transform.origin[0]);
	spin[4]->set_value(p_transform.basis[1][0]);
	spin[5]->set_value(p_transform.basis[1][1]);
	spin[6]->set_value(p_transform.basis[1][2]);
	spin[7]->set_value(p_transform.origin[1]);
	spin[8]->set_value(p_transform.basis[2][0]);
	spin[9]->set_value(p_transform.basis[2][1]);
	spin[10]->set_value(p_transform.basis[2][2]);
	spin[11]->set_value(p_transform.origin[2]);
	setting = false;
}

void EditorPropertyTransform3D::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		const Color *colors = _get_property_colors();
		for (int i = 0; i < 12; i++) {
			spin[i]->set_custom_label_color(true, colors[i % 4]);
		}
	}
}

void EditorPropertyTransform3D::_bind_methods() {
}

void EditorPropertyTransform3D::setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix) {
	for (int i = 0; i < 12; i++) {
		spin[i]->set_min(p_min);
		spin[i]->set_max(p_max);
		spin[i]->set_step(p_step);
		spin[i]->set_hide_slider(p_no_slider);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_suffix);
	}
}

EditorPropertyTransform3D::EditorPropertyTransform3D() {
	GridContainer *g = memnew(GridContainer);
	g->set_columns(4);
	add_child(g);

	static const char *desc[12] = { "xx", "xy", "xz", "xo", "yx", "yy", "yz", "yo", "zx", "zy", "zz", "zo" };
	for (int i = 0; i < 12; i++) {
		spin[i] = memnew(EditorSpinSlider);
		spin[i]->set_label(desc[i]);
		spin[i]->set_flat(true);
		g->add_child(spin[i]);
		spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		add_focusable(spin[i]);
		spin[i]->connect("value_changed", callable_mp(this, &EditorPropertyTransform3D::_value_changed), varray(desc[i]));
	}
	set_bottom_editor(g);
	setting = false;
}

////////////// COLOR PICKER //////////////////////

void EditorPropertyColor::_set_read_only(bool p_read_only) {
	picker->set_disabled(p_read_only);
};

void EditorPropertyColor::_color_changed(const Color &p_color) {
	// Cancel the color change if the current color is identical to the new one.
	if (get_edited_object()->get(get_edited_property()) == p_color) {
		return;
	}

	emit_changed(get_edited_property(), p_color, "", true);
}

void EditorPropertyColor::_popup_closed() {
	if (picker->get_pick_color() != last_color) {
		emit_changed(get_edited_property(), picker->get_pick_color(), "", false);
	}
}

void EditorPropertyColor::_picker_created() {
	// get default color picker mode from editor settings
	int default_color_mode = EDITOR_GET("interface/inspector/default_color_picker_mode");
	if (default_color_mode == 1) {
		picker->get_picker()->set_hsv_mode(true);
	} else if (default_color_mode == 2) {
		picker->get_picker()->set_raw_mode(true);
	}

	int picker_shape = EDITOR_GET("interface/inspector/default_color_picker_shape");
	picker->get_picker()->set_picker_shape((ColorPicker::PickerShapeType)picker_shape);
}

void EditorPropertyColor::_picker_opening() {
	last_color = picker->get_pick_color();
}

void EditorPropertyColor::_bind_methods() {
}

void EditorPropertyColor::update_property() {
	picker->set_pick_color(get_edited_object()->get(get_edited_property()));
	const Color color = picker->get_pick_color();

	// Add a tooltip to display each channel's values without having to click the ColorPickerButton
	if (picker->is_editing_alpha()) {
		picker->set_tooltip(vformat(
				"R: %s\nG: %s\nB: %s\nA: %s",
				rtos(color.r).pad_decimals(2),
				rtos(color.g).pad_decimals(2),
				rtos(color.b).pad_decimals(2),
				rtos(color.a).pad_decimals(2)));
	} else {
		picker->set_tooltip(vformat(
				"R: %s\nG: %s\nB: %s",
				rtos(color.r).pad_decimals(2),
				rtos(color.g).pad_decimals(2),
				rtos(color.b).pad_decimals(2)));
	}
}

void EditorPropertyColor::setup(bool p_show_alpha) {
	picker->set_edit_alpha(p_show_alpha);
}

EditorPropertyColor::EditorPropertyColor() {
	picker = memnew(ColorPickerButton);
	add_child(picker);
	picker->set_flat(true);
	picker->connect("color_changed", callable_mp(this, &EditorPropertyColor::_color_changed));
	picker->connect("popup_closed", callable_mp(this, &EditorPropertyColor::_popup_closed));
	picker->connect("picker_created", callable_mp(this, &EditorPropertyColor::_picker_created));
	picker->get_popup()->connect("about_to_popup", callable_mp(this, &EditorPropertyColor::_picker_opening));
}

////////////// NODE PATH //////////////////////

void EditorPropertyNodePath::_set_read_only(bool p_read_only) {
	assign->set_disabled(p_read_only);
	clear->set_disabled(p_read_only);
};

void EditorPropertyNodePath::_node_selected(const NodePath &p_path) {
	NodePath path = p_path;
	Node *base_node = nullptr;

	if (!use_path_from_scene_root) {
		base_node = Object::cast_to<Node>(get_edited_object());

		if (!base_node) {
			//try a base node within history
			if (EditorNode::get_singleton()->get_editor_history()->get_path_size() > 0) {
				Object *base = ObjectDB::get_instance(EditorNode::get_singleton()->get_editor_history()->get_path_object(0));
				if (base) {
					base_node = Object::cast_to<Node>(base);
				}
			}
		}
	}

	if (!base_node && get_edited_object()->has_method("get_root_path")) {
		base_node = get_edited_object()->call("get_root_path");
	}

	if (!base_node && Object::cast_to<RefCounted>(get_edited_object())) {
		Node *to_node = get_node(p_path);
		ERR_FAIL_COND(!to_node);
		path = get_tree()->get_edited_scene_root()->get_path_to(to_node);
	}

	if (base_node) { // for AnimationTrackKeyEdit
		path = base_node->get_path().rel_path_to(p_path);
	}
	emit_changed(get_edited_property(), path);
	update_property();
}

void EditorPropertyNodePath::_node_assign() {
	if (!scene_tree) {
		scene_tree = memnew(SceneTreeDialog);
		scene_tree->get_scene_tree()->set_show_enabled_subscene(true);
		scene_tree->get_scene_tree()->set_valid_types(valid_types);
		add_child(scene_tree);
		scene_tree->connect("selected", callable_mp(this, &EditorPropertyNodePath::_node_selected));
	}
	scene_tree->popup_scenetree_dialog();
}

void EditorPropertyNodePath::_node_clear() {
	emit_changed(get_edited_property(), NodePath());
	update_property();
}

void EditorPropertyNodePath::update_property() {
	NodePath p = get_edited_object()->get(get_edited_property());

	assign->set_tooltip(p);
	if (p == NodePath()) {
		assign->set_icon(Ref<Texture2D>());
		assign->set_text(TTR("Assign..."));
		assign->set_flat(false);
		return;
	}
	assign->set_flat(true);

	Node *base_node = nullptr;
	if (base_hint != NodePath()) {
		if (get_tree()->get_root()->has_node(base_hint)) {
			base_node = get_tree()->get_root()->get_node(base_hint);
		}
	} else {
		base_node = Object::cast_to<Node>(get_edited_object());
	}

	if (!base_node || !base_node->has_node(p)) {
		assign->set_icon(Ref<Texture2D>());
		assign->set_text(p);
		return;
	}

	Node *target_node = base_node->get_node(p);
	ERR_FAIL_COND(!target_node);

	if (String(target_node->get_name()).find("@") != -1) {
		assign->set_icon(Ref<Texture2D>());
		assign->set_text(p);
		return;
	}

	assign->set_text(target_node->get_name());
	assign->set_icon(EditorNode::get_singleton()->get_object_icon(target_node, "Node"));
}

void EditorPropertyNodePath::setup(const NodePath &p_base_hint, Vector<StringName> p_valid_types, bool p_use_path_from_scene_root) {
	base_hint = p_base_hint;
	valid_types = p_valid_types;
	use_path_from_scene_root = p_use_path_from_scene_root;
}

void EditorPropertyNodePath::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		Ref<Texture2D> t = get_theme_icon(SNAME("Clear"), SNAME("EditorIcons"));
		clear->set_icon(t);
	}
}

void EditorPropertyNodePath::_bind_methods() {
}

EditorPropertyNodePath::EditorPropertyNodePath() {
	HBoxContainer *hbc = memnew(HBoxContainer);
	add_child(hbc);
	assign = memnew(Button);
	assign->set_flat(true);
	assign->set_h_size_flags(SIZE_EXPAND_FILL);
	assign->set_clip_text(true);
	assign->connect("pressed", callable_mp(this, &EditorPropertyNodePath::_node_assign));
	hbc->add_child(assign);

	clear = memnew(Button);
	clear->set_flat(true);
	clear->connect("pressed", callable_mp(this, &EditorPropertyNodePath::_node_clear));
	hbc->add_child(clear);
	use_path_from_scene_root = false;

	scene_tree = nullptr; //do not allocate unnecessarily
}

///////////////////// RID /////////////////////////

void EditorPropertyRID::update_property() {
	RID rid = get_edited_object()->get(get_edited_property());
	if (rid.is_valid()) {
		int id = rid.get_id();
		label->set_text("RID: " + itos(id));
	} else {
		label->set_text(TTR("Invalid RID"));
	}
}

EditorPropertyRID::EditorPropertyRID() {
	label = memnew(Label);
	add_child(label);
}

////////////// RESOURCE //////////////////////

void EditorPropertyResource::_set_read_only(bool p_read_only) {
	resource_picker->set_editable(!p_read_only);
};

void EditorPropertyResource::_resource_selected(const RES &p_resource, bool p_edit) {
	if (!p_edit && use_sub_inspector) {
		bool unfold = !get_edited_object()->editor_is_section_unfolded(get_edited_property());
		get_edited_object()->editor_set_section_unfold(get_edited_property(), unfold);
		update_property();
	} else {
		emit_signal(SNAME("resource_selected"), get_edited_property(), p_resource);
	}
}

void EditorPropertyResource::_resource_changed(const RES &p_resource) {
	// Make visual script the correct type.
	Ref<Script> s = p_resource;
	if (get_edited_object() && s.is_valid()) {
		s->call("set_instance_base_type", get_edited_object()->get_class());
	}

	// Prevent the creation of invalid ViewportTextures when possible.
	Ref<ViewportTexture> vpt = p_resource;
	if (vpt.is_valid()) {
		Resource *r = Object::cast_to<Resource>(get_edited_object());
		if (r && r->get_path().is_resource_file()) {
			EditorNode::get_singleton()->show_warning(TTR("Can't create a ViewportTexture on resources saved as a file.\nResource needs to belong to a scene."));
			emit_changed(get_edited_property(), RES());
			update_property();
			return;
		}

		if (r && !r->is_local_to_scene()) {
			EditorNode::get_singleton()->show_warning(TTR("Can't create a ViewportTexture on this resource because it's not set as local to scene.\nPlease switch on the 'local to scene' property on it (and all resources containing it up to a node)."));
			emit_changed(get_edited_property(), RES());
			update_property();
			return;
		}
	}

	emit_changed(get_edited_property(), p_resource);
	update_property();

	// Automatically suggest setting up the path for a ViewportTexture.
	if (vpt.is_valid() && vpt->get_viewport_path_in_scene().is_empty()) {
		if (!scene_tree) {
			scene_tree = memnew(SceneTreeDialog);
			scene_tree->set_title(TTR("Pick a Viewport"));

			Vector<StringName> valid_types;
			valid_types.push_back("Viewport");
			scene_tree->get_scene_tree()->set_valid_types(valid_types);
			scene_tree->get_scene_tree()->set_show_enabled_subscene(true);

			add_child(scene_tree);
			scene_tree->connect("selected", callable_mp(this, &EditorPropertyResource::_viewport_selected));
		}

		scene_tree->popup_scenetree_dialog();
	}
}

void EditorPropertyResource::_sub_inspector_property_keyed(const String &p_property, const Variant &p_value, bool) {
	emit_signal(SNAME("property_keyed_with_value"), String(get_edited_property()) + ":" + p_property, p_value, false);
}

void EditorPropertyResource::_sub_inspector_resource_selected(const RES &p_resource, const String &p_property) {
	emit_signal(SNAME("resource_selected"), String(get_edited_property()) + ":" + p_property, p_resource);
}

void EditorPropertyResource::_sub_inspector_object_id_selected(int p_id) {
	emit_signal(SNAME("object_id_selected"), get_edited_property(), p_id);
}

void EditorPropertyResource::_open_editor_pressed() {
	RES res = get_edited_object()->get(get_edited_property());
	if (res.is_valid()) {
		// May clear the editor so do it deferred.
		EditorNode::get_singleton()->call_deferred(SNAME("edit_item_resource"), res);
	}
}

void EditorPropertyResource::_fold_other_editors(Object *p_self) {
	if (this == p_self) {
		return;
	}

	RES res = get_edited_object()->get(get_edited_property());
	if (!res.is_valid()) {
		return;
	}

	bool use_editor = false;
	for (int i = 0; i < EditorNode::get_editor_data().get_editor_plugin_count(); i++) {
		EditorPlugin *ep = EditorNode::get_editor_data().get_editor_plugin(i);
		if (ep->handles(res.ptr())) {
			use_editor = true;
			break;
		}
	}
	if (!use_editor) {
		return;
	}

	opened_editor = false;

	bool unfolded = get_edited_object()->editor_is_section_unfolded(get_edited_property());
	if (unfolded) {
		// Refold.
		resource_picker->set_toggle_pressed(false);
		get_edited_object()->editor_set_section_unfold(get_edited_property(), false);
		update_property();
	}
}

void EditorPropertyResource::_update_property_bg() {
	if (!is_inside_tree()) {
		return;
	}

	updating_theme = true;

	if (sub_inspector != nullptr) {
		int count_subinspectors = 0;
		Node *n = get_parent();
		while (n) {
			EditorInspector *ei = Object::cast_to<EditorInspector>(n);
			if (ei && ei->is_sub_inspector()) {
				count_subinspectors++;
			}
			n = n->get_parent();
		}
		count_subinspectors = MIN(15, count_subinspectors);

		add_theme_color_override("property_color", get_theme_color(SNAME("sub_inspector_property_color"), SNAME("Editor")));
		add_theme_style_override("bg_selected", get_theme_stylebox("sub_inspector_property_bg_selected" + itos(count_subinspectors), SNAME("Editor")));
		add_theme_style_override("bg", get_theme_stylebox("sub_inspector_property_bg" + itos(count_subinspectors), SNAME("Editor")));

		add_theme_constant_override("font_offset", get_theme_constant(SNAME("sub_inspector_font_offset"), SNAME("Editor")));
		add_theme_constant_override("vseparation", 0);
	} else {
		add_theme_color_override("property_color", get_theme_color(SNAME("property_color"), SNAME("EditorProperty")));
		add_theme_style_override("bg_selected", get_theme_stylebox(SNAME("bg_selected"), SNAME("EditorProperty")));
		add_theme_style_override("bg", get_theme_stylebox(SNAME("bg"), SNAME("EditorProperty")));
		add_theme_constant_override("vseparation", get_theme_constant(SNAME("vseparation"), SNAME("EditorProperty")));
		add_theme_constant_override("font_offset", get_theme_constant(SNAME("font_offset"), SNAME("EditorProperty")));
	}

	updating_theme = false;
	update();
}

void EditorPropertyResource::_update_preferred_shader() {
	Node *parent = get_parent();
	EditorProperty *parent_property = nullptr;

	while (parent && !parent_property) {
		parent_property = Object::cast_to<EditorProperty>(parent);
		parent = parent->get_parent();
	}

	if (parent_property) {
		EditorShaderPicker *shader_picker = Object::cast_to<EditorShaderPicker>(resource_picker);
		Object *object = parent_property->get_edited_object();
		const StringName &property = parent_property->get_edited_property();

		// Set preferred shader based on edited parent type.
		if ((Object::cast_to<GPUParticles2D>(object) || Object::cast_to<GPUParticles3D>(object)) && property == SNAME("process_material")) {
			shader_picker->set_preferred_mode(Shader::MODE_PARTICLES);
		} else if (Object::cast_to<FogVolume>(object)) {
			shader_picker->set_preferred_mode(Shader::MODE_FOG);
		} else if (Object::cast_to<CanvasItem>(object)) {
			shader_picker->set_preferred_mode(Shader::MODE_CANVAS_ITEM);
		} else if (Object::cast_to<Node3D>(object)) {
			shader_picker->set_preferred_mode(Shader::MODE_SPATIAL);
		} else if (Object::cast_to<Sky>(object)) {
			shader_picker->set_preferred_mode(Shader::MODE_SKY);
		}
	}
}

void EditorPropertyResource::_viewport_selected(const NodePath &p_path) {
	Node *to_node = get_node(p_path);
	if (!Object::cast_to<Viewport>(to_node)) {
		EditorNode::get_singleton()->show_warning(TTR("Selected node is not a Viewport!"));
		return;
	}

	Ref<ViewportTexture> vt;
	vt.instantiate();
	vt->set_viewport_path_in_scene(get_tree()->get_edited_scene_root()->get_path_to(to_node));
	vt->setup_local_to_scene();

	emit_changed(get_edited_property(), vt);
	update_property();
}

void EditorPropertyResource::setup(Object *p_object, const String &p_path, const String &p_base_type) {
	if (resource_picker) {
		resource_picker->disconnect("resource_selected", callable_mp(this, &EditorPropertyResource::_resource_selected));
		resource_picker->disconnect("resource_changed", callable_mp(this, &EditorPropertyResource::_resource_changed));
		memdelete(resource_picker);
	}

	if (p_path == "script" && p_base_type == "Script" && Object::cast_to<Node>(p_object)) {
		EditorScriptPicker *script_picker = memnew(EditorScriptPicker);
		script_picker->set_script_owner(Object::cast_to<Node>(p_object));
		resource_picker = script_picker;
	} else if (p_path == "shader" && p_base_type == "Shader" && Object::cast_to<ShaderMaterial>(p_object)) {
		EditorShaderPicker *shader_picker = memnew(EditorShaderPicker);
		shader_picker->set_edited_material(Object::cast_to<ShaderMaterial>(p_object));
		resource_picker = shader_picker;
		connect(SNAME("ready"), callable_mp(this, &EditorPropertyResource::_update_preferred_shader));
	} else {
		resource_picker = memnew(EditorResourcePicker);
	}

	resource_picker->set_base_type(p_base_type);
	resource_picker->set_editable(true);
	resource_picker->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(resource_picker);

	resource_picker->connect("resource_selected", callable_mp(this, &EditorPropertyResource::_resource_selected));
	resource_picker->connect("resource_changed", callable_mp(this, &EditorPropertyResource::_resource_changed));

	for (int i = 0; i < resource_picker->get_child_count(); i++) {
		Button *b = Object::cast_to<Button>(resource_picker->get_child(i));
		if (b) {
			add_focusable(b);
		}
	}
}

void EditorPropertyResource::update_property() {
	RES res = get_edited_object()->get(get_edited_property());

	if (use_sub_inspector) {
		if (res.is_valid() != resource_picker->is_toggle_mode()) {
			resource_picker->set_toggle_mode(res.is_valid());
		}

		if (res.is_valid() && get_edited_object()->editor_is_section_unfolded(get_edited_property())) {
			if (!sub_inspector) {
				sub_inspector = memnew(EditorInspector);
				sub_inspector->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
				sub_inspector->set_use_doc_hints(true);

				sub_inspector->set_sub_inspector(true);
				sub_inspector->set_enable_capitalize_paths(bool(EDITOR_GET("interface/inspector/capitalize_properties")));

				sub_inspector->connect("property_keyed", callable_mp(this, &EditorPropertyResource::_sub_inspector_property_keyed));
				sub_inspector->connect("resource_selected", callable_mp(this, &EditorPropertyResource::_sub_inspector_resource_selected));
				sub_inspector->connect("object_id_selected", callable_mp(this, &EditorPropertyResource::_sub_inspector_object_id_selected));
				sub_inspector->set_keying(is_keying());
				sub_inspector->set_read_only(is_read_only());
				sub_inspector->set_use_folding(is_using_folding());
				sub_inspector->set_undo_redo(EditorNode::get_undo_redo());

				sub_inspector_vbox = memnew(VBoxContainer);
				add_child(sub_inspector_vbox);
				set_bottom_editor(sub_inspector_vbox);

				sub_inspector_vbox->add_child(sub_inspector);
				resource_picker->set_toggle_pressed(true);

				bool use_editor = false;
				for (int i = 0; i < EditorNode::get_editor_data().get_editor_plugin_count(); i++) {
					EditorPlugin *ep = EditorNode::get_editor_data().get_editor_plugin(i);
					if (ep->handles(res.ptr())) {
						use_editor = true;
					}
				}

				if (use_editor) {
					// Open editor directly and hide other such editors which are currently open.
					_open_editor_pressed();
					if (is_inside_tree()) {
						get_tree()->call_deferred(SNAME("call_group"), "_editor_resource_properties", "_fold_other_editors", this);
					}
					opened_editor = true;
				}

				_update_property_bg();
			}

			if (res.ptr() != sub_inspector->get_edited_object()) {
				sub_inspector->edit(res.ptr());
			}

		} else {
			if (sub_inspector) {
				set_bottom_editor(nullptr);
				memdelete(sub_inspector_vbox);
				sub_inspector = nullptr;
				sub_inspector_vbox = nullptr;

				if (opened_editor) {
					EditorNode::get_singleton()->hide_top_editors();
					opened_editor = false;
				}

				_update_property_bg();
			}
		}
	}

	resource_picker->set_edited_resource(res);
}

void EditorPropertyResource::collapse_all_folding() {
	if (sub_inspector) {
		sub_inspector->collapse_all_folding();
	}
}

void EditorPropertyResource::expand_all_folding() {
	if (sub_inspector) {
		sub_inspector->expand_all_folding();
	}
}

void EditorPropertyResource::set_use_sub_inspector(bool p_enable) {
	use_sub_inspector = p_enable;
}

void EditorPropertyResource::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			if (!updating_theme) {
				_update_property_bg();
			}
		} break;
	}
}

void EditorPropertyResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_open_editor_pressed"), &EditorPropertyResource::_open_editor_pressed);
	ClassDB::bind_method(D_METHOD("_fold_other_editors"), &EditorPropertyResource::_fold_other_editors);
}

EditorPropertyResource::EditorPropertyResource() {
	use_sub_inspector = bool(EDITOR_GET("interface/inspector/open_resources_in_current_inspector"));

	add_to_group("_editor_resource_properties");
}

////////////// DEFAULT PLUGIN //////////////////////

bool EditorInspectorDefaultPlugin::can_handle(Object *p_object) {
	return true; // Can handle everything.
}

bool EditorInspectorDefaultPlugin::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const uint32_t p_usage, const bool p_wide) {
	Control *editor = EditorInspectorDefaultPlugin::get_editor_for_property(p_object, p_type, p_path, p_hint, p_hint_text, p_usage, p_wide);
	if (editor) {
		add_property_editor(p_path, editor);
	}
	return false;
}

struct EditorPropertyRangeHint {
	bool angle_in_degrees = false;
	bool greater = true;
	bool lesser = true;
	double min = -99999;
	double max = 99999;
	double step = 0;
	String suffix;
	bool exp_range = false;
	bool hide_slider = true;
	bool radians = false;
};

static EditorPropertyRangeHint _parse_range_hint(PropertyHint p_hint, const String &p_hint_text, double p_default_step) {
	EditorPropertyRangeHint hint;
	hint.step = p_default_step;
	bool degrees = false;
	if (p_hint == PROPERTY_HINT_RANGE && p_hint_text.get_slice_count(",") >= 2) {
		hint.greater = false; //if using ranged, assume false by default
		hint.lesser = false;

		hint.min = p_hint_text.get_slice(",", 0).to_float();
		hint.max = p_hint_text.get_slice(",", 1).to_float();
		if (p_hint_text.get_slice_count(",") >= 3) {
			hint.step = p_hint_text.get_slice(",", 2).to_float();
		}
		hint.hide_slider = false;
		for (int i = 2; i < p_hint_text.get_slice_count(","); i++) {
			String slice = p_hint_text.get_slice(",", i).strip_edges();
			if (slice == "radians") {
				hint.radians = true;
			} else if (slice == "degrees") {
				degrees = true;
			} else if (slice == "or_greater") {
				hint.greater = true;
			} else if (slice == "or_lesser") {
				hint.lesser = true;
			} else if (slice == "noslider") {
				hint.hide_slider = true;
			} else if (slice == "exp") {
				hint.exp_range = true;
			} else if (slice.begins_with("suffix:")) {
				hint.suffix = " " + slice.replace_first("suffix:", "").strip_edges();
			}
		}
	}

	if ((hint.radians || degrees) && hint.suffix == String()) {
		hint.suffix = U"\u00B0";
	}

	return hint;
}

EditorProperty *EditorInspectorDefaultPlugin::get_editor_for_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const uint32_t p_usage, const bool p_wide) {
	double default_float_step = EDITOR_GET("interface/inspector/default_float_step");

	switch (p_type) {
		// atomic types
		case Variant::NIL: {
			EditorPropertyNil *editor = memnew(EditorPropertyNil);
			return editor;
		} break;
		case Variant::BOOL: {
			EditorPropertyCheck *editor = memnew(EditorPropertyCheck);
			return editor;
		} break;
		case Variant::INT: {
			if (p_hint == PROPERTY_HINT_ENUM) {
				EditorPropertyEnum *editor = memnew(EditorPropertyEnum);
				Vector<String> options = p_hint_text.split(",");
				editor->setup(options);
				return editor;

			} else if (p_hint == PROPERTY_HINT_FLAGS) {
				EditorPropertyFlags *editor = memnew(EditorPropertyFlags);
				Vector<String> options = p_hint_text.split(",");
				editor->setup(options);
				return editor;

			} else if (p_hint == PROPERTY_HINT_LAYERS_2D_PHYSICS ||
					p_hint == PROPERTY_HINT_LAYERS_2D_RENDER ||
					p_hint == PROPERTY_HINT_LAYERS_2D_NAVIGATION ||
					p_hint == PROPERTY_HINT_LAYERS_3D_PHYSICS ||
					p_hint == PROPERTY_HINT_LAYERS_3D_RENDER ||
					p_hint == PROPERTY_HINT_LAYERS_3D_NAVIGATION) {
				EditorPropertyLayers::LayerType lt = EditorPropertyLayers::LAYER_RENDER_2D;
				switch (p_hint) {
					case PROPERTY_HINT_LAYERS_2D_RENDER:
						lt = EditorPropertyLayers::LAYER_RENDER_2D;
						break;
					case PROPERTY_HINT_LAYERS_2D_PHYSICS:
						lt = EditorPropertyLayers::LAYER_PHYSICS_2D;
						break;
					case PROPERTY_HINT_LAYERS_2D_NAVIGATION:
						lt = EditorPropertyLayers::LAYER_NAVIGATION_2D;
						break;
					case PROPERTY_HINT_LAYERS_3D_RENDER:
						lt = EditorPropertyLayers::LAYER_RENDER_3D;
						break;
					case PROPERTY_HINT_LAYERS_3D_PHYSICS:
						lt = EditorPropertyLayers::LAYER_PHYSICS_3D;
						break;
					case PROPERTY_HINT_LAYERS_3D_NAVIGATION:
						lt = EditorPropertyLayers::LAYER_NAVIGATION_3D;
						break;
					default: {
					} //compiler could be smarter here and realize this can't happen
				}
				EditorPropertyLayers *editor = memnew(EditorPropertyLayers);
				editor->setup(lt);
				return editor;
			} else if (p_hint == PROPERTY_HINT_OBJECT_ID) {
				EditorPropertyObjectID *editor = memnew(EditorPropertyObjectID);
				editor->setup(p_hint_text);
				return editor;

			} else {
				EditorPropertyInteger *editor = memnew(EditorPropertyInteger);

				EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, 1);

				editor->setup(hint.min, hint.max, hint.step, hint.greater, hint.lesser);

				return editor;
			}
		} break;
		case Variant::FLOAT: {
			if (p_hint == PROPERTY_HINT_EXP_EASING) {
				EditorPropertyEasing *editor = memnew(EditorPropertyEasing);
				bool full = true;
				bool flip = false;
				Vector<String> hints = p_hint_text.split(",");
				for (int i = 0; i < hints.size(); i++) {
					String h = hints[i].strip_edges();
					if (h == "attenuation") {
						flip = true;
					}
					if (h == "inout") {
						full = true;
					}
				}

				editor->setup(full, flip);
				return editor;

			} else {
				EditorPropertyFloat *editor = memnew(EditorPropertyFloat);

				EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
				editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.exp_range, hint.greater, hint.lesser, hint.suffix, hint.radians);

				return editor;
			}
		} break;
		case Variant::STRING: {
			if (p_hint == PROPERTY_HINT_ENUM || p_hint == PROPERTY_HINT_ENUM_SUGGESTION) {
				EditorPropertyTextEnum *editor = memnew(EditorPropertyTextEnum);
				Vector<String> options = p_hint_text.split(",", false);
				editor->setup(options, false, (p_hint == PROPERTY_HINT_ENUM_SUGGESTION));
				return editor;
			} else if (p_hint == PROPERTY_HINT_MULTILINE_TEXT) {
				EditorPropertyMultilineText *editor = memnew(EditorPropertyMultilineText);
				return editor;
			} else if (p_hint == PROPERTY_HINT_TYPE_STRING) {
				EditorPropertyClassName *editor = memnew(EditorPropertyClassName);
				editor->setup("Object", p_hint_text);
				return editor;
			} else if (p_hint == PROPERTY_HINT_DIR || p_hint == PROPERTY_HINT_FILE || p_hint == PROPERTY_HINT_SAVE_FILE || p_hint == PROPERTY_HINT_GLOBAL_DIR || p_hint == PROPERTY_HINT_GLOBAL_FILE) {
				Vector<String> extensions = p_hint_text.split(",");
				bool global = p_hint == PROPERTY_HINT_GLOBAL_DIR || p_hint == PROPERTY_HINT_GLOBAL_FILE;
				bool folder = p_hint == PROPERTY_HINT_DIR || p_hint == PROPERTY_HINT_GLOBAL_DIR;
				bool save = p_hint == PROPERTY_HINT_SAVE_FILE;
				EditorPropertyPath *editor = memnew(EditorPropertyPath);
				editor->setup(extensions, folder, global);
				if (save) {
					editor->set_save_mode();
				}
				return editor;
			} else if (p_hint == PROPERTY_HINT_METHOD_OF_VARIANT_TYPE ||
					p_hint == PROPERTY_HINT_METHOD_OF_BASE_TYPE ||
					p_hint == PROPERTY_HINT_METHOD_OF_INSTANCE ||
					p_hint == PROPERTY_HINT_METHOD_OF_SCRIPT ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_VARIANT_TYPE ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_BASE_TYPE ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_INSTANCE ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_SCRIPT) {
				EditorPropertyMember *editor = memnew(EditorPropertyMember);

				EditorPropertyMember::Type type = EditorPropertyMember::MEMBER_METHOD_OF_BASE_TYPE;
				switch (p_hint) {
					case PROPERTY_HINT_METHOD_OF_BASE_TYPE:
						type = EditorPropertyMember::MEMBER_METHOD_OF_BASE_TYPE;
						break;
					case PROPERTY_HINT_METHOD_OF_INSTANCE:
						type = EditorPropertyMember::MEMBER_METHOD_OF_INSTANCE;
						break;
					case PROPERTY_HINT_METHOD_OF_SCRIPT:
						type = EditorPropertyMember::MEMBER_METHOD_OF_SCRIPT;
						break;
					case PROPERTY_HINT_PROPERTY_OF_VARIANT_TYPE:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_VARIANT_TYPE;
						break;
					case PROPERTY_HINT_PROPERTY_OF_BASE_TYPE:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_BASE_TYPE;
						break;
					case PROPERTY_HINT_PROPERTY_OF_INSTANCE:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_INSTANCE;
						break;
					case PROPERTY_HINT_PROPERTY_OF_SCRIPT:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_SCRIPT;
						break;
					default: {
					}
				}
				editor->setup(type, p_hint_text);
				return editor;

			} else {
				EditorPropertyText *editor = memnew(EditorPropertyText);
				if (p_hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
					editor->set_placeholder(p_hint_text);
				}
				return editor;
			}
		} break;

			// math types

		case Variant::VECTOR2: {
			EditorPropertyVector2 *editor = memnew(EditorPropertyVector2(p_wide));

			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;

		} break;
		case Variant::VECTOR2I: {
			EditorPropertyVector2i *editor = memnew(EditorPropertyVector2i(p_wide));
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, 1);
			editor->setup(hint.min, hint.max, hint.hide_slider, hint.suffix);
			return editor;

		} break;
		case Variant::RECT2: {
			EditorPropertyRect2 *editor = memnew(EditorPropertyRect2(p_wide));
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;
		} break;
		case Variant::RECT2I: {
			EditorPropertyRect2i *editor = memnew(EditorPropertyRect2i(p_wide));
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, 1);
			editor->setup(hint.min, hint.max, hint.hide_slider, hint.suffix);

			return editor;
		} break;
		case Variant::VECTOR3: {
			EditorPropertyVector3 *editor = memnew(EditorPropertyVector3(p_wide));
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix, hint.radians);
			return editor;

		} break;
		case Variant::VECTOR3I: {
			EditorPropertyVector3i *editor = memnew(EditorPropertyVector3i(p_wide));
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, 1);
			editor->setup(hint.min, hint.max, hint.hide_slider, hint.suffix);
			return editor;

		} break;
		case Variant::TRANSFORM2D: {
			EditorPropertyTransform2D *editor = memnew(EditorPropertyTransform2D);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;
		} break;
		case Variant::PLANE: {
			EditorPropertyPlane *editor = memnew(EditorPropertyPlane(p_wide));
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;
		} break;
		case Variant::QUATERNION: {
			EditorPropertyQuaternion *editor = memnew(EditorPropertyQuaternion);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;
		} break;
		case Variant::AABB: {
			EditorPropertyAABB *editor = memnew(EditorPropertyAABB);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;
		} break;
		case Variant::BASIS: {
			EditorPropertyBasis *editor = memnew(EditorPropertyBasis);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;
		} break;
		case Variant::TRANSFORM3D: {
			EditorPropertyTransform3D *editor = memnew(EditorPropertyTransform3D);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.suffix);
			return editor;

		} break;

		// misc types
		case Variant::COLOR: {
			EditorPropertyColor *editor = memnew(EditorPropertyColor);
			editor->setup(p_hint != PROPERTY_HINT_COLOR_NO_ALPHA);
			return editor;
		} break;
		case Variant::STRING_NAME: {
			if (p_hint == PROPERTY_HINT_ENUM || p_hint == PROPERTY_HINT_ENUM_SUGGESTION) {
				EditorPropertyTextEnum *editor = memnew(EditorPropertyTextEnum);
				Vector<String> options = p_hint_text.split(",", false);
				editor->setup(options, true, (p_hint == PROPERTY_HINT_ENUM_SUGGESTION));
				return editor;
			} else {
				EditorPropertyText *editor = memnew(EditorPropertyText);
				if (p_hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
					editor->set_placeholder(p_hint_text);
				}
				editor->set_string_name(true);
				return editor;
			}
		} break;
		case Variant::NODE_PATH: {
			EditorPropertyNodePath *editor = memnew(EditorPropertyNodePath);
			if (p_hint == PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE && p_hint_text != String()) {
				editor->setup(p_hint_text, Vector<StringName>(), (p_usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
			}
			if (p_hint == PROPERTY_HINT_NODE_PATH_VALID_TYPES && p_hint_text != String()) {
				Vector<String> types = p_hint_text.split(",", false);
				Vector<StringName> sn = Variant(types); //convert via variant
				editor->setup(NodePath(), sn, (p_usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
			}
			return editor;

		} break;
		case Variant::RID: {
			EditorPropertyRID *editor = memnew(EditorPropertyRID);
			return editor;
		} break;
		case Variant::OBJECT: {
			EditorPropertyResource *editor = memnew(EditorPropertyResource);
			editor->setup(p_object, p_path, p_hint == PROPERTY_HINT_RESOURCE_TYPE ? p_hint_text : "Resource");

			if (p_hint == PROPERTY_HINT_RESOURCE_TYPE) {
				String open_in_new = EDITOR_GET("interface/inspector/resources_to_open_in_new_inspector");
				for (int i = 0; i < open_in_new.get_slice_count(","); i++) {
					String type = open_in_new.get_slicec(',', i).strip_edges();
					for (int j = 0; j < p_hint_text.get_slice_count(","); j++) {
						String inherits = p_hint_text.get_slicec(',', j);
						if (ClassDB::is_parent_class(inherits, type)) {
							editor->set_use_sub_inspector(false);
						}
					}
				}
			}

			return editor;

		} break;
		case Variant::DICTIONARY: {
			EditorPropertyDictionary *editor = memnew(EditorPropertyDictionary);
			return editor;
		} break;
		case Variant::ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::ARRAY, p_hint_text);
			return editor;
		} break;
		case Variant::PACKED_BYTE_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_BYTE_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_INT32_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_INT32_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_INT64_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_INT64_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_FLOAT32_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_FLOAT32_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_FLOAT64_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_FLOAT64_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_STRING_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_STRING_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_VECTOR2_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_VECTOR2_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_VECTOR3_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_VECTOR3_ARRAY);
			return editor;
		} break;
		case Variant::PACKED_COLOR_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::PACKED_COLOR_ARRAY);
			return editor;
		} break;
		default: {
		}
	}

	return nullptr;
}
