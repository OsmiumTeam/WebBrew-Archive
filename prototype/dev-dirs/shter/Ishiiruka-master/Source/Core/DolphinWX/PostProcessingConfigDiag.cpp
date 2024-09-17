// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <math.h>
#include <unordered_map>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "Common/StringUtil.h"

#include "DolphinWX/PostProcessingConfigDiag.h"

#include "VideoCommon/RenderBase.h"

PostProcessingConfigDiag::PostProcessingConfigDiag(wxWindow* parent, const std::string& shader_dir, const std::string& shader_name, PostProcessingShaderConfiguration* config)
  : wxDialog(parent, wxID_ANY, _("Post Processing Shader Configuration"))
  , m_config(config)
{
  if (!m_config)
  {
    // Config not active, so create a temporary config
    m_temp_config = std::make_unique<PostProcessingShaderConfiguration>();
    m_temp_config->LoadShader(shader_dir, shader_name);
    m_config = m_temp_config.get();
  }

  // Create our UI classes
  const PostProcessingShaderConfiguration::ConfigMap& config_map = m_config->GetOptions();
  for (const auto& it : config_map)
  {
    if (it.second.m_type == POST_PROCESSING_OPTION_TYPE_BOOL)
    {
      m_config_map[it.first] = std::make_unique<ConfigGrouping>(ConfigGrouping::WidgetType::TYPE_TOGGLE,
        it.second.m_gui_name, it.second.m_gui_description, it.first, it.second.m_dependent_option,
        &it.second);
    }
    else
    {
      m_config_map[it.first] = std::make_unique<ConfigGrouping>(ConfigGrouping::WidgetType::TYPE_SLIDER,
        it.second.m_gui_name, it.second.m_gui_description, it.first, it.second.m_dependent_option,
        &it.second);
    }
  }

  // Arrange our vectors based on dependency
  for (const auto& it : m_config_map)
  {
    const std::string parent_name = it.second->GetParent();
    if (parent_name.size())
    {
      // Since it depends on a different object, push it to a parent's object
      m_config_map[parent_name]->AddChild(m_config_map[it.first].get());
    }
    else
    {
      // It doesn't have a child, just push it to the vector
      m_config_groups.push_back(m_config_map[it.first].get());
    }
  }

  // Generate our UI
  wxNotebook* const notebook = new wxNotebook(this, wxID_ANY);
  wxPanel* page_general = nullptr;
  wxBoxSizer* szr_general = nullptr;
  wxFlexGridSizer* grid_general = nullptr;

  // Now let's actually populate our window with our information
  for (const auto& it : m_config_groups)
  {
    if (it->HasChildren())
    {
      // Options with children get their own tab
      wxPanel* const page_option = new wxPanel(notebook);
      wxBoxSizer* const szr_option = new wxBoxSizer(wxVERTICAL);
      wxFlexGridSizer* grid = new	wxFlexGridSizer(2, 10, 5);
      it->GenerateUI(this, page_option, grid);

      // Add all the children
      for (const auto& child : it->GetChildren())
      {
        child->GenerateUI(this, page_option, grid);
      }
      szr_option->Add(grid);
      CreateDescriptionArea(page_option, szr_option);
      page_option->SetSizerAndFit(szr_option);
      notebook->AddPage(page_option, _(it->GetGUIName()));
    }
    else
    {
      // Options with no children go in to the general tab
      // Make it so it doesn't show up if there aren't any options without children.
      if (!szr_general)
      {
        page_general = new wxPanel(notebook);
        szr_general = new wxBoxSizer(wxVERTICAL);
        grid_general = new	wxFlexGridSizer(2, 5, 5);
      }
      it->GenerateUI(this, page_general, grid_general);
    }
  }

  if (page_general)
  {
    szr_general->Add(grid_general);
    CreateDescriptionArea(page_general, szr_general);
    page_general->SetSizerAndFit(szr_general);
    notebook->InsertPage(0, page_general, _("General"));
  }

  // Close Button
  wxBoxSizer* const buttons_szr = new wxBoxSizer(wxHORIZONTAL);
  wxButton* const btn_defaults = new wxButton(this, wxID_ANY, _("Restore &Defaults"));
  btn_defaults->Bind(wxEVT_BUTTON, &PostProcessingConfigDiag::Event_RestoreDefaults, this);
  buttons_szr->Add(btn_defaults, 0, wxRIGHT, 5);
  wxButton* const btn_close = new wxButton(this, wxID_OK, _("Cl&ose"));

  btn_close->Bind(wxEVT_BUTTON, &PostProcessingConfigDiag::Event_ClickClose, this);
  Bind(wxEVT_CLOSE_WINDOW, &PostProcessingConfigDiag::Event_Close, this);
  buttons_szr->Add(btn_close, 0, wxRIGHT, 1);

  wxBoxSizer* const szr_main = new wxBoxSizer(wxVERTICAL);
  szr_main->Add(notebook, 1, wxEXPAND | wxALL, 5);
  szr_main->Add(buttons_szr, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 5);

  SetSizerAndFit(szr_main);
  Center();
  SetFocus();

  UpdateWindowUI();
}

