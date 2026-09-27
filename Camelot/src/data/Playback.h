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

#ifndef CAMELOT_SRC_DATA_PLAYBACK_H_
#define CAMELOT_SRC_DATA_PLAYBACK_H_

#include "Camelot/src/data/FoxgloveMessages.h"

namespace camelot {

// The playback clock of a recording: a position inside [start, end] that
// advances with wall time times the speed while playing. No I/O, no
// rendering; the owner applies the messages of every interval advance()
// returns.
class Playback {
 public:
  // A half-open interval [from, to) of recording time to apply. When the
  // tick reaches the end, `to` is end + 1 so the messages logged exactly at
  // `end` are included. `wrapped` is set on the tick that looped back to the
  // start: the owner should reload its state at `start` before continuing.
  struct Range {
    Time from{0};
    Time to{0};
    bool wrapped{false};

    [[nodiscard]] bool empty() const { return from == to; }
  };

  static constexpr double kMinSpeed = 0.001;
  static constexpr double kMaxSpeed = 1000.0;

  // Sets the recording bounds and rewinds to `start`; `end` < `start` is
  // clamped to `start`.
  void setRange(Time start, Time end);
  void play();
  void pause();
  void toggle();
  // Clamped to [kMinSpeed, kMaxSpeed].
  void setSpeed(double speed);
  // Clamped to the range. Does not change the playing state.
  void seek(Time t);
  void setLoop(bool loop);

  // Moves the clock by `wallSeconds` of wall time and returns the interval
  // to apply; empty while paused. Without loop the clock stops at the end.
  Range advance(double wallSeconds);

  [[nodiscard]] Time start() const { return m_start; }
  [[nodiscard]] Time end() const { return m_end; }
  [[nodiscard]] Time current() const { return m_current; }
  // Position in 0..1 (0 for an empty range).
  [[nodiscard]] double progress() const;
  [[nodiscard]] double speed() const { return m_speed; }
  [[nodiscard]] bool isPlaying() const { return m_playing; }
  [[nodiscard]] bool loop() const { return m_loop; }
  [[nodiscard]] bool atEnd() const { return m_current >= m_end; }

 private:
  Time m_start{0};
  Time m_end{0};
  Time m_current{0};
  double m_speed{1.0};
  bool m_playing{false};
  bool m_loop{false};
  // Sub-nanosecond remainder of the last advance, so slow speeds still move.
  double m_fraction{0.0};

  [[nodiscard]] Time clamp(Time t) const;
};

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_PLAYBACK_H_
