#ifndef EQUALIZER_UI_HPP
#define EQUALIZER_UI_HPP

#include "plugin_ui_base.hpp"

class EqualizerUi : public Gtk::Grid, public PluginUiBase {
 public:
  EqualizerUi(BaseObjectType* cobject,
              const Glib::RefPtr<Gtk::Builder>& builder,
              const std::string& schema,
              const std::string& schema_path,
              const std::string& schema_channel,
              const std::string& schema_channel_left_path,
              const std::string& schema_channel_right_path);
  EqualizerUi(const EqualizerUi&) = delete;
  auto operator=(const EqualizerUi&) -> EqualizerUi& = delete;
  EqualizerUi(const EqualizerUi&&) = delete;
  auto operator=(const EqualizerUi &&) -> EqualizerUi& = delete;
  ~EqualizerUi() override;

  void reset() override;

 private:
  int max_bands = 30;

  Glib::RefPtr<Gio::Settings> settings_left, settings_right;

  Glib::RefPtr<Gtk::Adjustment> nbands, input_gain, output_gain;
  Gtk::Grid *bands_grid_left = nullptr, *bands_grid_right = nullptr;
  Gtk::Button *flat_response = nullptr, *calculate_freqs = nullptr, *import_apo = nullptr;
  Gtk::ListBox* presets_listbox = nullptr;
  Gtk::Switch* split_channels = nullptr;
  Gtk::Stack* stack = nullptr;
  Gtk::StackSwitcher* stack_switcher = nullptr;
  Gtk::ComboBoxText* mode = nullptr;

  std::vector<sigc::connection> connections_bands;

  std::string presets_path = "/com/github/wwmm/pulseeffects/presets/";

  void load_preset(const std::string& file_name);

  void on_nbands_changed();

  void build_bands(Gtk::Grid* bands_grid, const Glib::RefPtr<Gio::Settings>& cfg, const int& nbands);

  void build_unified_bands(const int& nbands);

  void on_flat_response();

  void on_calculate_frequencies();

  static auto on_listbox_sort(Gtk::ListBoxRow* row1, Gtk::ListBoxRow* row2) -> int;

  void populate_presets_listbox();

  void on_import_apo_preset_clicked();

  void import_apo_preset(const std::string& file_path);
};

#endif
