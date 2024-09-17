// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <string>
#include <vector>

#include <wx/dialog.h>
#include "DolphinSlider.h"
#include <wx/textctrl.h>

#include "VideoCommon/PostProcessing.h"

class wxButton;
class wxCheckBox;
class wxFlexGridSizer;

class PostProcessingConfigDiag final : public wxDialog
{
public:
  PostProcessingConfigDiag(wxWindow* parent, const std::string& shader_dir, const std::string& shader_name, PostProcessingShaderConfiguration* config = nullptr);
  ~PostProcessingConfigDiag();

private:

  // This is literally the stupidest thing ever
  // wxWidgets takes ownership of any pointer given to a event handler
  // Instead of passing them a pointer to a std::string, we wrap around it here.
  class UserEventData : public wxObject
  {
  public:
    UserEventData(const std::string& data) : m_data(data)
    {}
    const std::string& GetData()
    {
      return m_data;
    }
  private:
    const std::string m_data;
  };

  class ConfigGrouping
  {
  public:
    enum WidgetType
    {
      TYPE_TOGGLE,
      TYPE_SLIDER,
    };

    ConfigGrouping(WidgetType type, const std::string& gui_name, const std::string& gui_description,
      const std::string& option_name, const std::string& parent,
      const PostProcessingShaderConfiguration::ConfigurationOption* config_option)
      : m_type(type), m_gui_name(gui_name), m_gui_description(gui_description),
      m_option(option_name), m_parent(parent),
      m_config_option(config_option)
    {}

    void AddChild(ConfigGrouping* child)
    {
      m_children.push_back(child);
    }
    bool HasChildren()
    {
      return m_children.size() != 0;
    }
    std::vector<ConfigGrouping*>& GetChildren()
    {
      return m_children;
    }

    // Gets the string that is shown in the UI for the option
    const std::string& GetGUIName()
    {
      return m_gui_name;
    }
    // Gets the string that is shown in the UI as a description for the option
    const std::string& GetGUIDescription()
    {
      return m_gui_description;
    }
    // Gets the option name for use in the shader
    // Also is a unique identifier for the option's configuration
    const std::string& GetOption()
    {
      return m_option;
    }
    // Gets the option name of the parent of this option
    const std::string& GetParent()
    {
      return m_parent;
    }

    void GenerateUI(PostProcessingConfigDiag* dialog, wxWindow* parent, wxFlexGridSizer* sizer);

    void EnableDependentChildren(bool enable);

    int GetSliderValue(int index)
    {
      return m_option_sliders[index]->GetValue();
    }
    void SetSliderText(int index, const std::string& text)
    {
      m_option_text_ctrls[index]->SetValue(text);
    }
    void SetSliderValue(int index, int value)
    {
      m_option_sliders[index]->SetValue(value);
    }
    void SetToggleValue(bool value)
    {
      m_option_checkbox->SetValue(value);
    }

  private:
    const WidgetType m_type;
    const std::string m_gui_name;
    const std::string m_gui_description;
    const std::string m_option;
    const std::string m_parent;
    const PostProcessingShaderConfiguration::ConfigurationOption* m_config_option;

    // For TYPE_TOGGLE
    wxCheckBox* m_option_checkbox;

    // For TYPE_SLIDER
    // Can have up to 4
    std::vector<DolphinSlider*> m_option_sliders;
    std::vector<wxTextCtrl*> m_option_text_ctrls;

    std::vector<ConfigGrouping*> m_children;
  };

  // WX UI things
  void Event_Close(wxCloseEvent& ev);
  void Event_ClickClose(wxCommandEvent& ev);
  void Event_RestoreDefaults(wxCommandEvent& ev);
  void Event_Slider(wxCommandEvent &ev);
  void Event_Slider_Finish(wxScrollEvent &ev);
  void Event_CheckBox(wxCommandEvent &ev);

  PostProcessingShaderConfiguration* m_config;
  std::unique_ptr<PostProcessingShaderConfiguration> m_temp_config;

  std::map<std::string, std::unique_ptr<ConfigGrouping>> m_config_map;
  std::vector<ConfigGrouping*> m_config_groups;

  // Tooltips
  wxControl* RegisterControl(wxControl* const control, const wxString& description);

  void Evt_EnterControl(wxMouseEvent& ev);
  void Evt_LeaveControl(wxMouseEvent& ev);
  void CreateDescriptionArea(wxPanel* const page, wxBoxSizer* const sizer);
  std::map<wxWindow*, wxString> ctrl_descs; // maps setting controls to their descriptions
  std::map<wxWindow*, wxStaticText*> desc_texts; // maps dialog tabs (which are the parents of the setting controls) to their description text objects
};

