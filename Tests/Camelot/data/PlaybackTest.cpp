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

#include <gtest/gtest.h>

#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/Playback.h"

namespace camelot {

namespace {

constexpr Time kSecond = 1'000'000'000ULL;
constexpr Time kStart = 100 * kSecond;
constexpr Time kEnd = 110 * kSecond;

Playback tenSeconds() {
  Playback playback;
  playback.setRange(kStart, kEnd);
  return playback;
}

}  // namespace

TEST(PlaybackTest, StartsPausedAtTheBeginning) {
  Playback playback = tenSeconds();
  EXPECT_EQ(playback.start(), kStart);
  EXPECT_EQ(playback.end(), kEnd);
  EXPECT_EQ(playback.current(), kStart);
  EXPECT_FALSE(playback.isPlaying());
  EXPECT_FALSE(playback.atEnd());
  EXPECT_DOUBLE_EQ(playback.progress(), 0.0);
  EXPECT_DOUBLE_EQ(playback.speed(), 1.0);
  EXPECT_FALSE(playback.loop());
  const Playback::Range range = playback.advance(1.0);
  EXPECT_TRUE(range.empty());
  EXPECT_EQ(range.from, kStart);
  EXPECT_EQ(playback.current(), kStart);
}

TEST(PlaybackTest, AdvancesWithWallTimeWhilePlaying) {
  Playback playback = tenSeconds();
  playback.play();
  EXPECT_TRUE(playback.isPlaying());
  const Playback::Range first = playback.advance(0.5);
  EXPECT_EQ(first.from, kStart);
  EXPECT_EQ(first.to, kStart + kSecond / 2);
  EXPECT_FALSE(first.wrapped);
  EXPECT_EQ(playback.current(), first.to);
  EXPECT_DOUBLE_EQ(playback.progress(), 0.05);
  // Consecutive ticks partition the time line.
  const Playback::Range second = playback.advance(0.25);
  EXPECT_EQ(second.from, first.to);
  EXPECT_EQ(second.to, kStart + 3 * kSecond / 4);
  // A zero-length tick applies nothing.
  EXPECT_TRUE(playback.advance(0.0).empty());
}

TEST(PlaybackTest, SpeedScalesTheAdvance) {
  Playback playback = tenSeconds();
  playback.play();
  playback.setSpeed(4.0);
  EXPECT_EQ(playback.advance(0.5).to, kStart + 2 * kSecond);
  playback.setSpeed(0.5);
  EXPECT_EQ(playback.advance(1.0).to, kStart + 2 * kSecond + kSecond / 2);
}

TEST(PlaybackTest, SpeedIsClamped) {
  Playback playback = tenSeconds();
  playback.setSpeed(0.0);
  EXPECT_DOUBLE_EQ(playback.speed(), Playback::kMinSpeed);
  playback.setSpeed(-3.0);
  EXPECT_DOUBLE_EQ(playback.speed(), Playback::kMinSpeed);
  playback.setSpeed(1e9);
  EXPECT_DOUBLE_EQ(playback.speed(), Playback::kMaxSpeed);
}

TEST(PlaybackTest, SubNanosecondRemaindersAccumulate) {
  Playback playback = tenSeconds();
  playback.play();
  playback.setSpeed(Playback::kMinSpeed);
  // 0.6 ns per tick: the first tick moves nothing, the second one moves 1 ns.
  EXPECT_TRUE(playback.advance(6e-7).empty());
  const Playback::Range range = playback.advance(6e-7);
  EXPECT_EQ(range.to - range.from, 1U);
}

TEST(PlaybackTest, StopsAtTheEndAndIncludesTheLastMessage) {
  Playback playback = tenSeconds();
  playback.play();
  playback.seek(kEnd - kSecond);
  const Playback::Range range = playback.advance(5.0);
  EXPECT_EQ(range.from, kEnd - kSecond);
  EXPECT_EQ(range.to, kEnd + 1);  // half-open, so the end itself is applied
  EXPECT_FALSE(range.wrapped);
  EXPECT_EQ(playback.current(), kEnd);
  EXPECT_TRUE(playback.atEnd());
  EXPECT_FALSE(playback.isPlaying());
  EXPECT_DOUBLE_EQ(playback.progress(), 1.0);
  EXPECT_TRUE(playback.advance(1.0).empty());
}

TEST(PlaybackTest, PlayAtTheEndRestartsFromTheBeginning) {
  Playback playback = tenSeconds();
  playback.seek(kEnd);
  EXPECT_TRUE(playback.atEnd());
  playback.play();
  EXPECT_EQ(playback.current(), kStart);
  EXPECT_TRUE(playback.isPlaying());
}

TEST(PlaybackTest, LoopWrapsToTheStart) {
  Playback playback = tenSeconds();
  playback.setLoop(true);
  EXPECT_TRUE(playback.loop());
  playback.play();
  playback.seek(kEnd - kSecond);
  const Playback::Range range = playback.advance(2.0);
  EXPECT_EQ(range.from, kEnd - kSecond);
  EXPECT_EQ(range.to, kEnd + 1);
  EXPECT_TRUE(range.wrapped);
  EXPECT_EQ(playback.current(), kStart);
  EXPECT_TRUE(playback.isPlaying());
  const Playback::Range next = playback.advance(1.0);
  EXPECT_EQ(next.from, kStart);
  EXPECT_EQ(next.to, kStart + kSecond);
  EXPECT_FALSE(next.wrapped);
}

TEST(PlaybackTest, SeekClampsAndKeepsThePlayingState) {
  Playback playback = tenSeconds();
  playback.seek(kEnd + 5 * kSecond);
  EXPECT_EQ(playback.current(), kEnd);
  playback.seek(0);
  EXPECT_EQ(playback.current(), kStart);
  playback.seek(kStart + 3 * kSecond);
  EXPECT_EQ(playback.current(), kStart + 3 * kSecond);
  EXPECT_DOUBLE_EQ(playback.progress(), 0.3);
  EXPECT_FALSE(playback.isPlaying());
  playback.play();
  playback.seek(kStart + 7 * kSecond);
  EXPECT_TRUE(playback.isPlaying());
  const Playback::Range range = playback.advance(1.0);
  EXPECT_EQ(range.from, kStart + 7 * kSecond);
  EXPECT_EQ(range.to, kStart + 8 * kSecond);
}

TEST(PlaybackTest, ToggleAndPause) {
  Playback playback = tenSeconds();
  playback.toggle();
  EXPECT_TRUE(playback.isPlaying());
  playback.toggle();
  EXPECT_FALSE(playback.isPlaying());
  playback.play();
  playback.pause();
  EXPECT_FALSE(playback.isPlaying());
  EXPECT_TRUE(playback.advance(1.0).empty());
}

TEST(PlaybackTest, SetRangeRewindsAndClampsAnInvertedRange) {
  Playback playback = tenSeconds();
  playback.seek(kStart + 5 * kSecond);
  playback.setRange(kStart, kStart);
  EXPECT_EQ(playback.current(), kStart);
  EXPECT_DOUBLE_EQ(playback.progress(), 0.0);
  EXPECT_TRUE(playback.atEnd());
  playback.setRange(kEnd, kStart);
  EXPECT_EQ(playback.start(), kEnd);
  EXPECT_EQ(playback.end(), kEnd);
}

TEST(PlaybackTest, EmptyRangeAppliesTheSingleInstant) {
  Playback playback;
  playback.setRange(kStart, kStart);
  playback.play();
  const Playback::Range range = playback.advance(0.1);
  EXPECT_EQ(range.from, kStart);
  EXPECT_EQ(range.to, kStart + 1);
  EXPECT_FALSE(playback.isPlaying());
}

}  // namespace camelot