PostProcessingConfigDiag::~PostProcessingConfigDiag()
{
  m_config->SaveOptionsConfiguration();
}

void PostProcessingConfigDiag::ConfigGrouping::GenerateUI(PostProcessingConfigDiag* dialog, wxWindow* parent, wxFlexGridSizer* sizer)
{

  if (m_type == WidgetType::TYPE_TOGGLE)
  {
    m_option_checkbox = new wxCheckBox(parent, wxID_ANY, _(m_gui_name));
    m_option_checkbox->SetValue(m_config_option->m_bool_value);
    m_option_checkbox->Bind(wxEVT_CHECKBOX, &PostProcessingConfigDiag::Event_CheckBox,
      dialog, wxID_ANY, wxID_ANY, new UserEventData(m_option));

    sizer->Add(dialog->RegisterControl(m_option_checkbox, _(m_gui_description)));
    sizer->AddStretchSpacer();
  }
  else
  {
    size_t vector_size = 0;
    if (m_config_option->m_type == POST_PROCESSING_OPTION_TYPE_INTEGER)
      vector_size = m_config_option->m_integer_values.size();
    else
      vector_size = m_config_option->m_float_values.size();

    wxFlexGridSizer* const szr_values = new wxFlexGridSizer(vector_size + 1, 0, 0);
    wxStaticText* const option_static_text = new wxStaticText(parent, wxID_ANY, m_gui_name);
    sizer->Add(dialog->RegisterControl(option_static_text, m_gui_description));

    for (size_t i = 0; i < vector_size; ++i)
    {
      // wxSlider uses a signed integer for it's minimum and maximum values
      // This won't work for floats.
      // Let's determine how many steps we can take and use that instead.
      int steps = 0;
      int current_value = 0;
      std::string string_value;
      if (m_config_option->m_type == POST_PROCESSING_OPTION_TYPE_INTEGER)
      {
        // Find out our range by taking the max subtracting the minimum.
        double range = m_config_option->m_integer_max_values[i] - m_config_option->m_integer_min_values[i];

        // How many steps we have is the range divided by the step interval configured.
        // This may not be 100% spot on accurate since developers can have odd stepping intervals set.
        // Round up so if it is outside our range, then set it to the minimum or maximum
        steps = ceil(range / (double)m_config_option->m_integer_step_values[i]);

        // Default value is just the currently set value here
        current_value = std::max((m_config_option->m_integer_values[i] - m_config_option->m_integer_min_values[i]) / m_config_option->m_integer_step_values[i], 0);
        string_value = std::to_string(m_config_option->m_integer_values[i]);
      }
      else
      {
        // Same as above but with floats
        float range = m_config_option->m_float_max_values[i] - m_config_option->m_float_min_values[i];
        steps = ceil(range / m_config_option->m_float_step_values[i]);

        // We need to convert our default float value from a float to the nearest step value range
        current_value = ((m_config_option->m_float_values[i] - m_config_option->m_float_min_values[i]) / m_config_option->m_float_step_values[i]);
        string_value = std::to_string(m_config_option->m_float_values[i]);
      }

      DolphinSlider* slider = new DolphinSlider(parent, wxID_ANY, current_value, 0, steps,
        wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL | wxSL_BOTTOM);
      wxTextCtrl* text_ctrl = new wxTextCtrl(parent, wxID_ANY, string_value);

      // Disable the textctrl, it's only there to show the absolute value from the slider
      text_ctrl->Disable();

      // wxWidget takes over the pointer provided to it in the event handler.
      // This won't be a memory leak, it'll be destroyed on dialog close.
      slider->Bind(wxEVT_SLIDER, &PostProcessingConfigDiag::Event_Slider,
        dialog, wxID_ANY, wxID_ANY, new UserEventData(m_option));
      slider->Bind(wxEVT_SCROLL_THUMBRELEASE, &PostProcessingConfigDiag::Event_Slider_Finish,
        dialog, wxID_ANY, wxID_ANY, new UserEventData(m_option));
      dialog->RegisterControl(slider, m_gui_description);
      dialog->RegisterControl(text_ctrl, m_gui_description);
      m_option_sliders.push_back(slider);
      m_option_text_ctrls.push_back(text_ctrl);
    }

    if (vector_size == 1)
    {
      szr_values->Add(m_option_sliders[0], wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
      szr_values->Add(m_option_text_ctrls[0]);

      sizer->Add(szr_values);
    }
    else
    {
      wxFlexGridSizer* const szr_inside = new wxFlexGridSizer(2, 0, 0);
      for (size_t i = 0; i < vector_size; ++i)
      {
        szr_inside->Add(m_option_sliders[i], wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
        szr_inside->Add(m_option_text_ctrls[i]);
      }

      szr_values->Add(szr_inside);
      sizer->Add(szr_values);
    }
  }
}

void PostProcessingConfigDiag::ConfigGrouping::EnableDependentChildren(bool enable)
{
  // Enable or disable all the children
  for (auto& it : m_children)
  {
    if (it->m_type == ConfigGrouping::WidgetType::TYPE_TOGGLE)
    {
      it->m_option_checkbox->Enable(enable);
    }
    else
    {
      for (auto& slider : it->m_option_sliders)
        slider->Enable(enable);
    }
    // Set this objects children as well
    it->EnableDependentChildren(enable);
  }
}

void PostProcessingConfigDiag::Event_CheckBox(wxCommandEvent &ev)
{
  UserEventData* config_option = (UserEventData*)ev.GetEventUserData();
  ConfigGrouping* config = m_config_map[config_option->GetData()].get();

  m_config->SetOptionb(config->GetOption(), ev.IsChecked());

  config->EnableDependentChildren(ev.IsChecked());

  ev.Skip();
}

void PostProcessingConfigDiag::Event_Slider_Finish(wxScrollEvent &ev)
{
  UserEventData* config_option = (UserEventData*)ev.GetEventUserData();
  ConfigGrouping* config = m_config_map[config_option->GetData()].get();

  const auto& option_data = m_config->GetOption(config->GetOption());
  if (!option_data.m_compile_time_constant)
  {
    // Just handle options that must be resolved at compilation time
    ev.Skip();
    return;
  }

  size_t vector_size = 0;
  if (option_data.m_type == POST_PROCESSING_OPTION_TYPE_INTEGER)
    vector_size = option_data.m_integer_values.size();
  else
    vector_size = option_data.m_float_values.size();

  for (size_t i = 0; i < vector_size; ++i)
  {
    // Got to do this garbage again.
    // Convert current step in to the real range value
    int current_step = config->GetSliderValue(i);
    std::string string_value;
    if (option_data.m_type == POST_PROCESSING_OPTION_TYPE_INTEGER)
    {
      s32 value = option_data.m_integer_step_values[i] * current_step + option_data.m_integer_min_values[i];
      m_config->SetOptioni(config->GetOption(), i, value);
    }
    else
    {
      float value = option_data.m_float_step_values[i] * current_step + option_data.m_float_min_values[i];
      m_config->SetOptionf(config->GetOption(), i, value);
    }
  }
  ev.Skip();
}

void PostProcessingConfigDiag::Event_Slider(wxCommandEvent &ev)
{
  UserEventData* config_option = (UserEventData*)ev.GetEventUserData();
  ConfigGrouping* config = m_config_map[config_option->GetData()].get();

  const auto& option_data = m_config->GetOption(config->GetOption());
  size_t vector_size = 0;
  if (option_data.m_type == POST_PROCESSING_OPTION_TYPE_INTEGER)
    vector_size = option_data.m_integer_values.size();
  else
    vector_size = option_data.m_float_values.size();

  for (size_t i = 0; i < vector_size; ++i)
  {
    // Got to do this garbage again.
    // Convert current step in to the real range value
    int current_step = config->GetSliderValue(i);
    std::string string_value;
    if (option_data.m_type == POST_PROCESSING_OPTION_TYPE_INTEGER)
    {
      s32 value = option_data.m_integer_step_values[i] * current_step + option_data.m_integer_min_values[i];
      if (!option_data.m_compile_time_constant)
        m_config->SetOptioni(config->GetOption(), i, value);
      string_value = std::to_string(value);
    }
    else
    {
      float value = option_data.m_float_step_values[i] * current_step + option_data.m_float_min_values[i];
      if (!option_data.m_compile_time_constant)
        m_config->SetOptionf(config->GetOption(), i, value);
      string_value = std::to_string(value);
    }
    // Update the text box to include the new value
    config->SetSliderText(i, string_value);
  }
  ev.Skip();
}

void PostProcessingConfigDiag::Event_ClickClose(wxCommandEvent&)
{
  Close();
}

void PostProcessingConfigDiag::Event_Close(wxCloseEvent& ev)
{
  EndModal(wxID_OK);
}

wxControl* PostProcessingConfigDiag::RegisterControl(wxControl* const control, const wxString& description)
{
  ctrl_descs.insert(std::pair<wxWindow*, wxString>(control, description));
  control->Bind(wxEVT_ENTER_WINDOW, &PostProcessingConfigDiag::Evt_EnterControl, this);
  control->Bind(wxEVT_LEAVE_WINDOW, &PostProcessingConfigDiag::Evt_LeaveControl, this);
  return control;
}

void PostProcessingConfigDiag::Evt_EnterControl(wxMouseEvent& ev)
{
  // TODO: Re-Fit the sizer if necessary!

  // Get settings control object from event
  wxWindow* ctrl = (wxWindow*)ev.GetEventObject();
  if (!ctrl) return;

  // look up description text object from the control's parent (which is the wxPanel of the current tab)
  wxStaticText* descr_text = desc_texts[ctrl->GetParent()];
  if (!descr_text) return;
  if (ctrl_descs[ctrl].size() > 0)
  {
    // look up the description of the selected control and assign it to the current description text object's label
    descr_text->SetLabel(ctrl_descs[ctrl]);
    descr_text->Wrap(descr_text->GetContainingSizer()->GetSize().x - 20);
  }
  ev.Skip();
}

// TODO: Don't hardcode the size of the description area via line breaks
#define DEFAULT_DESC_TEXT _("Move the mouse pointer over an option to display a detailed description.\n\n\n\n\n\n\n")
void PostProcessingConfigDiag::Evt_LeaveControl(wxMouseEvent& ev)
{
  // look up description text control and reset its label
  wxWindow* ctrl = (wxWindow*)ev.GetEventObject();
  if (!ctrl) return;
  wxStaticText* descr_text = desc_texts[ctrl->GetParent()];
  if (!descr_text) return;

  descr_text->SetLabel(DEFAULT_DESC_TEXT);
  descr_text->Wrap(descr_text->GetContainingSizer()->GetSize().x - 20);
  ev.Skip();
}

void PostProcessingConfigDiag::CreateDescriptionArea(wxPanel* const page, wxBoxSizer* const sizer)
{
  sizer->AddStretchSpacer();
  // Create description frame
  wxStaticBoxSizer* const desc_sizer = new wxStaticBoxSizer(wxVERTICAL, page, _("Description"));
  sizer->Add(desc_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  // Need to call SetSizerAndFit here, since we don't want the description texts to change the dialog width
  page->SetSizerAndFit(sizer);

  // Create description text
  wxStaticText* const desc_text = new wxStaticText(page, wxID_ANY, DEFAULT_DESC_TEXT);
  desc_text->Wrap(desc_sizer->GetSize().x - 20);
  desc_sizer->Add(desc_text, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  // Store description text object for later lookup
  desc_texts.insert(std::pair<wxWindow*, wxStaticText*>(page, desc_text));
}

void PostProcessingConfigDiag::Event_RestoreDefaults(wxCommandEvent& ev)
{
  for (auto& it : m_config->GetOptions())
  {
    // Skip options that aren't in the UI, since those can't be changed anyway
    ConfigGrouping* group;
    auto group_it = m_config_map.find(it.first);
    if (group_it != m_config_map.end())
      group = group_it->second.get();
    else
      continue;

    switch (it.second.m_type)
    {
    case POST_PROCESSING_OPTION_TYPE_BOOL:
      m_config->SetOptionb(it.first, it.second.m_default_bool_value);
      group->SetToggleValue(it.second.m_default_bool_value);
      break;

    case POST_PROCESSING_OPTION_TYPE_FLOAT:
    {
      for (size_t i = 0; i < it.second.m_default_float_values.size(); i++)
      {
        float value = it.second.m_default_float_values[i];
        float pos = (value - it.second.m_float_min_values[i]) / it.second.m_float_step_values[i];
        m_config->SetOptionf(it.first, i, value);
        group->SetSliderValue(i, static_cast<int>(pos));
        group->SetSliderText(i, std::to_string(value));
      }
    }
    break;

    case POST_PROCESSING_OPTION_TYPE_INTEGER:
    {
      for (size_t i = 0; i < it.second.m_default_integer_values.size(); i++)
      {
        int value = it.second.m_default_integer_values[i];
        int pos = std::max((value - it.second.m_integer_min_values[i]) / std::max(it.second.m_integer_step_values[i], 0), 0);
        m_config->SetOptioni(it.first, i, value);
        group->SetSliderValue(i, pos);
        group->SetSliderText(i, std::to_string(value));
      }
    }
    break;
    }
  }
}
