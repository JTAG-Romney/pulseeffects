#include "blacklist_settings_ui.hpp"
#include "util.hpp"

BlacklistSettingsUi::BlacklistSettingsUi(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Grid(cobject), settings(Gio::Settings::create("com.github.wwmm.pulseeffects")) {
  // loading glade widgets

  builder->get_widget("blacklist_in_scrolled_window", blacklist_in_scrolled_window);
  builder->get_widget("blacklist_out_scrolled_window", blacklist_out_scrolled_window);
  builder->get_widget("add_blacklist_in", add_blacklist_in);
  builder->get_widget("add_blacklist_out", add_blacklist_out);
  builder->get_widget("blacklist_in_listbox", blacklist_in_listbox);
  builder->get_widget("blacklist_out_listbox", blacklist_out_listbox);
  builder->get_widget("blacklist_in_name", blacklist_in_name);
  builder->get_widget("blacklist_out_name", blacklist_out_name);

  // signals connection

  blacklist_in_listbox->set_sort_func(sigc::ptr_fun(BlacklistSettingsUi::on_listbox_sort));

  blacklist_out_listbox->set_sort_func(sigc::ptr_fun(BlacklistSettingsUi::on_listbox_sort));

  add_blacklist_in->signal_clicked().connect([=]() {
    if (add_new_entry(settings, blacklist_in_name->get_text(), PresetType::input)) {
      blacklist_in_name->set_text("");
      populate_blacklist_in_listbox();
    }
  });

  add_blacklist_out->signal_clicked().connect([=]() {
    if (add_new_entry(settings, blacklist_out_name->get_text(), PresetType::output)) {
      blacklist_out_name->set_text("");
      populate_blacklist_out_listbox();
    }
  });

  connections.emplace_back(
      settings->signal_changed("blacklist-in").connect([&](auto key) { populate_blacklist_in_listbox(); }));

  connections.emplace_back(
      settings->signal_changed("blacklist-out").connect([&](auto key) { populate_blacklist_out_listbox(); }));

  populate_blacklist_in_listbox();
  populate_blacklist_out_listbox();
}

BlacklistSettingsUi::~BlacklistSettingsUi() {
  for (auto c : connections) {
    c.disconnect();
  }

  util::debug(log_tag + "destroyed");
}

bool BlacklistSettingsUi::add_new_entry(Glib::RefPtr<Gio::Settings> settings, const std::string& name, PresetType preset_type) {
  if (name.empty()) {
    return false;
  }

  auto blacklist_preset_type = (preset_type == PresetType::output) ? "blacklist-out" : "blacklist-in";
  std::vector<std::string> bl = settings->get_string_array(blacklist_preset_type);

  // Check if the entry is already added
  for (const auto& str : bl) {
    if (name == str) {
      util::debug("blacklist_settings_ui: entry already present in the list");
      return false;
    }
  }

  bl.emplace_back(name);
  settings->set_string_array(blacklist_preset_type, bl);

  util::debug("blacklist_settings_ui: new entry added in the list");
  return true;
}

void BlacklistSettingsUi::remove_entry(Glib::RefPtr<Gio::Settings> settings, const std::string& name, PresetType preset_type) {
  auto blacklist_preset_type = (preset_type == PresetType::output) ? "blacklist-out" : "blacklist-in";

  std::vector<std::string> bl = settings->get_string_array(blacklist_preset_type);

  bl.erase(std::remove_if(bl.begin(), bl.end(), [=](auto& a) { return a == name; }), bl.end());

  settings->set_string_array(blacklist_preset_type, bl);
}

void BlacklistSettingsUi::add_to_stack(Gtk::Stack* stack) {
  auto builder = Gtk::Builder::create_from_resource("/com/github/wwmm/pulseeffects/ui/blacklist_settings.glade");

  BlacklistSettingsUi* ui;

  builder->get_widget_derived("widgets_grid", ui);

  stack->add(*ui, "settings_blacklist", _("Blacklist"));
}

void BlacklistSettingsUi::populate_blacklist_in_listbox() {
  auto children = blacklist_in_listbox->get_children();

  for (const auto& c : children) {
    blacklist_in_listbox->remove(*c);
  }

  std::vector<std::string> names = settings->get_string_array("blacklist-in");

  for (const auto& name : names) {
    auto b = Gtk::Builder::create_from_resource("/com/github/wwmm/pulseeffects/ui/blacklist_row.glade");

    Gtk::ListBoxRow* row;
    Gtk::Button* remove_btn;
    Gtk::Label* label;

    b->get_widget("blacklist_row", row);
    b->get_widget("remove", remove_btn);
    b->get_widget("name", label);

    row->set_name(name);
    label->set_text(name);

    connections.emplace_back(remove_btn->signal_clicked().connect([=]() {
      remove_entry(settings, name, PresetType::input);

      populate_blacklist_in_listbox();
    }));

    blacklist_in_listbox->add(*row);
    blacklist_in_listbox->show_all();
  }
}

void BlacklistSettingsUi::populate_blacklist_out_listbox() {
  auto children = blacklist_out_listbox->get_children();

  for (auto c : children) {
    blacklist_out_listbox->remove(*c);
  }

  std::vector<std::string> names = settings->get_string_array("blacklist-out");

  for (const auto& name : names) {
    auto b = Gtk::Builder::create_from_resource("/com/github/wwmm/pulseeffects/ui/blacklist_row.glade");

    Gtk::ListBoxRow* row;
    Gtk::Button* remove_btn;
    Gtk::Label* label;

    b->get_widget("blacklist_row", row);
    b->get_widget("remove", remove_btn);
    b->get_widget("name", label);

    row->set_name(name);
    label->set_text(name);

    connections.emplace_back(remove_btn->signal_clicked().connect([=]() {
      remove_entry(settings, name, PresetType::output);

      populate_blacklist_out_listbox();
    }));

    blacklist_out_listbox->add(*row);
    blacklist_out_listbox->show_all();
  }
}

auto BlacklistSettingsUi::on_listbox_sort(Gtk::ListBoxRow* row1, Gtk::ListBoxRow* row2) -> int {
  auto name1 = row1->get_name();
  auto name2 = row2->get_name();

  std::vector<std::string> names = {name1, name2};

  std::sort(names.begin(), names.end());

  if (name1 == names[0]) {
    return -1;
  }

  if (name2 == names[0]) {
    return 1;
  }

  return 0;
}
