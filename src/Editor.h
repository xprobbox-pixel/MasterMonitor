#pragma once
#include "Processor.h"
class HardwareLook final:public juce::LookAndFeel_V4 {
public:
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;
 void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool)override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool)override;
 void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&)override;
 juce::Font getComboBoxFont(juce::ComboBox&)override;
 void drawCornerResizer(juce::Graphics&,int,int,bool,bool)override;
};
class MasterEditor final:public juce::AudioProcessorEditor,public juce::SettableTooltipClient,private juce::Timer {
 MasterProcessor& processor;HardwareLook look;juce::TooltipWindow tooltips{this,500};
 // Scale a child, never the editor: the host owns the editor's HiDPI transform.
 juce::Component controls;
 juce::ComboBox uiScale;
 juce::Label scaleLabel;
 mm::Snapshot data;juce::Image logo,background;
 std::array<juce::Slider,8> knobs;std::array<juce::Label,8> labels;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,8> knobLinks;
 std::array<juce::ToggleButton,4> toggles;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,4> toggleLinks;
 juce::TextButton reset{"RESET MEASUREMENTS"},peaks{"PEAK HOLD RESET"};
 void timerCallback()override;void drawStatic();void updateScaleReadout();
public:
 explicit MasterEditor(MasterProcessor&);~MasterEditor()override;
 void paint(juce::Graphics&)override;void resized()override;
};
