/** @file

    MemArena unit tests.

    @section license License

    Licensed to the Apache Software Foundation (ASF) under one or more contributor license
    agreements.  See the NOTICE file distributed with this work for additional information regarding
    copyright ownership.  The ASF licenses this file to you under the Apache License, Version 2.0
    (the "License"); you may not use this file except in compliance with the License.  You may
    obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string_view>
#include <random>
#include "swoc/MemArena.h"
#include "swoc/TextView.h"
#include "catch.hpp"

using swoc::MemSpan;
using swoc::MemArena;
using swoc::FixedArena;
using std::string_view;
using swoc::TextView;
using namespace std::literals;

static constexpr std::string_view CHARS{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/."};
std::uniform_int_distribution<short> char_gen{0, short(CHARS.size() - 1)};
std::minstd_rand randu;

namespace
{
TextView
localize(MemArena &arena, TextView const &view)
{
  auto span = arena.alloc(view.size()).rebind<char>();
  memcpy(span, view);
  return span.view();
}
} // namespace

TEST_CASE("MemArena generic", "[libswoc][MemArena]")
{
  swoc::MemArena arena{64};
  REQUIRE(arena.size() == 0);
  REQUIRE(arena.reserved_size() == 0);
  arena.alloc(0);
  REQUIRE(arena.size() == 0);
  REQUIRE(arena.reserved_size() >= 64);
  REQUIRE(arena.remaining() >= 64);

  swoc::MemSpan span1 = arena.alloc(32);
  REQUIRE(span1.size() == 32);
  REQUIRE(arena.remaining() >= 32);

  swoc::MemSpan span2 = arena.alloc(32);
  REQUIRE(span2.size() == 32);

  REQUIRE(span1.data() != span2.data());
  REQUIRE(arena.size() == 64);

  auto extent{arena.reserved_size()};
  span1 = arena.alloc(128);
  REQUIRE(extent < arena.reserved_size());
}

TEST_CASE("MemArena freeze and thaw", "[libswoc][MemArena]")
{
  MemArena arena;
  MemSpan span1{arena.alloc(1024)};
  REQUIRE(span1.size() == 1024);
  REQUIRE(arena.size() == 1024);
  REQUIRE(arena.reserved_size() >= 1024);

  arena.freeze();

  REQUIRE(arena.size() == 0);
  REQUIRE(arena.allocated_size() == 1024);
  REQUIRE(arena.reserved_size() >= 1024);

  arena.thaw();
  REQUIRE(arena.size() == 0);
  REQUIRE(arena.allocated_size() == 0);
  REQUIRE(arena.reserved_size() == 0);

  span1 = arena.alloc(1024);
  arena.freeze();
  auto extent{arena.reserved_size()};
  arena.alloc(512);
  REQUIRE(arena.reserved_size() > extent); // new extent should be bigger.
  arena.thaw();
  REQUIRE(arena.size() == 512);
  REQUIRE(arena.reserved_size() >= 1024);

  arena.clear();
  REQUIRE(arena.size() == 0);
  REQUIRE(arena.reserved_size() == 0);

  span1 = arena.alloc(262144);
  arena.freeze();
  extent = arena.reserved_size();
  arena.alloc(512);
  REQUIRE(arena.reserved_size() > extent); // new extent should be bigger.
  arena.thaw();
  REQUIRE(arena.size() == 512);
  REQUIRE(arena.reserved_size() >= 262144);

  arena.clear();

  span1  = arena.alloc(262144);
  extent = arena.reserved_size();
  arena.freeze();
  for (int i = 0; i < 262144 / 512; ++i)
    arena.alloc(512);
  REQUIRE(arena.reserved_size() > extent); // Bigger while frozen memory is still around.
  arena.thaw();
  REQUIRE(arena.size() == 262144);
  REQUIRE(arena.reserved_size() == extent); // should be identical to before freeze.

  arena.alloc(512);
  arena.alloc(768);
  arena.freeze(32000);
  arena.thaw();
  arena.alloc(0);
  REQUIRE(arena.reserved_size() >= 32000);
  REQUIRE(arena.reserved_size() < 2 * 32000);
}

TEST_CASE("MemArena helper", "[libswoc][MemArena]")
{
  struct Thing {
    int ten{10};
    std::string name{"name"};

    Thing() {}
    Thing(int x) : ten(x) {}
    Thing(std::string const &s) : name(s) {}
    Thing(int x, std::string_view s) : ten(x), name(s) {}
    Thing(std::string const &s, int x) : ten(x), name(s) {}
  };

  swoc::MemArena arena{256};
  REQUIRE(arena.size() == 0);
  swoc::MemSpan s = arena.alloc(56).rebind<char>();
  REQUIRE(arena.size() == 56);
  REQUIRE(arena.remaining() >= 200);
  void *ptr = s.begin();

  REQUIRE(arena.contains((char *)ptr));
  REQUIRE(arena.contains((char *)ptr + 100)); // even though span isn't this large, this pointer should still be in arena
  REQUIRE(!arena.contains((char *)ptr + 300));
  REQUIRE(!arena.contains((char *)ptr - 1));

  arena.freeze(128);
  REQUIRE(arena.contains((char *)ptr));
  REQUIRE(arena.contains((char *)ptr + 100));
  swoc::MemSpan s2 = arena.alloc(10).rebind<char>();
  void *ptr2       = s2.begin();
  REQUIRE(arena.contains((char *)ptr));
  REQUIRE(arena.contains((char *)ptr2));
  REQUIRE(arena.allocated_size() == 56 + 10);

  arena.thaw();
  REQUIRE(!arena.contains((char *)ptr));
  REQUIRE(arena.contains((char *)ptr2));

  Thing *thing_one{arena.make<Thing>()};

  REQUIRE(thing_one->ten == 10);
  REQUIRE(thing_one->name == "name");

  thing_one = arena.make<Thing>(17, "bob"sv);

  REQUIRE(thing_one->name == "bob");
  REQUIRE(thing_one->ten == 17);

  thing_one = arena.make<Thing>("Dave", 137);

  REQUIRE(thing_one->name == "Dave");
  REQUIRE(thing_one->ten == 137);

  thing_one = arena.make<Thing>(9999);

  REQUIRE(thing_one->ten == 9999);
  REQUIRE(thing_one->name == "name");

  thing_one = arena.make<Thing>("Persia");

  REQUIRE(thing_one->ten == 10);
  REQUIRE(thing_one->name == "Persia");
}

TEST_CASE("MemArena large alloc", "[libswoc][MemArena]")
{
  swoc::MemArena arena;
  auto s = arena.alloc(4000);
  REQUIRE(s.size() == 4000);

  decltype(s) s_a[10];
  s_a[0] = arena.alloc(100);
  s_a[1] = arena.alloc(200);
  s_a[2] = arena.alloc(300);
  s_a[3] = arena.alloc(400);
  s_a[4] = arena.alloc(500);
  s_a[5] = arena.alloc(600);
  s_a[6] = arena.alloc(700);
  s_a[7] = arena.alloc(800);
  s_a[8] = arena.alloc(900);
  s_a[9] = arena.alloc(1000);

  // ensure none of the spans have any overlap in memory.
  for (int i = 0; i < 10; ++i) {
    s = s_a[i];
    for (int j = i + 1; j < 10; ++j) {
      REQUIRE(s_a[i].data() != s_a[j].data());
    }
  }
}

TEST_CASE("MemArena block allocation", "[libswoc][MemArena]")
{
  swoc::MemArena arena{64};
  swoc::MemSpan s  = arena.alloc(32).rebind<char>();
  swoc::MemSpan s2 = arena.alloc(16).rebind<char>();
  swoc::MemSpan s3 = arena.alloc(16).rebind<char>();

  REQUIRE(s.size() == 32);
  REQUIRE(arena.allocated_size() == 64);

  REQUIRE(arena.contains((char *)s.begin()));
  REQUIRE(arena.contains((char *)s2.begin()));
  REQUIRE(arena.contains((char *)s3.begin()));

  REQUIRE((char *)s.begin() + 32 == (char *)s2.begin());
  REQUIRE((char *)s.begin() + 48 == (char *)s3.begin());
  REQUIRE((char *)s2.begin() + 16 == (char *)s3.begin());

  REQUIRE(s.end() == s2.begin());
  REQUIRE(s2.end() == s3.begin());
  REQUIRE((char *)s.begin() + 64 == s3.end());
}

TEST_CASE("MemArena full blocks", "[libswoc][MemArena]")
{
  // couple of large allocations - should be exactly sized in the generation.
  size_t init_size = 32000;
  swoc::MemArena arena(init_size);

  MemSpan m1{arena.alloc(init_size - 64).rebind<uint8_t>()};
  MemSpan m2{arena.alloc(32000).rebind<unsigned char>()};
  MemSpan m3{arena.alloc(64000).rebind<char>()};

  REQUIRE(arena.remaining() >= 64);
  REQUIRE(arena.reserved_size() > 32000 + 64000 + init_size);
  REQUIRE(arena.reserved_size() < 2 * (32000 + 64000 + init_size));

  // Let's see if that memory is really there.
  memset(m1, 0xa5);
  memset(m2, 0xc2);
  memset(m3, 0x56);

  REQUIRE(std::all_of(m1.begin(), m1.end(), [](uint8_t c) { return 0xa5 == c; }));
  REQUIRE(std::all_of(m2.begin(), m2.end(), [](unsigned char c) { return 0xc2 == c; }));
  REQUIRE(std::all_of(m3.begin(), m3.end(), [](char c) { return 0x56 == c; }));
}

TEST_CASE("MemArena esoterica", "[libswoc][MemArena]")
{
  MemArena a1;
  MemSpan<char> span;
  {
    MemArena a2{512};
    span = a2.alloc(128).rebind<char>();
    REQUIRE(a2.contains(span.data()));
    a1 = std::move(a2);
  }
  REQUIRE(a1.contains(span.data()));
  REQUIRE(a1.remaining() >= 384);

  {
    MemArena *arena = MemArena::construct_self_contained();
    arena->~MemArena();
  }

  {
    std::unique_ptr<MemArena, void (*)(MemArena *)> arena(MemArena::construct_self_contained(),
                                                          [](MemArena *arena) -> void { arena->~MemArena(); });
    static constexpr unsigned MAX = 512;
    std::uniform_int_distribution<unsigned> length_gen{6, MAX};
    char buffer[MAX];
    for (unsigned i = 0; i < 50; ++i) {
      auto n = length_gen(randu);
      for (unsigned k = 0; k < n; ++k) {
        buffer[k] = CHARS[char_gen(randu)];
      }
      localize(*arena, {buffer, n});
    }
    // Really, at this point just make sure there's no memory corruption on destruction.
  }
}

// --- temporary allocation
TEST_CASE("MemArena temporary", "[libswoc][MemArena][tmp]")
{
  MemArena arena;
  std::vector<std::string_view> strings;

  static constexpr short MAX{8000};
  static constexpr int N{100};

  std::uniform_int_distribution<unsigned> length_gen{100, MAX};
  std::array<char, MAX> url;

  REQUIRE(arena.remaining() == 0);
  int i;
  unsigned max{0};
  for (i = 0; i < N; ++i) {
    auto n = length_gen(randu);
    max    = std::max(max, n);
    arena.require(n);
    auto span = arena.remnant().rebind<char>();
    if (span.size() < n)
      break;
    for (auto j = n; j > 0; --j) {
      span[j - 1] = url[j - 1] = CHARS[char_gen(randu)];
    }
    if (string_view{span.data(), n} != string_view{url.data(), n})
      break;
  }
  REQUIRE(i == N);            // did all the loops.
  REQUIRE(arena.size() == 0); // nothing actually allocated.
  // Hard to get a good value, but shouldn't be more than twice.
  REQUIRE(arena.reserved_size() < 2 * MAX);
  // Should be able to allocate at least the longest string without increasing the reserve size.
  unsigned rsize = arena.reserved_size();
  auto count     = max;
  std::uniform_int_distribution<unsigned> alloc_size{32, 128};
  while (count >= 128) { // at least the max distribution value
    auto k = alloc_size(randu);
    arena.alloc(k);
    count -= k;
  }
  REQUIRE(arena.reserved_size() == rsize);

  // Check for switching full blocks - calculate something like the total free space
  // and then try to allocate most of it without increasing the reserved size.
  count = rsize - (max - count);
  while (count >= 128) {
    auto k = alloc_size(randu);
    arena.alloc(k);
    count -= k;
  }
  REQUIRE(arena.reserved_size() == rsize);
}

TEST_CASE("FixedArena", "[libswoc][FixedArena]") {
  struct Thing {
    int x = 0;
    std::string name;
  };
  MemArena arena;
  FixedArena<Thing> fa{arena};

  Thing * one = fa.make();
  Thing * two = fa.make();
  two->x = 17;
  two->name = "Bob";
  fa.destroy(two);
  Thing * three = fa.make();
  REQUIRE(three == two); // reused instance.
  REQUIRE(three->x == 0); // but reconstructed.
  REQUIRE(three->name.empty() == true);
  fa.destroy(three);
  std::array<Thing*, 17> things;
  for ( auto & ptr : things ) {
    ptr = fa.make();
  }
  two = things[things.size() - 1];
  for ( auto & ptr : things) {
    fa.destroy(ptr);
  }
  three = fa.make();
  REQUIRE(two == three);
};
