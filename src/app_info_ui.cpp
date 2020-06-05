#include "app_info_ui.hpp"
#include "blacklist_settings_ui.hpp"
#include "preset_type.hpp"
#include "util.hpp"
#include <glibmm/i18n.h>
#include <sstream>

AppInfoUi::AppInfoUi(BaseObjectType* cobject,
                     const Glib::RefPtr<Gtk::Builder>& builder,
                     std::shared_ptr<AppInfo> info,
                     PulseManager* pulse_manager)
    : Gtk::Grid(cobject), app_info(std::move(info)), pm(pulse_manager),
      settings(Gio::Settings::create("com.github.wwmm.pulseeffects")) {
  // loading glade widgets

  builder->get_widget("enable", enable);
  builder->get_widget("app_icon", app_icon);
  builder->get_widget("app_name", app_name);
  builder->get_widget("volume", volume);
  builder->get_widget("mute", mute);
  builder->get_widget("blacklist", blacklist);
  builder->get_widget("mute_icon", mute_icon);
  builder->get_widget("format", format);
  builder->get_widget("rate", rate);
  builder->get_widget("channels", channels);
  builder->get_widget("resampler", resampler);
  builder->get_widget("buffer", buffer);
  builder->get_widget("latency", latency);
  builder->get_widget("state", state);

  std::vector<std::string> blacklisted_apps =
      settings->get_string_array((app_info->app_type == "sink_input") ? "blacklist-out" : "blacklist-in");
  blacklisted =
      std::find(std::begin(blacklisted_apps), std::end(blacklisted_apps), app_info->name) != std::end(blacklisted_apps);

  enabled = app_info->connected && !blacklisted;
  pre_bl_state =
      settings->get_boolean((app_info->app_type == "sink_input") ? "enable-all-sinkinputs" : "enable-all-sourceoutputs");

  init_widgets();
  connect_signals();

  if (app_info->app_type == "sink_input") {
    timeout_connection = Glib::signal_timeout().connect_seconds(
        [&]() {
          if (app_info != nullptr) {
            if (app_info->wants_to_play) {
              pm->get_sink_input_info(app_info->index);
            }
          }

          return running;
        },
        5);
  }
}

AppInfoUi::~AppInfoUi() {
  running = false;
  timeout_connection.disconnect();

  util::debug(log_tag + app_info->name + " info ui destroyed");
}

auto AppInfoUi::latency_to_str(uint value) -> std::string {
  std::ostringstream msg;
  const float ms_factor = 0.001F;

  msg.precision(1);
  msg << std::fixed << value * ms_factor << " ms";

  return msg.str();
}

void AppInfoUi::init_widgets() {
  enable->set_active(enabled && !blacklisted);
  enable->set_sensitive(!blacklisted);

  blacklist->set_active(blacklisted);

  app_icon->set_from_icon_name(app_info->icon_name, Gtk::ICON_SIZE_BUTTON);

  app_name->set_text(app_info->name);

  volume->set_value(app_info->volume);

  mute->set_active(app_info->mute != 0);

  format->set_text(app_info->format);

  rate->set_text(std::to_string(app_info->rate) + " Hz");

  channels->set_text(std::to_string(app_info->channels));

  resampler->set_text(app_info->resampler);

  buffer->set_text(latency_to_str(app_info->buffer));

  latency->set_text(latency_to_str(app_info->latency));

  if (app_info->corked != 0) {
    state->set_text(_("paused"));
  } else {
    state->set_text(_("playing"));
  }
}

void AppInfoUi::connect_signals() {
  enable_connection = enable->signal_state_set().connect(sigc::mem_fun(*this, &AppInfoUi::on_enable_app), false);

  volume_connection = volume->signal_value_changed().connect(sigc::mem_fun(*this, &AppInfoUi::on_volume_changed));

  mute_connection = mute->signal_toggled().connect(sigc::mem_fun(*this, &AppInfoUi::on_mute));

  blacklist_connection = blacklist->signal_clicked().connect([=]() {
    PresetType preset_type = (app_info->app_type == "sink_input") ? PresetType::output : PresetType::input;

    if (blacklist->get_active()) {
      // Add new entry to blacklist vector
      BlacklistSettingsUi::add_new_entry(settings, app_info->name, preset_type);
      pre_bl_state = enabled;
      blacklisted = true;
      if (enabled) {
        enable->set_active(false);
      }
      enable->set_sensitive(false);
    } else {
      // Remove app name entry from blacklist vector
      BlacklistSettingsUi::remove_entry(settings, app_info->name, preset_type);
      blacklisted = false;
      enable->set_sensitive(true);
      if (pre_bl_state) {
        enable->set_active(true);
      }
    }
  });
}

auto AppInfoUi::on_enable_app(bool state) -> bool {
  if (state) {
    if (app_info->app_type == "sink_input") {
      pm->move_sink_input_to_pulseeffects(app_info->name, app_info->index);
    } else {
      pm->move_source_output_to_pulseeffects(app_info->name, app_info->index);
    }
  } else {
    if (app_info->app_type == "sink_input") {
      pm->remove_sink_input_from_pulseeffects(app_info->name, app_info->index);
    } else {
      pm->remove_source_output_from_pulseeffects(app_info->name, app_info->index);
    }
  }

  enabled = state;

  return false;
}

void AppInfoUi::on_volume_changed() {
  auto value = volume->get_value();

  if (app_info->app_type == "sink_input") {
    pm->set_sink_input_volume(app_info->name, app_info->index, app_info->channels, value);
  } else {
    pm->set_source_output_volume(app_info->name, app_info->index, app_info->channels, value);
  }
}

void AppInfoUi::on_mute() {
  bool state = mute->get_active();

  if (state) {
    mute_icon->set_from_icon_name("audio-volume-muted-symbolic", Gtk::ICON_SIZE_BUTTON);

    volume->set_sensitive(false);
  } else {
    mute_icon->set_from_icon_name("audio-volume-high-symbolic", Gtk::ICON_SIZE_BUTTON);

    volume->set_sensitive(true);
  }

  if (app_info->app_type == "sink_input") {
    pm->set_sink_input_mute(app_info->name, app_info->index, state);
  } else {
    pm->set_source_output_mute(app_info->name, app_info->index, state);
  }
}

void AppInfoUi::update(const std::shared_ptr<AppInfo>& info) {
  app_info = info;

  enable_connection.disconnect();
  volume_connection.disconnect();
  mute_connection.disconnect();
  blacklist_connection.disconnect();

  init_widgets();
  connect_signals();
}
