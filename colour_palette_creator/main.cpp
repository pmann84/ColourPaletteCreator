#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>

#include <array>
#include <sstream>

// Articles used to help with this generation method
// https://iquilezles.org/www/articles/palettes/palettes.html
// https://softologyblog.wordpress.com/2019/03/23/automatic-color-palette-creation/

class colour
{
public:
   colour()
      : m_channels{0.0f, 0.0f, 0.0f, 1.0f }
   {
   }

   colour(float r, float g, float b, float a = 1.0f)
      : m_channels{r, g, b, a}
   {
   }

   colour(float c[4]) : m_channels{c[0], c[1], c[2], c[3]}
   {
   }

   colour(uint32_t c)
      : m_channels{
         static_cast<float>(c & 0x000000ff) / 255.0f,
         static_cast<float>((c & 0x0000ff00) >> 8) / 255.0f,
         static_cast<float>((c & 0x00ff0000) >> 16) / 255.0f,
         static_cast<float>((c & 0xff000000) >> 24) / 255.0f }
   {
   }

   uint32_t to_int() const
   {
      // Convert internal format to 32 bit colour representation
      // Bits 0 - 7 are the red channel
      // bits 8 - 15 the green channel,
      // bits 16 - 23 the blue channel,
      // and bits 24 -  31 the alpha channel
      const uint8_t r = static_cast<uint8_t>(m_channels[0] * 255);
      const uint8_t g = static_cast<uint8_t>(m_channels[1] * 255);
      const uint8_t b = static_cast<uint8_t>(m_channels[2] * 255);
      const uint8_t a = static_cast<uint8_t>(m_channels[3] * 255);
      return a | (b << 8) | (g << 16) | (r << 24);
   }

   std::array<float, 4> rgba() const
   {
      return m_channels;
   }
   float r() const { return m_channels[0]; }
   float g() const { return m_channels[1]; }
   float b() const { return m_channels[2]; }
   float a() const { return m_channels[3]; }
   void set(float r, float g, float b, float a = 1.0f)
   {
      m_channels[0] = r;
      m_channels[1] = g;
      m_channels[2] = b;
      m_channels[3] = a;
   }

private:
   std::array<float, 4> m_channels;
};


struct sine_params
{
   float amplitude;
   float frequency;
   float vertical_offset;
   float horizontal_offset;
};

inline float sine_colour_palette(float x, sine_params params)
{
   static const float pi = 3.141592654f;
   return params.amplitude * sin(2.0*pi*(params.frequency*x + params.horizontal_offset)) + params.vertical_offset;
}

inline colour sine_colour_palette(float x, colour frequency, colour v_offset, colour h_offset, colour amplitude)
{
   // Need to work out actually what im calculating 0..255, or 0.0..1.0? Be consistent
   const sine_params r_params = { amplitude.r(), frequency.r(), v_offset.r(), h_offset.r() };
   const sine_params g_params = { amplitude.g(), frequency.g(), v_offset.g(), h_offset.g() };
   const sine_params b_params = { amplitude.b(), frequency.b(), v_offset.b(), h_offset.b() };
   const float r = sine_colour_palette(x, r_params);
   const float g = sine_colour_palette(x, g_params);
   const float b = sine_colour_palette(x, b_params);
   return { r, g, b, 1.0f };
}

void draw_rectangle(sf::RenderWindow& window, sf::Color colour, sf::Vector2f square_size, sf::Vector2f square_centre)
{
   sf::RectangleShape rectangle(square_size);
   rectangle.setPosition(square_centre);
   rectangle.setFillColor(colour);
   window.draw(rectangle);
}

static void HelpMarker(const char* desc)
{
   ImGui::TextDisabled("(?)");
   if (ImGui::IsItemHovered())
   {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextUnformatted(desc);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
   }
}

