#ifndef CORE_PERFORMANCESTATS_HPP
#define CORE_PERFORMANCESTATS_HPP

#include <array>
#include <cstddef>

struct PerformanceStats
{
  float FrameTime = 0.0F;
  float Fps = 0.0F;
};

class PerformanceTracker
{
public:
  void Update(float delta_time)
  {
    m_FrameTimes[m_Index] = delta_time;
    m_Index = (m_Index + 1) % SAMPLE_COUNT;
    if (m_Count < SAMPLE_COUNT) {
      ++m_Count;
    }

    float sum = 0.0F;
    for (size_t i = 0; i < m_Count; ++i) {
      sum += m_FrameTimes[i];
    }

    const float avg_frame_time = sum / static_cast<float>(m_Count);
    m_Stats.FrameTime = avg_frame_time;
    m_Stats.Fps = avg_frame_time > 0.0F ? 1.0F / avg_frame_time : 0.0F;
  }

  [[nodiscard]] auto GetStats() const -> const PerformanceStats&
  {
    return m_Stats;
  }

private:
  static constexpr size_t SAMPLE_COUNT = 100;

  std::array<float, SAMPLE_COUNT> m_FrameTimes {};
  size_t m_Index = 0;
  size_t m_Count = 0;
  PerformanceStats m_Stats {};
};

#endif
