// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

// Klugy, but includes need to be in this order.

#include <local_sketches.hpp>

#if __has_include(<cereal/cereal.hpp>)
#include <check_archive.hpp>
#endif

#include <linearsketch_test.hpp>

int main(int argc, char **argv) { do_main(argc, argv); }