int main()
{
   static const float pi = 3.141592654f;
   const unsigned int width = 1024;
   const unsigned int height = 768;
   const float drag_speed = 0.0001f;

   sf::RenderWindow window(sf::VideoMode(width, height), "Colour Palette Creator");
   ImGui::SFML::Init(window);
   window.resetGLStates(); // call it if you only draw ImGui. Otherwise not needed.

   // run the program as long as the window is open
   sf::Clock deltaClock;
   while (window.isOpen())
   {
      // check all the window's events that were triggered since the last iteration of the loop
      sf::Event event;
      while (window.pollEvent(event))
      {
         ImGui::SFML::ProcessEvent(event);
         // "close requested" event: we close the window
         if (event.type == sf::Event::Closed)
            window.close();
      }

      ImGui::SFML::Update(window, deltaClock.restart());

      ImGui::Begin("Creator");

      static float bg_col[3] = { 1.0f, 1.0f, 1.0f };
      ImGui::ColorEdit3("Background Colour", bg_col);

      // Number of colours
      static int num_colours = 256;
      ImGui::DragInt("Number of Colours", &num_colours, 1, 5, 2000);

      // Frequency controls
      static bool wrap_palette = true;
      ImGui::Checkbox("Wrap Colours", &wrap_palette);
      static bool common_freq = true;
      static int init_num_even_halves = 1;
      static float init_common_freq = 1.0f;
      static float init_ind_freq[3] = { init_common_freq, init_common_freq, init_common_freq };
      if (wrap_palette)
      {
         ImGui::DragInt("Frequency Multiplier", &init_num_even_halves, 1, 0, 10);
      }
      else
      {
         ImGui::Checkbox("Common Frequency", &common_freq);
         if (common_freq)
         {
            ImGui::DragFloat("Frequency", &init_common_freq, drag_speed * 10.0, 0.0f, 10.0f);
         }
         else
         {
            ImGui::DragFloat3("Frequency", init_ind_freq, drag_speed * 10.0, 0.0f, 10.0f);
         }
      }

      // Gamut controls for generating pastel shades
      static float init_amp = 0.5f;
      ImGui::SliderFloat("Amplitude", &init_amp, 0.0f, 0.5f);
      ImGui::SameLine(); HelpMarker("Amplitude and Vertical offset can be used to create pastel shades, the maximum amplitude can really only be 0.5. And the range of the vertical offset is determined by the current amplitude.");

      static float init_voff = init_amp;
      float range1 = init_amp;
      float range2 = 1.0f - init_amp;
      if (init_voff > range2)
         init_voff = range2;
      ImGui::SliderFloat("Vertical Offset", &init_voff, range1, range2);

      // TODO: Make two options, phase + offset and 3 vec offset
      // Controls for adjusting the starting colour
      static float init_hoff_adj = 0.0;
      ImGui::SliderFloat("Horizontal Offset", &init_hoff_adj, 0.0f, 2.0*pi);

      // Phase adjustment - adjusts the phase of the 3 colour channel waves
      static float init_phase_shift = 2.0f * pi / 3.0f; // On screen is degrees, and output to this variable in radians
      ImGui::SliderAngle("Phase Shift", &init_phase_shift, 0.0f, 360.0f);

      colour freq;
      float calc_freq = init_num_even_halves * 2.0f * 0.5f / num_colours;
      freq.set(calc_freq, calc_freq, calc_freq);
      if (!wrap_palette && common_freq)
      {
         calc_freq = init_common_freq / num_colours;
         freq.set(calc_freq, calc_freq, calc_freq);
      }
      else if (!wrap_palette && !common_freq)
      {

         freq.set(init_ind_freq[0] / num_colours, init_ind_freq[1] / num_colours, init_ind_freq[2] / num_colours);
      }

      colour v_off(init_voff, init_voff, init_voff);
      colour amp(init_amp, init_amp, init_amp);
      colour h_off(0.0f + init_hoff_adj, 2.0f * init_phase_shift / (2.0f*pi) + init_hoff_adj, 4.0f * init_phase_shift / (2.0f*pi) + init_hoff_adj);

      // Final label to show sine function parameters that you have generated!
      // TODO: Show amplitude, frequency, v_offset, h_offset
      std::stringstream eqn_ss;
      eqn_ss << "c(t) = A sin(2pi(fx + d)) + C";
      ImGui::LabelText(eqn_ss.str().c_str(), "Colour Equation");
      std::stringstream amp_ss;
      amp_ss << "A(r,g,b): " << amp.r() << ", " << amp.g() << ", " << amp.b();
      ImGui::TextDisabled(amp_ss.str().c_str());

      std::stringstream freq_ss;
      freq_ss << "f(r,g,b): " << freq.r() << ", " << freq.g() << ", " << freq.b();
      ImGui::TextDisabled(freq_ss.str().c_str());

      std::stringstream h_off_ss;
      h_off_ss << "d(r,g,b): " << h_off.r() << ", " << h_off.g() << ", " << h_off.b();
      ImGui::TextDisabled(h_off_ss.str().c_str());

      std::stringstream v_off_ss;
      v_off_ss << "C(r,g,b): " << v_off.r() << ", " << v_off.g() << ", " << v_off.b();
      ImGui::TextDisabled(v_off_ss.str().c_str());

      std::stringstream num_cols_ss;
      num_cols_ss << "Colour Range: 0 - " << num_colours;
      ImGui::TextDisabled(num_cols_ss.str().c_str());

      // TODO: Button to export
      //ImGui::Button("Export", {100, 20});
      ImGui::End();
      // Clear to black
      window.clear(sf::Color(bg_col[0] * 255, bg_col[1] * 255, bg_col[2] * 255)); // fill background with color

      // How many rectangles
      float square_width = static_cast<float>(width) / static_cast<float>(num_colours);
      float square_height = 50.0f;
      sf::Vector2f square_size(square_width, square_height);
      int strip_margin = 10;
      std::vector<colour> colours;
      for (int i = 0; i < num_colours; ++i)
      {
         colour square_color = sine_colour_palette(static_cast<float>(i), freq, v_off, h_off, amp);
         colours.push_back(square_color);
         // Draw red channel
         sf::Vector2f red_centre(i * square_width, 0);
         draw_rectangle(window, sf::Color(square_color.r() * 255, 0, 0, 255), square_size, red_centre);
         // Draw green channel
         sf::Vector2f green_centre(i * square_width, square_height + strip_margin);
         draw_rectangle(window, sf::Color(0, square_color.g() * 255, 0, 255), square_size, green_centre);
         // Draw blue channel
         sf::Vector2f blue_centre(i * square_width, 2 * (square_height + strip_margin));
         draw_rectangle(window, sf::Color(0, 0, square_color.b() * 255, 255), square_size, blue_centre);
         // Draw total colour
         sf::Vector2f centre(i*square_width, 3 * (square_height + 2*strip_margin));
         draw_rectangle(window, sf::Color(square_color.to_int()), square_size, centre);
      }

      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SFML::Shutdown();
   return 0;
}