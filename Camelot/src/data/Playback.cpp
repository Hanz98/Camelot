/*
 * Copyright 2024 Jan Filip
 *
 * Licensed under the MIT License. You may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Camelot/src/data/Playback.h"

#include <algorithm>
#include <cmath>

namespace camelot {

namespace {

constexpr double kNanosPerSecond = 1e9;

}  // namespace

void Playback::setRange(Time start, Time end) {
  m_start = start;
  m_end = std::max(start, end);
  m_current = start;
  m_fraction = 0.0;
}

void Playback::play() {
  if (atEnd() && !m_loop) {
    m_current = m_start;
  }
  m_playing = true;
}

void Playback::pause() { m_playing = false; }

void Playback::toggle() {
  if (m_playing) {
    pause();
  } else {
    play();
  }
}

void Playback::setSpeed(double speed) {
  m_speed = std::clamp(speed, kMinSpeed, kMaxSpeed);
}

void Playback::seek(Time t) {
  m_current = clamp(t);
  m_fraction = 0.0;
}

void Playback::setLoop(bool loop) { m_loop = loop; }

Time Playback::clamp(Time t) const { return std::clamp(t, m_start, m_end); }

Playback::Range Playback::advance(double wallSeconds) {
  Range range{.from = m_current, .to = m_current, .wrapped = false};
  if (!m_playing || wallSeconds <= 0.0) {
    return range;
  }
  const double nanos =
      std::max(0.0, wallSeconds) * m_speed * kNanosPerSecond + m_fraction;
  const double whole = std::floor(nanos);
  m_fraction = nanos - whole;
  const auto step = static_cast<Time>(whole);

  const Time remaining = m_end - m_current;
  if (step < remaining) {
    m_current += step;
    range.to = m_current;
    return range;
  }
  // The tick reaches the end: apply everything up to and including `end`.
  range.to = m_end + 1;
  if (m_loop) {
    m_current = m_start;
    range.wrapped = true;
  } else {
    m_current = m_end;
    m_playing = false;
  }
  return range;
}

double Playback::progress() const {
  if (m_end <= m_start) {
    return 0.0;
  }
  return static_cast<double>(m_current - m_start) /
         static_cast<double>(m_end - m_start);
}

}  // namespace camelot
